/*----------------------------------------------------------
OpenKey - The Cross platform Open source Vietnamese Keyboard application.

Copyright (C) 2019 Mai Vu Tuyen
Contact: maivutuyen.91@gmail.com
Github: https://github.com/tuyenvm/OpenKey
Fanpage: https://www.facebook.com/OpenKeyVN

This file is belong to the OpenKey project, Win32 version
which is released under GPL license.
You can fork, modify, improve this program. If you
redistribute your new version, it MUST be open source.

Portions Copyright (C) 2026 NextKey Project
Maintainer: Mai Tan Phat
-----------------------------------------------------------*/
#include "AppDelegate.h"
#include "SharedState.h"
#include "ConfigManager.h"
#include "QuickConvert.h"
#include "SequentialConvert.h"
#include "PerformanceLogger.h"  // For DEBUG_LOG
#include <thread>

// Helper function to forcefully bring window to foreground
// Uses Alt key simulation to bypass Windows focus stealing prevention
static void ForceForegroundWindow(HWND hWnd) {
	// First restore if minimized
	if (IsIconic(hWnd)) {
		ShowWindow(hWnd, SW_RESTORE);
	}
	
	// Make sure window is visible
	ShowWindow(hWnd, SW_SHOW);
	
	// AttachThreadInput pattern for reliable focus
	DWORD dwCurrentThread = GetCurrentThreadId();
	DWORD dwForegroundThread = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
	
	if (dwCurrentThread != dwForegroundThread) {
		AttachThreadInput(dwCurrentThread, dwForegroundThread, TRUE);
	}
	
	// Simulate Alt key press to trick Windows
	keybd_event(VK_MENU, 0, 0, 0);  // Alt down
	SetForegroundWindow(hWnd);
	keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);  // Alt up
	
	if (dwCurrentThread != dwForegroundThread) {
		AttachThreadInput(dwCurrentThread, dwForegroundThread, FALSE);
	}
	
	// Additional methods for reliability
	BringWindowToTop(hWnd);
	SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
}

static AppDelegate* _instance;

//see document in Engine.h
int vLanguage = 1;
int vInputType = 0;
int vFreeMark = 0;
int vCodeTable = 0;
int vCheckSpelling = 1;
int vUseModernOrthography = 1;
int vQuickTelex = 0;
#define DEFAULT_SWITCH_STATUS 0x5A00025A //default option + z
int vSwitchKeyStatus = DEFAULT_SWITCH_STATUS;
int vRestoreIfWrongSpelling = 1;
int vFixRecommendBrowser = 0;
int vUseMacro = 1;
int vUseMacroInEnglishMode = 1;
int vAutoCapsMacro = 0;
int vSendKeyStepByStep = 1;
int vUseSmartSwitchKey = 1;
int vUpperCaseFirstChar = 0;
int vTempOffSpelling = 0;
int vAllowConsonantZFWJ = 0;
int vQuickStartConsonant = 0;
int vQuickEndConsonant = 0;
int vOtherLanguage = 1;
int vRememberCode = 1;
int vTempOffOpenKey = 0;
int vTempOffMacro = 0;  // ESC key to skip macro expansion for next word

int vUseGrayIcon = 0;
int vShowOnStartUp = 0;
int vRunWithWindows = 0;  // Default OFF - user should enable manually

int vSupportMetroApp = 1;
int vCreateDesktopShortcut = 0;
int vRunAsAdmin = 0;
int vCheckNewVersion = 0;
//beta feature
int vFixChromiumBrowser = 0; //new on version 2.0
int vExcludeApps = 0; //enable/disable exclude apps feature (default OFF, only Smart Switch is ON)
int vShowAdvancedSettings = 0; //remember advanced settings panel state
int vBackgroundOpacity = 80; //UI background opacity (0-100)

