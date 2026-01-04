Critical Code Review: OpenKey Vietnamese IME Patch
Executive Summary
This patch contains significant architectural changes to a Vietnamese IME tool, including:

Performance optimizations in the engine (O(1) lookup tables)
Subprocess-based UI dialogs (Sciter migration)
Keyboard hook modifications with early-exit logic
IPC mechanisms between processes
Smart Switch Key deferred save mechanism
Process name caching
I've identified several critical issues that can cause input problems, race conditions, and application failures.

CRITICAL ISSUES
1. Race Condition in ReinstallHooks() with Sleep(100)
File: OpenKey.cpp (lines 2918-2977)

cpp
// Small delay to ensure hooks are fully released
Sleep(100);
What's Wrong:

Sleep() inside a mutex-protected section causes deadlock risk
100ms is arbitrary and may not be sufficient on slow systems
If called from the keyboard hook thread, this blocks the entire input pipeline
Failure Scenario: User locks/unlocks screen repeatedly. ReinstallHooks() is called, but the 100ms isn't enough. Old hooks aren't fully released, new ones fail. Result: complete keyboard input loss.

Fix:

cpp
// Instead of Sleep, verify hooks are actually released
for (int attempt = 0; attempt < 10; attempt++) {
    hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboardHookProcess, hInstance, 0);
    if (hKeyboardHook) break;
    Sleep(50);  // Retry with backoff
}
2. AttachThreadInput Misuse in forceForegroundWindow()
File: ExcludedAppsDialogSciter.cpp (lines 1037-1062)

cpp
AttachThreadInput(dwCurrentThread, dwForegroundThread, TRUE);
SetForegroundWindow(hwnd);
// ...
AttachThreadInput(dwCurrentThread, dwForegroundThread, FALSE);
What's Wrong:

AttachThreadInput changes keyboard focus behavior globally
If SetForegroundWindow fails or throws, the detach never happens
This can cause sticky focus where input goes to wrong window
Failure Scenario: User clicks dropdown in browser, forceForegroundWindow is called, AttachThreadInput succeeds but SetForegroundWindow fails (window destroyed). The threads remain attached. All subsequent keyboard input may go to wrong process.

Fix:

cpp
// Use RAII guard
struct ThreadInputGuard {
    DWORD from, to;
    bool attached = false;
    ThreadInputGuard(DWORD f, DWORD t) : from(f), to(t) {
        if (from != to) attached = AttachThreadInput(from, to, TRUE);
    }
    ~ThreadInputGuard() { if (attached) AttachThreadInput(from, to, FALSE); }
};
3. Early Exit in English Mode Duplicates Hotkey Logic
File: OpenKey.cpp (lines 3146-3197)

