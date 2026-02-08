/*----------------------------------------------------------
NextKey - The Modern Vietnamese Input Method Engine.
Based on OpenKey architecture.

Copyright (C) 2026 NextKey Project
Author: Mai Tan Phat
License: GPL (Inherited from OpenKey)
-----------------------------------------------------------*/
#include "stdafx.h"
#include "QuickConvert.h"
#include "SystemTrayHelper.h"
#include "ToastPopup.h"
#include "RuntimeProfile.h"
#include "../../../engine/ConvertTool.h"
#include "../../../engine/Engine.h"

// Windows Clipboard History exclusion format (Win10 1809+)
static UINT CF_EXCLUDE_CLIPBOARD_HISTORY_LOCAL = RegisterClipboardFormat(_T("ExcludeClipboardContentFromMonitorProcessing"));

QuickConvertResult QuickConvert::convert() {
	// Convert clipboard text and return length for reselect
	// 
	// ARCHITECTURE NOTE: We intentionally ONLY convert CF_UNICODETEXT, NOT CF_HTML.
	// 
	// Reason: convertUtil() assumes plain text input. CF_HTML contains full HTML markup
	// (e.g., "<html><body><p class=MsoNormal>xin chào</p></body></html>") which breaks
	// the capitalization/case conversion logic - HTML tags get processed as text,
	// causing "shouldUpperCase" flag to reset before reaching actual content.
	// 
	// By not writing CF_HTML back, rich text apps (Word) will fallback to CF_UNICODETEXT,
	// ensuring correct text conversion at the cost of losing formatting (bold/italic).
	// This is an acceptable trade-off: Correctness > Formatting for a text utility.
	
	QuickConvertResult result = {0, false};

	if (!OpenClipboard(nullptr)) {
		return result;
	}

	std::wstring dataUnicode;

	// Read Unicode format ONLY - ignore CF_HTML (see note above)
	HANDLE clipboardHandle = GetClipboardData(CF_UNICODETEXT);
	if (clipboardHandle) {
		wchar_t* pUnicode = static_cast<wchar_t*>(GlobalLock(clipboardHandle));
		if (pUnicode) {
			dataUnicode = pUnicode;
			GlobalUnlock(clipboardHandle);
		}
	}

	// Convert using engine's convertUtil
	if (!dataUnicode.empty()) {
		dataUnicode = utf8ToWideString(convertUtil(wideStringToUtf8(dataUnicode)));
		result.utf16Length = (int)dataUnicode.length();  // UTF-16 code units!
	}

	// Write back CF_UNICODETEXT only (EmptyClipboard removes CF_HTML)
	EmptyClipboard();

	if (!dataUnicode.empty()) {
		HGLOBAL memHandle = GlobalAlloc(GMEM_MOVEABLE, (dataUnicode.size() + 1) * sizeof(wchar_t));
		if (memHandle) {
			memcpy(GlobalLock(memHandle), dataUnicode.c_str(), (dataUnicode.size() + 1) * sizeof(wchar_t));
			GlobalUnlock(memHandle);
			SetClipboardData(CF_UNICODETEXT, memHandle);
		}
	}

	// Best-effort: exclude from Win+V history (Win10 1809+, fails silently on older)
	SetClipboardData(CF_EXCLUDE_CLIPBOARD_HISTORY_LOCAL, NULL);

	CloseClipboard();
	result.success = true;
	return result;
}

std::wstring QuickConvert::readClipboardText() {
	std::wstring result;
	if (!OpenClipboard(nullptr)) {
		return result;
	}
	
	HANDLE clipboardHandle = GetClipboardData(CF_UNICODETEXT);
	if (clipboardHandle) {
		wchar_t* pText = static_cast<wchar_t*>(GlobalLock(clipboardHandle));
		if (pText) {
			result = pText;
			GlobalUnlock(clipboardHandle);
		}
	}
	
	CloseClipboard();
	return result;
}