// Tray icon customization
COLORREF vTrayIconColorV = 0;  // 0 = use default (red #F36267 = RGB(243, 98, 103))
COLORREF vTrayIconColorE = 0;  // 0 = use default (blue #2FAFDA = RGB(47, 175, 218))
wchar_t vTrayIconFontName[LF_FACESIZE] = L"Arial Rounded MT Bold";
int vEnablePerfLog = 0;  // Performance logging disabled by default
int vReduceMemory = 0;   // EmptyWorkingSet RAM reduction for Settings dialog (disabled by default)
int vQuickConvertAutoPaste = 0;  // OFF = clipboard only, ON = auto-paste + reselect

bool AppDelegate::isDialogMsg(MSG & msg) const {
	return (mainDialog != NULL && IsDialogMessage(mainDialog->getHwnd(), &msg)) ||
		(macroDialog != NULL && IsDialogMessage(macroDialog->getHwnd(), &msg)) || 
		(convertDialog != NULL && IsDialogMessage(convertDialog->getHwnd(), &msg)) || 
		(excludedAppsDialog != NULL && IsDialogMessage(excludedAppsDialog->getHwnd(), &msg));
		// AboutDialog is now a Sciter window, not a Win32 dialog, so it doesn't need IsDialogMessage
}

#define ABOUT_WINDOW_TITLE L"V\u1EC1 NextKey"