cpp
// OPTIMIZATION P1.1 (REVISED): Early exit for English mode
if (vLanguage == 0) {
    // Still need to handle language switch hotkey in English mode
    if ((wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) && !_isFlagKey && _keycode != 0) {
        if (GET_SWITCH_KEY(vSwitchKeyStatus) == _keycode && 
            // ... duplicate logic
What's Wrong:

The hotkey handling logic is now duplicated in English mode path AND Vietnamese mode path
Any fix to hotkey behavior must be applied in two places
The macro handling in English mode (line 3186-3196) doesn't properly check !_isFlagKey
Failure Scenario: User presses Ctrl+Z (undo) while in English mode. If vUseMacro && vUseMacroInEnglishMode is true, the code calls vEnglishMode() even though Ctrl is held. This may interfere with the undo operation.

Fix: Refactor to single unified hotkey handler called from both paths, or use goto to common code:

cpp
// Before early exit, check hotkeys in unified manner
if (!handleGlobalHotkeys()) {
    if (vLanguage == 0) return CallNextHookEx(...);
    // Continue Vietnamese processing
}
4. Deferred Save Timer Without Thread Safety
File: OpenKey.cpp (lines 3023-3042)

cpp
static bool _smartSwitchKeyDirty = false;
static UINT_PTR _smartSwitchSaveTimerId = 0;
void markSmartSwitchKeyDirty() {
    _smartSwitchKeyDirty = true;
    if (_smartSwitchSaveTimerId != 0) {
        KillTimer(NULL, _smartSwitchSaveTimerId);  // Not thread-safe!
    }
    _smartSwitchSaveTimerId = SetTimer(NULL, 0, SMART_SWITCH_SAVE_DELAY_MS, smartSwitchKeySaveTimerProc);
}
What's Wrong:

markSmartSwitchKeyDirty() is called from winEventProcCallback which runs on a different thread than the timer callback
KillTimer(NULL, ...) with NULL hwnd is problematic - timer may fire between check and kill
No synchronization on _smartSwitchKeyDirty
Failure Scenario: Rapid app switching causes multiple markSmartSwitchKeyDirty() calls. Timer callback starts executing, sets _smartSwitchKeyDirty = false, but another call sets it true again before KillTimer. Data may be lost or saved twice.

Fix:

cpp
static std::atomic<bool> _smartSwitchKeyDirty{false};
static HWND _timerHwnd = NULL;  // Use main window hwnd
void markSmartSwitchKeyDirty() {
    _smartSwitchKeyDirty.store(true, std::memory_order_release);
    // Timer management should happen on main thread via PostMessage
}
5. SendMessageTimeout with 10ms Timeout May Still Block
File: OpenKey.cpp (lines 3208-3214)

cpp
if (SendMessageTimeout(hIME, WM_IME_CONTROL, IMC_GETOPENSTATUS, 0, 
                       SMTO_ABORTIFHUNG | SMTO_BLOCK, 10, &dwResult)) {
What's Wrong:

10ms timeout is still blocking in a low-level keyboard hook
SMTO_BLOCK means the call WILL block for up to 10ms
A hung IME window will cause 10ms latency on EVERY keystroke
Failure Scenario: User has Chinese IME installed but the IME process is slow. Every keystroke in ANY app experiences 10ms latency because the hook waits for the IME check.

Fix:

cpp
// Option 1: Use SMTO_ABORTIFHUNG only (non-blocking for hung windows)
if (SendMessageTimeout(hIME, WM_IME_CONTROL, IMC_GETOPENSTATUS, 0, 
                       SMTO_ABORTIFHUNG, 1, &dwResult))
// Option 2: Cache the result and check asynchronously
static LRESULT cachedImeStatus = 0;
static DWORD lastImeCheck = 0;
if (GetTickCount() - lastImeCheck > 100) {
    PostMessage(internalHwnd, WM_CHECK_IME, ...);  // Check on separate thread
}
6. ExitProcess(0) in Subprocess Closes Without Cleanup
File: Multiple dialog files (AboutDialog.cpp, SettingsDialog.cpp, etc.)

cpp
if (msg == WM_CLOSE) {
    ExitProcess(0);  // Force exit subprocess
    return 0;
}
What's Wrong:

ExitProcess(0) bypasses C++ destructors, RAII cleanup, DLL unload hooks
If subprocess modified any shared state (clipboard, registry writes in progress), it may corrupt data
Any pending IPC messages are lost
Failure Scenario: User closes Settings dialog while registry write is in progress (from saveEnglishOnlyAppsData). The write is interrupted mid-way. Next launch, registry data is corrupted.

Fix:

cpp
if (msg == WM_CLOSE) {
    // Flush pending operations
    FlushPendingWrites();  // Ensure registry writes complete
    PostQuitMessage(0);  // Clean exit
    return 0;
}
7. Process Cache Can Return Stale Data After Process Exit
File: OpenKeyHelper.cpp (lines 3446-3630)

cpp
static std::unordered_map<DWORD, std::string> _processNameCache;
// ...
auto cacheIt = _processNameCache.find(_tempProcessId);
if (cacheIt != _processNameCache.end()) {
    _exeNameUtf8 = cacheIt->second;
    return _exeNameUtf8;
}
What's Wrong:

PIDs are reused by Windows after process exits
Cache has no TTL or validation
A new process with same PID will get wrong cached name
Failure Scenario: User opens Notepad (PID 1234), then closes it. Later opens Chrome (reuses PID 1234). Cache returns "notepad.exe" for Chrome. Smart Switch Key applies notepad settings to Chrome. Vietnamese typing broken in Chrome.

Fix:

cpp
// Validate cache entry is still valid
auto cacheIt = _processNameCache.find(_tempProcessId);
if (cacheIt != _processNameCache.end()) {
    // Quick validation: check if process still exists with same name
    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, _tempProcessId);
    if (hProc) {
        WCHAR currentName[MAX_PATH];
        // ... verify name matches cache
        CloseHandle(hProc);
    }
}
8. Backspace After tempDisableKey = false Reset May Cause State Corruption
File: Engine.cpp (lines 167-185)

cpp
// FIX: Reset Vietnamese mode flag to allow re-evaluation with fresh spell check
// Bug: After backspace, tempDisableKey was left true from previous spell check,
// causing engine to incorrectly stay in English mode
tempDisableKey = false;
What's Wrong:

This unconditionally resets tempDisableKey after every backspace
But tempDisableKey might have been set intentionally by user action (e.g., Ctrl held)
Could cause Vietnamese chars to appear when user expects English
Failure Scenario: User types Ctrl to temporarily disable spelling, then presses backspace. The patch resets tempDisableKey = false, enabling spelling again without user releasing Ctrl.

Fix:

cpp
// Only reset if spell check is enabled and no modifier keys are held
if (vCheckSpelling && !(_flag & MASK_CONTROL)) {
    tempDisableKey = false;
}
9. Qt/Electron App List is Hardcoded and Incomplete
File: OpenKey.cpp (lines 3036-3049)

cpp
static vector<string> _qtElectronApps = {
    "NotepadNext.exe", "notepad-next.exe",
    "Code.exe", "code.exe",
    // ...
};
What's Wrong:

Hardcoded list will become outdated as apps update
Case-sensitivity issues (VSCode might also be "Code - Insiders.exe")
No way for users to customize
Recommendation: Make this configurable via registry or config file, OR detect Qt/Electron at runtime by checking for loaded DLLs like Qt5Core.dll or libnode.dll.

MEDIUM ISSUES
10. Mutex in Settings Controller May Deadlock
File: OpenKeySettingsController.cpp

cpp
void OpenKeySettingsController::updateSmartSwitchKeyData() {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    // ...
    saveSmartSwitchKeyData();  // May trigger registry I/O
}
Holding mutex while doing I/O is risky if the I/O callback tries to acquire the same mutex.

11. SetSystemCursor Not Restored on Crash
File: ExcludedAppsDialogSciter.cpp (lines 1544-1573)

cpp
SetSystemCursor(CopyCursor(hCross), OCR_NORMAL);  // Changes GLOBAL system cursor
// ...
stopWindowPicking() {
    SetSystemCursor(m_hSavedArrowCursor, OCR_NORMAL);  // Restore
}
If app crashes while picking, the system cursor is stuck as crosshair until reboot.

Fix: Use SetCursor() with mouse capture instead of modifying system cursor.

12. vKeyInit() Called Multiple Times
File: Engine.cpp

cpp
void* vKeyInit() {
    // ...
    initLookupTables();  // Called every init
    static bool isMapInitialized = false;
    if (!isMapInitialized || _keyCodeToChar.size() == 0) {
        initKeyCodeToChar();
        isMapInitialized = true;
    }
}
If vKeyInit() is called after settings change, initLookupTables() runs again but isMapInitialized stays true. Inconsistent state.

Summary of Required Fixes
Priority	Issue	Risk
P0	AttachThreadInput without RAII guard	Focus loss
P0	ExitProcess(0) bypasses cleanup	Data corruption
P0	Process cache PID reuse	Wrong app settings
P1	Sleep(100) in hook reinstall	Input loss
P1	Deferred save race condition	Data loss
P1	SendMessageTimeout blocking hook	Latency
P2	Duplicated hotkey logic	Maintenance burden
P2	tempDisableKey reset	Wrong mode
P3	Hardcoded Qt/Electron list	Future breakage