void QuickConvert::writeClipboardText(const std::wstring& text) {
	if (text.empty()) return;
	if (!OpenClipboard(nullptr)) return;
	
	EmptyClipboard();
	
	HGLOBAL memHandle = GlobalAlloc(GMEM_MOVEABLE, (text.size() + 1) * sizeof(wchar_t));
	if (memHandle) {
		memcpy(GlobalLock(memHandle), text.c_str(), (text.size() + 1) * sizeof(wchar_t));
		GlobalUnlock(memHandle);
		SetClipboardData(CF_UNICODETEXT, memHandle);
	}
	
	// Best-effort: exclude from Win+V history (Win10 1809+)
	SetClipboardData(CF_EXCLUDE_CLIPBOARD_HISTORY_LOCAL, NULL);
	
	CloseClipboard();
}

void QuickConvert::simulateCopy() {
	HWND hwnd = GetForegroundWindow();
	
	// Check if app needs Ctrl+C instead of WM_COPY (e.g., Office apps)
	RuntimeProfile* profile = getProfileForHwnd(hwnd);
	bool useCtrlC = profile && profile->hasFlag(ProfileFlags::UseCtrlCCopy);
	
	if (useCtrlC) {
		// SendInput Ctrl+C for Office apps where WM_COPY doesn't work
		const ULONG_PTR OPENKEY_MAGIC = 0x4F4B;
		
		INPUT inputs[4] = {};
		
		inputs[0].type = INPUT_KEYBOARD;
		inputs[0].ki.wVk = VK_CONTROL;
		inputs[0].ki.dwExtraInfo = OPENKEY_MAGIC;
		
		inputs[1].type = INPUT_KEYBOARD;
		inputs[1].ki.wVk = 'C';
		inputs[1].ki.dwExtraInfo = OPENKEY_MAGIC;
		
		inputs[2].type = INPUT_KEYBOARD;
		inputs[2].ki.wVk = 'C';
		inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
		inputs[2].ki.dwExtraInfo = OPENKEY_MAGIC;
		
		inputs[3].type = INPUT_KEYBOARD;
		inputs[3].ki.wVk = VK_CONTROL;
		inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
		inputs[3].ki.dwExtraInfo = OPENKEY_MAGIC;
		
		SendInput(4, inputs, sizeof(INPUT));
	} else {
		// WM_COPY for standard apps (Notepad, etc.)
		HWND focusWnd = GetFocus();
		DWORD foregroundThread = GetWindowThreadProcessId(hwnd, NULL);
		DWORD currentThread = GetCurrentThreadId();
		
		if (foregroundThread != currentThread) {
			AttachThreadInput(currentThread, foregroundThread, TRUE);
			focusWnd = GetFocus();
			AttachThreadInput(currentThread, foregroundThread, FALSE);
		}
		
		if (focusWnd) {
			SendMessage(focusWnd, WM_COPY, 0, 0);
		} else {
			SendMessage(hwnd, WM_COPY, 0, 0);
		}
	}
}

bool QuickConvert::waitForCopy(int delayMs) {
	// Simple delay to wait for clipboard to update after Ctrl+C
	// Most apps update clipboard within 50ms
	Sleep(delayMs);
	return true;
}

bool QuickConvert::waitForModifiersRelease(int maxWaitMs) {
	// Wait for all modifier keys to be released
	// This is critical when called from a hotkey - user's Ctrl/Shift/Alt might still be down
	// Without this, simulateCopy() would conflict with user's held modifiers
	// Returns: true if released, false if timeout (user still holding keys)
	
	int waited = 0;
	const int checkInterval = 10;
	
	while (waited < maxWaitMs) {
		bool anyModifierDown = false;
		
		// Check all modifier keys
		if (GetAsyncKeyState(VK_CONTROL) & 0x8000) anyModifierDown = true;
		if (GetAsyncKeyState(VK_SHIFT) & 0x8000) anyModifierDown = true;
		if (GetAsyncKeyState(VK_MENU) & 0x8000) anyModifierDown = true;     // Alt
		if (GetAsyncKeyState(VK_LWIN) & 0x8000) anyModifierDown = true;
		if (GetAsyncKeyState(VK_RWIN) & 0x8000) anyModifierDown = true;
		
		if (!anyModifierDown) {
			return true;  // All modifiers released!
		}
		
		Sleep(checkInterval);
		waited += checkInterval;
	}
	
	return false;  // Timeout - user still holding keys
}