void AppDelegate::onOpenKeyAbout() {
	// Anti-spam: Check if About window already exists
	HWND existingAbout = FindWindowW(NULL, ABOUT_WINDOW_TITLE);
	if (existingAbout) {
		// Use IPC to let subprocess bring itself to foreground
		PostMessage(existingAbout, WM_USER + 107, 0, 0);
		return;
	}
	
	// Spawn subprocess (Fire and Forget)
	WCHAR exePath[MAX_PATH];
	GetModuleFileNameW(NULL, exePath, MAX_PATH);
	
	STARTUPINFOW si = { sizeof(si) };
	PROCESS_INFORMATION pi;
	
	wchar_t cmdLine[MAX_PATH + 20];
	swprintf_s(cmdLine, L"\"%s\" --about", exePath);
	
	if (CreateProcessW(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
		CloseHandle(pi.hThread);
		trackChildProcess(pi.hProcess);  // Store handle for cleanup
	}
}

// TaskDialog callback to handle hyperlink clicks
HRESULT CALLBACK TaskDialogCallback(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LONG_PTR lpRefData) {
	if (msg == TDN_HYPERLINK_CLICKED) {
		// lParam contains the URL as a wide string
		ShellExecute(NULL, L"open", (LPCWSTR)lParam, NULL, NULL, SW_SHOWNORMAL);
	}
	return S_OK;
}

void AppDelegate::checkUpdate(bool showNoUpdateMessage) {
	// Prevent multiple simultaneous update checks
	static bool isChecking = false;
	if (isChecking) return;
	isChecking = true;
	
	// Get foreground window as parent so dialog appears on top
	HWND parentWnd = GetForegroundWindow();
	
	string newVersion;
	if (OpenKeyManager::checkUpdate(newVersion)) {
		std::wstring versionW = utf8ToWideString(newVersion);
		
		// Build content with clickable hyperlink
		// Note: TaskDialog doesn't support HTML tags like <b>, only hyperlinks with <a>
		WCHAR content[512];
		wsprintf(content, 
			TEXT("Có phiên bản mới %s !\n\n")
			TEXT("<a href=\"https://github.com/phatMT97/NextKey/releases/tag/%s\">Xem Changelogs</a>"),
			versionW.c_str(), versionW.c_str());
		
		// Custom buttons
		TASKDIALOG_BUTTON buttons[] = {
			{ 1001, L"Cập nhật ngay" },
			{ 1002, L"Bỏ qua" }
		};
		
		TASKDIALOGCONFIG config = {0};
		config.cbSize = sizeof(config);
		config.hwndParent = parentWnd;  // Use foreground window as parent
		config.dwFlags = TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS;
		config.pszWindowTitle = L"NextKey Update";
		config.pszMainIcon = TD_INFORMATION_ICON;
		config.pszMainInstruction = L"Đã có bản cập nhật mới!";
		config.pszContent = content;
		config.cButtons = 2;
		config.pButtons = buttons;
		config.nDefaultButton = 1001;
		config.pfCallback = TaskDialogCallback;
		
		int buttonPressed = 0;
		HRESULT hr = TaskDialogIndirect(&config, &buttonPressed, NULL, NULL);
		
		if (SUCCEEDED(hr) && buttonPressed == 1001) {
			// Update now - MUST set working directory for NextKeyUpdate.exe
			// Otherwise it inherits wrong cwd and file operations fail
			WCHAR exeDir[MAX_PATH];
			WCHAR exePath[MAX_PATH];
			GetCurrentDirectory(MAX_PATH, exeDir);
			wsprintf(exePath, TEXT("%s\\NextKeyUpdate.exe"), exeDir);
			ShellExecute(0, L"open", exePath, 0, exeDir, SW_SHOWNORMAL);
			AppDelegate::getInstance()->onOpenKeyExit();
		}
		// buttonPressed == 1002 or dialog closed = Skip
	} else if (showNoUpdateMessage) {
		MessageBox(
			parentWnd,  // Use foreground window as parent
			_T("Bạn đang sử dụng phiên bản mới nhất!"),
			_T("NextKey Update"),
			MB_ICONINFORMATION | MB_OK
		);
	}
	
	isChecking = false;
}

AppDelegate::AppDelegate() {
	_instance = this;
}

AppDelegate * AppDelegate::getInstance() {
	return _instance;
}

int AppDelegate::run(HINSTANCE hInstance) {
	this->hInstance = hInstance;

	//check app has already run or not
	HWND previousInstance = FindWindow(APP_CLASS, NULL);
	if (previousInstance) {
		MessageBeep(MB_OK);
		SendMessage(previousInstance, WM_USER + 2019, 0, 0);
		PostQuitMessage(0);
		return 0;
	}

	// ConfigManager now reads directly from TuyenMai\OpenKey registry if config.toml doesn't exist

	//init OpenKey Engine
	OpenKeyManager::initEngine();

	// Initialize SharedState (main process = owner)
	if (!SharedState::instance().init(true)) {
		LOG(L"[AppDelegate] SharedState init failed\n");
	}
	// Sync initial language state to shared memory
	SharedState::instance().setLanguage(vLanguage);
	SharedState::instance().setInputType(vInputType);
	SharedState::instance().setCodeTable(vCodeTable);
	SharedState::instance().setCheckSpelling(vCheckSpelling);
	SharedState::instance().setSmartSwitch(vUseSmartSwitchKey);
	SharedState::instance().setUseMacro(vUseMacro);

	//create system tray
	SystemTrayHelper::createSystemTrayIcon(hInstance);
	SystemTrayHelper::updateData();

	//create main control
	if (vShowOnStartUp)
		createMainDialog();
	MessageBeep(MB_OK);

	//check update (run on background thread to avoid blocking UI)
	if (vCheckNewVersion) {
		std::thread([this]() {
			checkUpdate();
		}).detach();
	}

	MSG msg;
	// Main message loop:
	while (GetMessage(&msg, nullptr, 0, 0))	{
		if (msg.message == WM_KEYDOWN) {
			OpenKeyManager::_lastKeyCode = (UINT16)msg.wParam;
		}
		if (!isDialogMsg(msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return 0;
}

#define SETTINGS_WINDOW_TITLE L"NextKey Settings"

void AppDelegate::createMainDialog() {
	// Anti-spam: Check if Settings window already exists
	HWND existingSettings = FindWindowW(NULL, SETTINGS_WINDOW_TITLE);
	if (existingSettings) {
		// Send message to subprocess to bring itself to foreground
		// Cross-process SetForegroundWindow usually fails, so let subprocess do it
		PostMessage(existingSettings, WM_USER + 107, 0, 0);
		return;
	}
	
	// Spawn settings subprocess (Fire and Forget)
	WCHAR exePath[MAX_PATH];
	GetModuleFileNameW(NULL, exePath, MAX_PATH);
	
	STARTUPINFOW si = { sizeof(si) };
	PROCESS_INFORMATION pi;
	
	wchar_t cmdLine[MAX_PATH + 20];
	swprintf_s(cmdLine, L"\"%s\" --settings", exePath);
	
	if (CreateProcessW(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
		CloseHandle(pi.hThread);
		trackChildProcess(pi.hProcess);  // Store handle for cleanup
	}
}

void AppDelegate::closeDialog(BaseDialog * dialog) {
	dialog->closeDialog();
	if (mainDialog == dialog) {
		delete mainDialog;
		mainDialog = NULL;
	} else if (macroDialog == dialog) {
		delete macroDialog;
		macroDialog = NULL;
	} 	else if (convertDialog == dialog) {
		delete convertDialog;
		convertDialog = NULL;
	} else if (excludedAppsDialog == dialog) {
		delete excludedAppsDialog;
		excludedAppsDialog = NULL;
	}
	// AboutDialog is handled separately via closeAboutDialog()
}

void AppDelegate::closeAboutDialog() {
	// Note: Sciter windows cannot be simply deleted due to internal reference counting
	// The window is hidden and reused on next open
}

void AppDelegate::onInputMethodChangedFromHotKey() {
	APP_SET_DATA(vLanguage, vLanguage);
	if (mainDialog) {
		mainDialog->fillData();
	}
	SystemTrayHelper::updateData();
	
	// Sync to SharedState for subprocess polling
	SharedState::instance().setLanguage(vLanguage);
	
	// Notify UI subprocesses (Settings dialog) of language change
	NotifyUILanguageChange(vLanguage != 0);  // true = Vietnamese
}

void AppDelegate::onDefaultConfig() {
	APP_SET_DATA(vLanguage, 1);
	APP_SET_DATA(vInputType, 0);
	vFreeMark = 0;
	APP_SET_DATA(vCodeTable, 0);
	APP_SET_DATA(vCheckSpelling, 1);
	APP_SET_DATA(vUseModernOrthography, 0);
	APP_SET_DATA(vQuickTelex, 0);
	APP_SET_DATA(vSwitchKeyStatus, DEFAULT_SWITCH_STATUS);
	APP_SET_DATA(vRestoreIfWrongSpelling, 1);
	APP_SET_DATA(vFixRecommendBrowser, 1);
	APP_SET_DATA(vUseMacro, 0);
	APP_SET_DATA(vUseMacroInEnglishMode, 0);
	APP_SET_DATA(vSendKeyStepByStep, 1);
	APP_SET_DATA(vUseSmartSwitchKey, 1);
	APP_SET_DATA(vUpperCaseFirstChar, 0);
	APP_SET_DATA(vAllowConsonantZFWJ, 0);
	APP_SET_DATA(vTempOffSpelling, 0);

	APP_SET_DATA(vUseGrayIcon, 0);
	APP_SET_DATA(vShowOnStartUp, 1);
	APP_SET_DATA(vRunWithWindows, 1);

	APP_SET_DATA(vSupportMetroApp, 1);
	APP_SET_DATA(vRememberCode, 1);
	APP_SET_DATA(vOtherLanguage, 1);
	APP_SET_DATA(vTempOffOpenKey, 0);
	APP_SET_DATA(vFixChromiumBrowser, 0);

	if (mainDialog) {
		mainDialog->fillData();
	}
	SystemTrayHelper::updateData();
}

void AppDelegate::onToggleVietnamese() {
	APP_SET_DATA(vLanguage, vLanguage ? 0 : 1);
	if (mainDialog) {
		mainDialog->fillData();
	}
	
	// Sync to SharedState
	SharedState::instance().setLanguage(vLanguage);
	
	if (vUseSmartSwitchKey) {
		string& exe = OpenKeyHelper::getLastAppExecuteName();
		setAppInputMethodStatus(exe, vLanguage | (vCodeTable << 1));
		saveSmartSwitchKeyData();
	}

	SystemTrayHelper::updateData();
}

void AppDelegate::onToggleCheckSpelling() {
	APP_SET_DATA(vCheckSpelling, vCheckSpelling ? 0 : 1);
	if (mainDialog) {
		mainDialog->fillData();
	}
	vSetCheckSpelling();
	
	// Sync to SharedState for realtime UI update
	SharedState::instance().setCheckSpelling(vCheckSpelling);
	SharedState::instance().signalConfigChanged();
	SystemTrayHelper::updateData();
}

void AppDelegate::onToggleUseSmartSwitchKey() {
	APP_SET_DATA(vUseSmartSwitchKey, vUseSmartSwitchKey ? 0 : 1);
	if (mainDialog) {
		mainDialog->fillData();
	}
	SharedState::instance().setSmartSwitch(vUseSmartSwitchKey);
	SharedState::instance().signalConfigChanged();
}

void AppDelegate::onToggleUseMacro() {
	APP_SET_DATA(vUseMacro, vUseMacro ? 0 : 1);
	if (mainDialog) {
		mainDialog->fillData();
	}
	SharedState::instance().setUseMacro(vUseMacro);
	SharedState::instance().signalConfigChanged();
}

// "Bảng gõ tắt" in Unicode escape sequences
#define MACRO_WINDOW_TITLE L"B\u1EA3ng g\u00F5 t\u1EAFt"

void AppDelegate::onMacroTable() {
	// Anti-spam: Check if Macro window already exists
	HWND existingMacro = FindWindowW(NULL, MACRO_WINDOW_TITLE);
	if (existingMacro) {
		// Use IPC to let subprocess bring itself to foreground
		PostMessage(existingMacro, WM_USER + 107, 0, 0);
		return;
	}
	
	// Spawn macro subprocess (Fire and Forget)
	WCHAR exePath[MAX_PATH];
	GetModuleFileNameW(NULL, exePath, MAX_PATH);
	
	STARTUPINFOW si = { sizeof(si) };
	PROCESS_INFORMATION pi;
	
	wchar_t cmdLine[MAX_PATH + 20];
	swprintf_s(cmdLine, L"\"%s\" --macro", exePath);
	
	if (CreateProcessW(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
		CloseHandle(pi.hThread);
		trackChildProcess(pi.hProcess);  // Store handle for cleanup
	}
}

void AppDelegate::onConvertTool() {
	// Use Sciter subprocess instead of Win32 dialog
	onSpawnConvertToolSciter();
}

void AppDelegate::onQuickConvert() {
	// Flag lock to prevent retrigger while processing
	static bool isProcessing = false;
	if (isProcessing) {
		return;  // Already processing
	}
	isProcessing = true;
	
	// Capture state before spawning thread
	HWND targetHwnd = GetForegroundWindow();
	bool autoPaste = vQuickConvertAutoPaste != 0;
	bool showAlert = convertToolDontAlertWhenCompleted == 0;
	bool useSequential = SequentialConvert::instance().isEnabled();
	
	// CRITICAL: Capture anchor IMMEDIATELY while selection is still intact
	// Must be before any delays (waitForModifiersRelease, Sleep, etc.)
	auto anchor = QuickConvert::getSelectionAnchor(targetHwnd);
	
	DEBUG_LOG_FMT("QC_START", "hwnd=%p, autoPaste=%d, sequential=%d, anchor.valid=%d, anchor.start=%d, anchor.end=%d",
		(void*)targetHwnd, autoPaste, useSequential, anchor.valid, (int)anchor.start, (int)anchor.end);
	
	// Run in separate thread to avoid blocking keyboard hook
	std::thread([targetHwnd, autoPaste, showAlert, useSequential, anchor]() {
		// Wait for user to release hotkey modifiers (Ctrl+Shift+X etc.)
		bool modifiersReleased = QuickConvert::waitForModifiersRelease(500);
		
		// Small delay to ensure keyboard hook has fully returned
		Sleep(30);
		
		// ============================================================
		// STEP 1: COPY (always - same for sequential and normal)
		// ============================================================
		QuickConvert::simulateCopy();
		QuickConvert::waitForCopy(100);
		
		// ============================================================
		// STEP 2: CONVERT
		// ============================================================
		QuickConvertResult result = { false, 0 };
		std::wstring sequentialStepName;
		
		if (useSequential) {
			// Sequential mode: convert with single option, cycle through
			auto& seq = SequentialConvert::instance();
			
			// Read clipboard content using helper
		std::wstring clipboardText = QuickConvert::readClipboardText();
		
		DEBUG_LOG_FMT("QC_COPY", "clipboard.len=%d, text='%.30ls...'",
			(int)clipboardText.length(), clipboardText.empty() ? L"(empty)" : clipboardText.c_str());
		
		if (clipboardText.empty()) {
			DEBUG_LOG("QC_COPY", "FAILED - clipboard empty, aborting");
			isProcessing = false;
			return;
		}
			
			bool isNewSelection = false;
			const char* detectionReason = "unknown";
			if (!seq.isActive()) {
				isNewSelection = true;
				detectionReason = "not_active";
			} else if (seq.hasTimedOut()) {
				isNewSelection = true;
				detectionReason = "timeout";
			} else if (seq.isWindowChanged(targetHwnd)) {
				isNewSelection = true;
				detectionReason = "window_changed";
			} else if (anchor.valid) {
				// Anchor available: compare position
				isNewSelection = seq.isNewSelection(anchor);
				detectionReason = isNewSelection ? "anchor_pos_changed" : "anchor_same";
			} else {
				// Office apps: compare clipboard content
				isNewSelection = seq.isNewSelectionByContent(clipboardText);
				detectionReason = isNewSelection ? "content_changed" : "content_same";
			}
			
			DEBUG_LOG_FMT("QC_DETECT", "isNewSelection=%d, reason=%s, seq.currentIdx=%d",
				isNewSelection, detectionReason, seq.isActive() ? 1 : 0);
			
			if (isNewSelection) {
				// New selection: reset and start fresh
				seq.reset();
				seq.setOrigin(clipboardText, anchor, targetHwnd);
			}
			// Else: same position, just advance to next option
			
			// Apply current option and advance
			std::wstring convertedText = seq.applyCurrentAndAdvance();
			sequentialStepName = seq.getCurrentStepName();
			
			DEBUG_LOG_FMT("QC_APPLY", "stepName='%ls', converted.len=%d",
				sequentialStepName.c_str(), (int)convertedText.length());
			
			if (convertedText.empty()) {
				DEBUG_LOG("QC_APPLY", "FAILED - empty result, aborting");
				isProcessing = false;
				return;
			}
			
			// Write converted text to clipboard using helper
			QuickConvert::writeClipboardText(convertedText);
			
			result.success = true;
			result.utf16Length = (int)convertedText.length();
			
		} else {
			// Normal mode: convert with all options at once
			result = QuickConvert::convert();
			
			if (!result.success) {
				isProcessing = false;
				return;
			}
		}
		
		// ============================================================
		// STEP 3: PASTE (same pattern for both modes)
		// ============================================================
		
		if (!autoPaste) {
			if (showAlert) {
				QuickConvert::showToast(L"Đã chuyển mã (Ctrl+V để dán)");
			}
			isProcessing = false;
			return;
		}
		
		// Paste - simple, no complex pre-selection
		Sleep(50);
		
		QuickConvert::simulatePaste();
		
		// Wait for paste to commit - Office apps need more time
		Sleep(100);
		
		// ============================================================
		// STEP 4: RESELECT (optional, best-effort)
		// ============================================================
		
		if (GetForegroundWindow() != targetHwnd) {
			if (useSequential && !sequentialStepName.empty()) {
				QuickConvert::showToast((L"\u2192 " + sequentialStepName).c_str());
			} else {
				QuickConvert::showToast(L"Đã chuyển mã");
			}
			isProcessing = false;
			return;
		}
		
		bool reselectOk = false;
		if (result.utf16Length <= QUICK_CONVERT_RESELECT_CUTOFF) {
			// Use original selection length for keystroke fallback
			// BUT: if anchor is invalid (Office apps), use pasted length instead
			int originalSelectionLength = anchor.valid ? (anchor.end - anchor.start) : result.utf16Length;
			reselectOk = QuickConvert::tryReselect(targetHwnd, anchor, result.utf16Length, originalSelectionLength);
			
			DEBUG_LOG_FMT("QC_RESELECT", "success=%d, utf16Len=%d, origSelLen=%d",
				reselectOk, result.utf16Length, originalSelectionLength);
		}
		
		// Toast
		if (useSequential && !sequentialStepName.empty()) {
			QuickConvert::showToast((L"\u2192 " + sequentialStepName).c_str());
		} else if (reselectOk) {
			QuickConvert::showToast(L"Đã chuyển mã");
		} else if (!modifiersReleased) {
			QuickConvert::showToast(L"Đã chuyển (thả phím tắt để bôi đen)");
		} else {
			QuickConvert::showToast(L"Đã chuyển mã");
		}
		
		isProcessing = false;
	}).detach();
}

// NOTE: onManageExcludedApps() replaced by onSpawnExcludedAppsSciter()

// "Ứng dụng loại trừ" in Unicode escape sequences
#define EXCLUDED_APPS_WINDOW_TITLE L"\u1EE8ng d\u1EE5ng lo\u1EA1i tr\u1EEB"

void AppDelegate::onSpawnExcludedAppsSciter() {
	// Anti-spam: Check if Excluded Apps window already exists
	HWND existingWindow = FindWindowW(NULL, EXCLUDED_APPS_WINDOW_TITLE);
	if (existingWindow) {
		// Use IPC to let subprocess bring itself to foreground
		PostMessage(existingWindow, WM_USER + 107, 0, 0);
		return;
	}
	
	// Spawn excluded apps subprocess (Fire and Forget)
	WCHAR exePath[MAX_PATH];
	GetModuleFileNameW(NULL, exePath, MAX_PATH);
	
	STARTUPINFOW si = { sizeof(si) };
	PROCESS_INFORMATION pi;
	
	wchar_t cmdLine[MAX_PATH + 30];
	swprintf_s(cmdLine, L"\"%s\" --excludedapps", exePath);
	
	if (CreateProcessW(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
		CloseHandle(pi.hThread);
		trackChildProcess(pi.hProcess);  // Store handle for cleanup
	}
}

void AppDelegate::onCheckUpdate() {
	// Run on background thread to avoid blocking UI (network request)
	std::thread([this]() {
		checkUpdate(true);  // Show message even if no update available
	}).detach();
}

#define CONVERT_TOOL_WINDOW_TITLE L"C\u00F4ng c\u1EE5 chuy\u1EC3n m\u00E3"

void AppDelegate::onSpawnConvertToolSciter() {
	// Anti-spam: Check if Convert Tool window already exists
	HWND existingWindow = FindWindowW(NULL, CONVERT_TOOL_WINDOW_TITLE);
	if (existingWindow) {
		// Send IPC message to subprocess to bring itself to foreground
		PostMessage(existingWindow, WM_USER + 107, 0, 0);
		return;
	}
	
	// Spawn convert tool subprocess
	WCHAR exePath[MAX_PATH];
	GetModuleFileNameW(NULL, exePath, MAX_PATH);
	
	STARTUPINFOW si = { sizeof(si) };
	PROCESS_INFORMATION pi;
	
	wchar_t cmdLine[MAX_PATH + 30];
	swprintf_s(cmdLine, L"\"%s\" --convert-tool", exePath);
	
	if (CreateProcessW(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
		CloseHandle(pi.hThread);
		trackChildProcess(pi.hProcess);
	}
}

// "Cấu hình ứng dụng" = "C\u1EA5u h\u00ECnh \u1EE9ng d\u1EE5ng"
#define APP_OVERRIDES_WINDOW_TITLE L"C\u1EA5u h\u00ECnh \u1EE9ng d\u1EE5ng"

void AppDelegate::onSpawnAppOverrides() {
	// Anti-spam: Check if App Overrides window already exists
	HWND existingWindow = FindWindowW(NULL, APP_OVERRIDES_WINDOW_TITLE);
	if (existingWindow) {
		// Use IPC to let subprocess bring itself to foreground
		PostMessage(existingWindow, WM_USER + 107, 0, 0);
		return;
	}
	
	// Spawn app overrides subprocess
	WCHAR exePath[MAX_PATH];
	GetModuleFileNameW(NULL, exePath, MAX_PATH);
	
	STARTUPINFOW si = { sizeof(si) };
	PROCESS_INFORMATION pi;
	
	wchar_t cmdLine[MAX_PATH + 30];
	swprintf_s(cmdLine, L"\"%s\" --appoverrides", exePath);
	
	if (CreateProcessW(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
		CloseHandle(pi.hThread);
		trackChildProcess(pi.hProcess);
	}
}

void AppDelegate::onInputType(const int & type) {
	APP_SET_DATA(vInputType, type);
	if (mainDialog) {
		mainDialog->fillData();
	}
	// Sync to SharedState for realtime UI update
	SharedState::instance().setInputType(vInputType);
	SharedState::instance().signalConfigChanged();
	SystemTrayHelper::updateData();
}

void AppDelegate::onTableCode(const int & code) {
	APP_SET_DATA(vCodeTable, code);
	if (mainDialog) {
		mainDialog->fillData();
	}
	if (vRememberCode) {
		setAppInputMethodStatus(OpenKeyHelper::getFrontMostAppExecuteName(), vLanguage | (vCodeTable << 1));
		saveSmartSwitchKeyData();
	}
	// Sync to SharedState for realtime UI update
	SharedState::instance().setCodeTable(vCodeTable);
	SharedState::instance().signalConfigChanged();
	SystemTrayHelper::updateData();
}

void AppDelegate::onControlPanel() {
	createMainDialog();
}

void AppDelegate::onOpenKeyExit() {
	// Terminate all subprocess dialogs at once - no FindWindow needed!
	terminateAllChildren();
	
	// Shutdown SharedState (cleanup shared memory)
	SharedState::instance().shutdown();
	
	// Save config to disk before exit (in case destructor doesn't run)
	ConfigManager::instance().saveIfDirty();
	
	OpenKeyManager::freeEngine();
	SystemTrayHelper::removeSystemTray();
	PostQuitMessage(0);
}

void AppDelegate::trackChildProcess(HANDLE hProcess) {
	if (hProcess) {
		m_childProcesses.push_back(hProcess);
	}
}

void AppDelegate::terminateAllChildren() {
	for (HANDLE h : m_childProcesses) {
		if (h) {
			TerminateProcess(h, 0);
			CloseHandle(h);
		}
	}
	m_childProcesses.clear();
}