void QuickConvert::simulatePaste() {
	// Use SendInput Ctrl+V - universally compatible including Word, Notepad, browsers
	// WM_PASTE doesn't work in many modern apps (Word, RichEdit, etc.)
	
	// Magic number to identify our events - hook will skip these
	const ULONG_PTR OPENKEY_MAGIC = 0x4F4B;  // Same as OPENKEY_EXTRA_INFO in OpenKey.cpp
	
	INPUT inputs[4] = {};
	
	// Ctrl down
	inputs[0].type = INPUT_KEYBOARD;
	inputs[0].ki.wVk = VK_CONTROL;
	inputs[0].ki.dwExtraInfo = OPENKEY_MAGIC;
	
	// V down
	inputs[1].type = INPUT_KEYBOARD;
	inputs[1].ki.wVk = 'V';
	inputs[1].ki.dwExtraInfo = OPENKEY_MAGIC;
	
	// V up
	inputs[2].type = INPUT_KEYBOARD;
	inputs[2].ki.wVk = 'V';
	inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
	inputs[2].ki.dwExtraInfo = OPENKEY_MAGIC;
	
	// Ctrl up
	inputs[3].type = INPUT_KEYBOARD;
	inputs[3].ki.wVk = VK_CONTROL;
	inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
	inputs[3].ki.dwExtraInfo = OPENKEY_MAGIC;
	
	SendInput(4, inputs, sizeof(INPUT));
}

// ============================================================================
// ANCHOR-BASED RESELECT (Final Solution)
// - Anchor captured BEFORE copy for reliable positioning
// - Tier 1: EM_SETSEL for Edit controls (poll for paste completion)
// - Tier 2: SendInput for generic apps (best-effort)
// ============================================================================

// Helper: Get focused control with thread attachment
static HWND getFocusedControl(HWND foregroundWnd) {
	DWORD foregroundThread = GetWindowThreadProcessId(foregroundWnd, NULL);
	DWORD currentThread = GetCurrentThreadId();
	HWND focusWnd = NULL;
	
	if (foregroundThread != currentThread) {
		AttachThreadInput(currentThread, foregroundThread, TRUE);
		focusWnd = GetFocus();
		AttachThreadInput(currentThread, foregroundThread, FALSE);
	} else {
		focusWnd = GetFocus();
	}
	
	return focusWnd ? focusWnd : foregroundWnd;
}

SelectionAnchor QuickConvert::getSelectionAnchor(HWND hwnd) {
	SelectionAnchor anchor = {0, 0, false};
	if (!hwnd) return anchor;
	
	HWND focusWnd = getFocusedControl(hwnd);
	
	// Try EM_GETSEL - works for Edit/RichEdit controls
	LRESULT result = SendMessageTimeoutW(
		focusWnd, EM_GETSEL, 
		(WPARAM)&anchor.start, (LPARAM)&anchor.end,
		SMTO_ABORTIFHUNG | SMTO_NORMAL, 50, NULL
	);
	
	// Validate: message succeeded AND sane values AND there's a selection
	// PowerPoint and other non-Edit controls return garbage (e.g. start=-296743120)
	// Reject: negative values, unreasonably large values (>1M chars), start >= end
	const DWORD MAX_REASONABLE_POS = 1000000;  // 1M chars should be enough for any text
	bool isSane = (anchor.start >= 0) && 
	              (anchor.end >= 0) && 
	              (anchor.start < MAX_REASONABLE_POS) && 
	              (anchor.end <= MAX_REASONABLE_POS) &&
	              (anchor.start < anchor.end);
	
	anchor.valid = (result != 0) && isSane;
	return anchor;
}

// Tier 1: Edit control reselect using anchor
// Polls for selection collapse (paste committed), then EM_SETSEL using actual caret
static bool tryEditControlReselect(HWND hwnd, SelectionAnchor anchor, int pastedLength) {
	if (!anchor.valid || pastedLength <= 0) return false;
	
	HWND focusWnd = getFocusedControl(hwnd);
	
	// Poll: wait for selection to collapse (paste committed)
	// Max 160ms (8 × 20ms) - Word/RichEdit needs more time
	for (int i = 0; i < 8; i++) {
		Sleep(20);
		
		DWORD s = 0, e = 0;
		SendMessage(focusWnd, EM_GETSEL, (WPARAM)&s, (LPARAM)&e);
		
		if (s == e) {  // Selection collapsed → paste done!
			// Use actual caret position (e) which accounts for Unicode normalization
			// Caret after paste = anchor.start + actual pasted length in app
			if (e >= anchor.start) {
				SendMessage(focusWnd, EM_SETSEL, anchor.start, e);
				return true;
			}
			// Fallback: use anchor.start + pastedLength if caret seems wrong
			SendMessage(focusWnd, EM_SETSEL, anchor.start, anchor.start + pastedLength);
			return true;
		}
	}
	
	return false;  // Paste didn't settle in 160ms
}

// Tier 2: Keystroke reselect (best-effort for generic apps)
static void tryKeystrokeReselectOnce(int utf16Length) {
	if (utf16Length <= 0 || utf16Length > QUICK_CONVERT_RESELECT_CUTOFF) return;
	
	const ULONG_PTR OPENKEY_MAGIC = 0x4F4B;
	
	std::vector<INPUT> inputs;
	inputs.reserve(1 + utf16Length * 2 + 1);
	
	INPUT in = {};
	in.type = INPUT_KEYBOARD;
	in.ki.dwExtraInfo = OPENKEY_MAGIC;
	
	// Shift down
	in.ki.wVk = VK_SHIFT;
	in.ki.dwFlags = 0;
	inputs.push_back(in);
	
	// Left × N
	for (int i = 0; i < utf16Length; i++) {
		in.ki.wVk = VK_LEFT;
		in.ki.dwFlags = 0;
		inputs.push_back(in);
		
		in.ki.dwFlags = KEYEVENTF_KEYUP;
		inputs.push_back(in);
	}
	
	// Shift up
	in.ki.wVk = VK_SHIFT;
	in.ki.dwFlags = KEYEVENTF_KEYUP;
	inputs.push_back(in);
	
	SendInput((UINT)inputs.size(), inputs.data(), sizeof(INPUT));
}

bool QuickConvert::tryReselect(HWND hwnd, SelectionAnchor anchor, int pastedLength, int originalSelLength) {
	// Anchor-based reselect with tier-specific length handling:
	// - Tier 1 (EM_SETSEL): Use pastedLength (exact, what we actually pasted)
	// - Tier 2 (Keystroke): Use originalSelLength (workaround for Unicode normalization)
	//   Why? Some apps normalize NFD→NFC on paste, changing length. Using original
	//   selection length is a best-effort workaround since we can't detect this.
	
	if (pastedLength <= 0) return true;  // Nothing to select
	
	// Check if should skip EM_SETSEL (RichEdit controls crash with it)
	bool skipEmSetsel = false;
	RuntimeProfile* profile = getProfileForHwnd(hwnd);
	
	if (profile) {
		// Fast path: O(1) lookup from cache
		skipEmSetsel = profile->hasFlag(ProfileFlags::SkipEmSetsel);
	} else {
		// Fallback: detect on-the-fly (rare - profile usually exists)
		HWND focus = getFocusedControl(hwnd);
		TCHAR className[64] = {0};
		GetClassName(focus, className, 64);
		skipEmSetsel = (wcsstr(className, L"RichEdit") != nullptr ||
		                wcsstr(className, L"RICHEDIT") != nullptr ||
		                wcsstr(className, L"_WwG") != nullptr);
	}
	
	// TIER 1: Edit controls - use anchor + EM_SETSEL with exact pastedLength
	if (anchor.valid && !skipEmSetsel) {
		if (tryEditControlReselect(hwnd, anchor, pastedLength)) {
			return true;
		}
		// Tier 1 failed (paste didn't settle) → don't fall through
		return false;
	}
	
	// TIER 2: Generic apps - use originalSelLength as best-effort workaround
	int keystrokeLength = (originalSelLength > 0) ? originalSelLength : pastedLength;
	if (keystrokeLength <= QUICK_CONVERT_RESELECT_CUTOFF) {
		tryKeystrokeReselectOnce(keystrokeLength);
	}
	
	return true;  // Assume success for generic apps
}

void QuickConvert::showToast(LPCWSTR message) {
	// Use lightweight popup for instant feedback (no Shell_NotifyIcon delay)
	ToastPopup::show(message);
}

// === Smart Timing Detection Implementation ===

bool QuickConvert::waitForClipboardUnicode(int maxWaitMs, int checkIntervalMs) {
	// Wait for clipboard to have Unicode text available
	DWORD startTick = GetTickCount();
	
	while (GetTickCount() - startTick < (DWORD)maxWaitMs) {
		if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
			return true;  // Unicode text ready
		}
		Sleep(checkIntervalMs);
	}
	
	return false;  // Timeout
}

bool QuickConvert::isOfficeApp(HWND hwnd) {
	// Check if window belongs to Office suite
	if (!hwnd) return false;
	
	wchar_t className[256] = {};
	GetClassNameW(hwnd, className, 256);
	
	// Office apps have characteristic class names
	const wchar_t* officeClasses[] = {
		L"_WwG",      // Word
		L"EXCEL7",     // Excel  
		L"PPTFrameClass", // PowerPoint
		L"rctrl_renwnd32", // Outlook
		nullptr
	};
	
	for (const wchar_t** cls = officeClasses; *cls != nullptr; cls++) {
		if (wcscmp(className, *cls) == 0) {
			return true;
		}
	}
	
	// Fallback: check process name
	DWORD pid = 0;
	GetWindowThreadProcessId(hwnd, &pid);
	
	if (HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid)) {
		wchar_t processName[MAX_PATH] = {};
		DWORD size = MAX_PATH;
		
		if (QueryFullProcessImageNameW(hProcess, 0, processName, &size)) {
			// Check for office processes
			const wchar_t* officeProcesses[] = {
				L"WINWORD.EXE",
				L"EXCEL.EXE", 
				L"POWERPNT.EXE",
				L"OUTLOOK.EXE",
				nullptr
			};
			
			for (const wchar_t** proc = officeProcesses; *proc != nullptr; proc++) {
				if (wcsstr(processName, *proc) != nullptr) {
					CloseHandle(hProcess);
					return true;
				}
			}
		}
		CloseHandle(hProcess);
	}
	
	return false;
}

bool QuickConvert::waitForWindowFocus(HWND targetHwnd, int maxWaitMs, int checkIntervalMs) {
	// Wait for target window to regain focus after paste
	DWORD startTick = GetTickCount();
	
	while (GetTickCount() - startTick < (DWORD)maxWaitMs) {
		if (GetFocus() == targetHwnd || GetForegroundWindow() == targetHwnd) {
			return true;  // Window has focus
		}
		Sleep(checkIntervalMs);
	}
	
	return false;  // Timeout
}
