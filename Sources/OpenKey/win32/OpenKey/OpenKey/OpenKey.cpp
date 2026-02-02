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
#include "stdafx.h"
#include "AppDelegate.h"
#include "SettingsDialog.h"
#include "PerformanceLogger.h"
#include "ConfigManager.h"
#include <mutex>
#include <set>
#include <map>
#include <sstream>
#include <algorithm>
#include "RuntimeProfile.h"
#include "../../../engine/DebugSnapshot.h"

// Tripwire logging helper - called from DebugSnapshot.cpp
void logTripwire(const char* msg) {
	if (PerformanceLogger::isEnabled()) {
		PerformanceLogger::log(msg, 0.0);
	}
	OutputDebugStringA(msg);
	OutputDebugStringA("\n");
}

#pragma comment(lib, "imm32")
#define IMC_GETOPENSTATUS 0x0005

#define MASK_SHIFT				0x01
#define MASK_CONTROL			0x02
#define MASK_ALT				0x04
#define MASK_CAPITAL			0x08
#define MASK_NUMLOCK			0x10
#define MASK_WIN				0x20
#define MASK_SCROLL				0x40

#define OTHER_CONTROL_KEY (_flag & MASK_ALT) || (_flag & MASK_CONTROL)
#define DYNA_DATA(macro, pos) (macro ? pData->macroData[pos] : pData->charData[pos])
#define EMPTY_HOTKEY 0xFE0000FE

static vector<string> _chromiumBrowser = {
	"chrome.exe", "brave.exe", "msedge.exe"
};

// Qt and Electron apps that don't need empty char fix (causes lag)
// NOTE: Non-static to allow extern access from AppOverridesDialogSciter
// Use lowercase - matching is case-insensitive via toLower()
vector<string> _qtElectronApps = {
	"notepadnext.exe",    // NotepadNext (Qt)
	"code.exe",           // VSCode (Electron)
	"sublime_text.exe",   // Sublime Text
	"atom.exe",           // Atom (Electron)
	"discord.exe",        // Discord (Electron)
	"slack.exe"           // Slack (Electron)
};

// MS Office apps that falsely report IME as ON - skip IME check for these
// PowerPoint reports isImeON=1 even when no IME is active, blocking Vietnamese input
// NOTE: Non-static to allow extern access from AppOverridesDialogSciter
// Use lowercase - matching is case-insensitive via toLower()
vector<string> _skipImeCheckApps = {
	"powerpnt.exe",   // Microsoft PowerPoint
	"winword.exe",    // Microsoft Word
	"excel.exe"       // Microsoft Excel
};

// Default lists for reload (these never change)
static vector<string> _defaultQtElectronApps = {
	"notepadnext.exe", "code.exe", "sublime_text.exe", "atom.exe", "discord.exe", "slack.exe"
};
static vector<string> _defaultSkipImeCheckApps = {
	"powerpnt.exe", "winword.exe", "excel.exe"
};

// ConfigManager keys for special apps
static const char* CFG_SECTION_SPECIAL_APPS = "specialApps";
static const char* CFG_QT_ELECTRON_APPS = "qtElectronApps";
static const char* CFG_SKIP_IME_APPS = "skipImeCheckApps";
static const char* CFG_DELETED_DEFAULTS = "deletedDefaults";

// Helper to convert wide string to UTF-8
static string wideToUtf8(const wstring& wide) {
	if (wide.empty()) return "";
	int size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
	string result(size - 1, 0);
	WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, &result[0], size, nullptr, nullptr);
	return result;
}

// Helper for lowercase
static string strToLower(const string& s) {
	string result = s;
	std::transform(result.begin(), result.end(), result.begin(),
		[](unsigned char c) { return std::tolower(c); });
	return result;
}

// ============================================================
// Clipboard Apps - Per-app injection method configuration
// ============================================================
// Method: 0 = ShiftInsert (default), 1 = CtrlV, 2 = SendInputKey (with 12ms smart batch delay)
struct ClipboardAppConfig {
	int method = 0;
	int delayMs = 0;
};
static std::map<std::string, ClipboardAppConfig> _clipboardApps;
static const int SMART_BATCH_DELAY_MS = 12;  // 1 frame at 60fps = 16.6ms, this covers 60-144fps

// Reload clipboard apps from ConfigManager
void reloadClipboardApps() {
	_clipboardApps.clear();
	
	auto& config = ConfigManager::instance();
	auto apps = config.getClipboardApps();
	for (const auto& app : apps) {
		ClipboardAppConfig cfg;
		cfg.method = app.method;
		cfg.delayMs = app.delayMs;
		_clipboardApps[strToLower(app.exeName)] = cfg;
	}
}

// Get clipboard method for current app (only used when clipboard mode is enabled)
// Returns: -1 = use global default, 0 = ShiftInsert, 1 = CtrlV, 2 = SendInputKey
static int getClipboardMethodForCurrentApp(int& outDelayMs) {
	std::string appName = strToLower(OpenKeyHelper::getLastAppExecuteName());
	auto it = _clipboardApps.find(appName);
	if (it != _clipboardApps.end()) {
		outDelayMs = it->second.delayMs;
		return it->second.method;
	}
	outDelayMs = 0;
	return -1;  // Use global default (ShiftInsert)
}

// Reload special apps lists from ConfigManager + defaults (called when settings change)
void reloadSpecialAppsLists() {
	OutputDebugStringA("[Main] reloadSpecialAppsLists called\n");
	
	auto& config = ConfigManager::instance();
	
	// Load deleted defaults from config
	set<string> deletedDefaults;
	auto deletedList = config.getStringArray(CFG_SECTION_SPECIAL_APPS, CFG_DELETED_DEFAULTS);
	for (const auto& app : deletedList) {
		deletedDefaults.insert(strToLower(app));
	}
	
	// Rebuild Qt/Electron list - NORMALIZE TO LOWERCASE on load
	_qtElectronApps.clear();
	for (const auto& app : _defaultQtElectronApps) {
		string lowerApp = strToLower(app);
		if (deletedDefaults.find(lowerApp) == deletedDefaults.end()) {
			_qtElectronApps.push_back(lowerApp);  // Store lowercase
		}
	}
	// Add user apps from config (also normalized)
	auto userQtApps = config.getStringArray(CFG_SECTION_SPECIAL_APPS, CFG_QT_ELECTRON_APPS);
	for (const auto& app : userQtApps) {
		_qtElectronApps.push_back(strToLower(app));
	}
	
	// Rebuild Skip IME list - NORMALIZE TO LOWERCASE on load
	_skipImeCheckApps.clear();
	for (const auto& app : _defaultSkipImeCheckApps) {
		string lowerApp = strToLower(app);
		if (deletedDefaults.find(lowerApp) == deletedDefaults.end()) {
			_skipImeCheckApps.push_back(lowerApp);  // Store lowercase
		}
	}
	// Add user apps from config (also normalized)
	auto userImeApps = config.getStringArray(CFG_SECTION_SPECIAL_APPS, CFG_SKIP_IME_APPS);
	for (const auto& app : userImeApps) {
		_skipImeCheckApps.push_back(strToLower(app));
	}
	
	OutputDebugStringA(("[Main] Qt apps: " + to_string(_qtElectronApps.size()) + ", IME apps: " + to_string(_skipImeCheckApps.size()) + "\n").c_str());
}

// Check if current app should skip IME check
// OPTIMIZED: List is pre-lowercased on load, so just lowercase app name once
static bool shouldSkipImeCheck() {
	string lowerAppName = strToLower(OpenKeyHelper::getLastAppExecuteName());
	// List is already lowercase from reloadSpecialAppsLists()
	return std::find(_skipImeCheckApps.begin(), _skipImeCheckApps.end(), lowerAppName) != _skipImeCheckApps.end();
}

extern int vSendKeyStepByStep;
extern int vUseGrayIcon;
extern int vShowOnStartUp;
extern int vRunWithWindows;

static HHOOK hKeyboardHook;
static HHOOK hMouseHook;
static HWINEVENTHOOK hSystemEvent;
static KBDLLHOOKSTRUCT* keyboardData;
static MSLLHOOKSTRUCT* mouseData;
static vKeyHookState* pData;
static vector<Uint16> _syncKey;
static Uint32 _flag = 0, _lastFlag = 0, _privateFlag;
static bool _flagChanged = false, _isFlagKey;
static Uint16 _keycode;
static Uint16 _newChar, _newCharHi;

static vector<Uint16> _newCharString;
static Uint16 _newCharSize;
static bool _willSendControlKey = false;

static Uint16 _uniChar[2];
static int _i, _j, _k;
static Uint32 _tempChar;

static string macroText, macroContent;
static int _languageTemp = 0; //use for smart switch key
static vector<Byte> savedSmartSwitchKeyData; ////use for smart switch key
static int _languageBeforeExcludedApp = -1; // Remember language state before entering excluded app (fix for state pollution)

static bool _hasJustUsedHotKey = false;

// Double-tap Alt timing for vTempOffOpenKey (avoids conflicts with apps that use Alt for menus)
static DWORD _lastAltReleaseTime = 0;
static const DWORD DOUBLE_TAP_THRESHOLD_MS = 400;  // Time window for double-tap detection

// IME session cache - check once per composing session, not every keystroke
// This reduces latency by 80%+ in Vietnamese mode
bool _cachedImeState = false;             // Non-static for DebugSnapshot extern access
static bool _imeCheckedThisSession = false; // Reset when buffer cleared or focus changes

// Tripwire E: App switch detection for stale buffer check
static bool _appJustSwitched = false;

// Performance optimization: IME check caching with timeout to prevent blocking
// Prevents GPU-heavy apps from freezing input when they don't respond to IME queries
static uint64_t _lastImeCheckTime = 0;     // Last time IME was checked (GetTickCount64)
static const uint64_t IME_CHECK_TIMEOUT_MS = 500; // Minimum time between IME checks

// Helper to reset IME session cache
inline void resetImeSessionCache() {
    _imeCheckedThisSession = false;
    _cachedImeState = false;
    _lastImeCheckTime = 0; // Reset timestamp to force fresh check
}

// Magic number to identify OpenKey-generated events (prevent hook re-entry)
// Industry standard practice - UniKey, EVKey use similar approach
#define OPENKEY_EXTRA_INFO 0x4F4B

static INPUT backspaceEvent[2];
static INPUT keyEvent[2];

LRESULT CALLBACK keyboardHookProcess(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK mouseHookProcess(int nCode, WPARAM wParam, LPARAM lParam);
VOID CALLBACK winEventProcCallback(HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);

void OpenKeyFree() {
	PerformanceLogger::shutdown();
	UnhookWindowsHookEx(hMouseHook);
	UnhookWindowsHookEx(hKeyboardHook);
	UnhookWinEvent(hSystemEvent);
}

void ReinstallHooks() {
	PERF_START_SECTION(reinstall);  // Track total reinstall time
	
	// Thread-safe: Use static mutex to avoid concurrent reinstalls
	static std::mutex reinstallMutex;
	std::lock_guard<std::mutex> lock(reinstallMutex);
	
	OutputDebugString(_T("NextKey: ReinstallHooks - Starting...\n"));
	
	// Unhook old hooks (if still active)
	if (hKeyboardHook) {
		if (UnhookWindowsHookEx(hKeyboardHook)) {
			OutputDebugString(_T("NextKey: ReinstallHooks - Keyboard hook unhooked\n"));
		}
		hKeyboardHook = NULL;
	}
	
	if (hMouseHook) {
		if (UnhookWindowsHookEx(hMouseHook)) {
			OutputDebugString(_T("NextKey: ReinstallHooks - Mouse hook unhooked\n"));
		}
		hMouseHook = NULL;
	}
	
	// Small delay to ensure hooks are fully released (reduced from 100ms)
	Sleep(20);
	
	// CRITICAL: Resync keyboard state (like OpenKeyInit does)
	// Reset flags first
	_lastFlag = 0;
	_keycode = 0;
	_hasJustUsedHotKey = false;
	
	// Resync _flag with current keyboard state
	_flag = 0;
	if (GetKeyState(VK_LSHIFT) < 0 || GetKeyState(VK_RSHIFT) < 0) _flag |= MASK_SHIFT;
	if (GetKeyState(VK_LCONTROL) < 0 || GetKeyState(VK_RCONTROL) < 0) _flag |= MASK_CONTROL;
	if (GetKeyState(VK_LMENU) < 0 || GetKeyState(VK_RMENU) < 0) _flag |= MASK_ALT;
	if (GetKeyState(VK_LWIN) < 0 || GetKeyState(VK_RWIN) < 0) _flag |= MASK_WIN;
	if (GetKeyState(VK_NUMLOCK) < 0) _flag |= MASK_NUMLOCK;
	if (GetKeyState(VK_CAPITAL) == 1) _flag |= MASK_CAPITAL;
	if (GetKeyState(VK_SCROLL) < 0) _flag |= MASK_SCROLL;
	
	// Log keyboard state after resync (for debugging modifier issues)
	if (PerformanceLogger::isEnabled()) {
		char stateLog[128];
		sprintf_s(stateLog, "REINSTALL_HOOKS: _flag=0x%02X after resync", _flag);
		PerformanceLogger::log(stateLog, 0);
	}
	
	OutputDebugString(_T("NextKey: ReinstallHooks - State variables reset and resynced\n"));
	
	// Reinstall hooks
	HINSTANCE hInstance = GetModuleHandle(NULL);
	hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboardHookProcess, hInstance, 0);
	hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, mouseHookProcess, hInstance, 0);
	
	if (hKeyboardHook && hMouseHook) {
		OutputDebugString(_T("NextKey: ReinstallHooks - Success!\n"));
	} else {
		OutputDebugString(_T("NextKey: ReinstallHooks - FAILED!\n"));
		if (!hKeyboardHook) {
			OutputDebugString(_T("NextKey: ReinstallHooks - Keyboard hook failed\n"));
			PerformanceLogger::log("REINSTALL_HOOKS_KEYBOARD_FAILED", 0);
		}
		if (!hMouseHook) {
			OutputDebugString(_T("NextKey: ReinstallHooks - Mouse hook failed\n"));
			PerformanceLogger::log("REINSTALL_HOOKS_MOUSE_FAILED", 0);
		}
	}
	
	PERF_END_SECTION(reinstall, "REINSTALL_HOOKS_TOTAL");
}

void OpenKeyInit() {
	// === Phase 2: Initialize ConfigManager (handles migration from Registry) ===
	auto& config = ConfigManager::instance();
	config.init();
	
	// Migrate from Registry if config.toml didn't exist (first run or fresh install)
	if (config.needsMigration()) {
		ConfigManager::migrateFromRegistry();
	}
	
	// === Load settings from ConfigManager (RAM, not disk) ===
	// General Tab
	vLanguage = config.getInt("general", "language", 1);
	vInputType = config.getInt("general", "inputType", 0);
	vFreeMark = 0;
	vCodeTable = config.getInt("general", "codeTable", 0);
	vSwitchKeyStatus = config.getInt("general", "switchKey", 0x7A000206);
	vUseSmartSwitchKey = config.getBool("general", "smartSwitch", true) ? 1 : 0;
	
	// Typing Tab (Bộ gõ)
	vCheckSpelling = config.getBool("typing", "checkSpelling", true) ? 1 : 0;
	vRestoreIfWrongSpelling = config.getBool("typing", "restoreWrongSpelling", true) ? 1 : 0;
	vUseModernOrthography = config.getBool("typing", "modernOrthography", false) ? 1 : 0;
	vFixRecommendBrowser = config.getBool("typing", "fixRecommendBrowser", true) ? 1 : 0;
	vUpperCaseFirstChar = config.getBool("typing", "upperCaseFirstChar", false) ? 1 : 0;
	vAllowConsonantZFWJ = config.getBool("typing", "allowZwfj", false) ? 1 : 0;
	vTempOffSpelling = config.getBool("typing", "tempOffSpellingCtrl", false) ? 1 : 0;
	vTempOffOpenKey = config.getBool("typing", "tempOffOpenKeyAlt", false) ? 1 : 0;
	vRememberCode = config.getBool("typing", "rememberCode", true) ? 1 : 0;
	
	// Macro Tab (Gõ tắt)
	vUseMacro = config.getBool("macro", "enabled", true) ? 1 : 0;
	vUseMacroInEnglishMode = config.getBool("macro", "useInEnglishMode", false) ? 1 : 0;
	vAutoCapsMacro = config.getBool("macro", "autoCaps", false) ? 1 : 0;
	vQuickTelex = config.getBool("macro", "quickTelex", false) ? 1 : 0;
	vQuickStartConsonant = config.getBool("macro", "quickStartConsonant", false) ? 1 : 0;
	vQuickEndConsonant = config.getBool("macro", "quickEndConsonant", false) ? 1 : 0;
	vTempOffMacro = config.getBool("macro", "tempOffMacroEsc", false) ? 1 : 0;
	
	// System Tab (Hệ thống)
	vRunWithWindows = config.getBool("system", "runWithWindows", false) ? 1 : 0;
	vRunAsAdmin = config.getBool("system", "runAsAdmin", false) ? 1 : 0;
	vCheckNewVersion = config.getBool("system", "checkNewVersion", false) ? 1 : 0;
	vCreateDesktopShortcut = config.getBool("system", "createDesktopShortcut", false) ? 1 : 0;
	vSupportMetroApp = config.getBool("system", "supportMetroApp", false) ? 1 : 0;
	vFixChromiumBrowser = config.getBool("system", "fixChromiumBrowser", false) ? 1 : 0;
	vSendKeyStepByStep = config.getBool("system", "useClipboard", true) ? 0 : 1;  // Inverted!
	vUseGrayIcon = config.getInt("system", "iconStyle", 0);  // 0=Color, 1=Dark, 2=Light, 3=Custom
	vTrayIconColorV = (COLORREF)config.getInt("system", "customColorV", 0);
	vTrayIconColorE = (COLORREF)config.getInt("system", "customColorE", 0);
	vShowOnStartUp = config.getBool("system", "showOnStartup", false) ? 1 : 0;
	// Font name from config (defaults to "Arial")
	std::string fontName = config.getString("system", "trayIconFontName", "Arial");
	MultiByteToWideChar(CP_UTF8, 0, fontName.c_str(), -1, vTrayIconFontName, sizeof(vTrayIconFontName)/sizeof(TCHAR));
	
	// UI Effects load (shared with dialogs)
	SettingsDialog::s_blurMode = (SettingsDialog::BlurMode)config.getInt("ui", "blurMode", 0);
	
	// Excluded Apps
	vExcludeApps = config.getBool("excludedApps", "enabled", false) ? 1 : 0;
	
	// Debug
	vEnablePerfLog = config.getBool("debug", "enablePerfLog", false) ? 1 : 0;
	vReduceMemory = config.getBool("debug", "reduceMemory", false) ? 1 : 0;
	
	// Initialize performance logger
	PerformanceLogger::init();
	PerformanceLogger::setEnabled(vEnablePerfLog != 0);
	
	// Convert Tool
	convertToolHotKey = config.getInt("convertTool", "hotkey", EMPTY_HOTKEY);
	if (convertToolHotKey == 0) {
		convertToolHotKey = EMPTY_HOTKEY;
	}
	convertToolFromCode = config.getInt("convertTool", "fromCode", 0);
	convertToolToCode = config.getInt("convertTool", "toCode", 0);
	convertToolToAllCaps = config.getBool("convertTool", "toAllCaps", false) ? 1 : 0;
	convertToolToAllNonCaps = config.getBool("convertTool", "toAllNonCaps", false) ? 1 : 0;
	convertToolRemoveMark = config.getBool("convertTool", "removeMark", false) ? 1 : 0;
	convertToolToCapsEachWord = config.getBool("convertTool", "toCapsEachWord", false) ? 1 : 0;
	convertToolToCapsFirstLetter = config.getBool("convertTool", "toCapsFirstLetter", false) ? 1 : 0;
	convertToolDontAlertWhenCompleted = config.getBool("convertTool", "dontAlertCompleted", false) ? 1 : 0;
	vQuickConvertAutoPaste = config.getBool("convertTool", "autoPasteReselect", false) ? 1 : 0;
	vQuickConvertSequential = config.getBool("convertTool", "sequentialMode", false) ? 1 : 0;

	pData = (vKeyHookState*)vKeyInit();

	//pre-create back key
	backspaceEvent[0].type = INPUT_KEYBOARD;
	backspaceEvent[0].ki.dwFlags = 0;
	backspaceEvent[0].ki.wVk = VK_BACK;
	backspaceEvent[0].ki.wScan = 0;
	backspaceEvent[0].ki.time = 0;
	backspaceEvent[0].ki.dwExtraInfo = OPENKEY_EXTRA_INFO;

	backspaceEvent[1].type = INPUT_KEYBOARD;
	backspaceEvent[1].ki.dwFlags = KEYEVENTF_KEYUP;
	backspaceEvent[1].ki.wVk = VK_BACK;
	backspaceEvent[1].ki.wScan = 0;
	backspaceEvent[1].ki.time = 0;
	backspaceEvent[1].ki.dwExtraInfo = OPENKEY_EXTRA_INFO;

	//get key state
	_flag = 0;
	if (GetKeyState(VK_LSHIFT) < 0 || GetKeyState(VK_RSHIFT) < 0) _flag |= MASK_SHIFT;
	if (GetKeyState(VK_LCONTROL) < 0 || GetKeyState(VK_RCONTROL) < 0) _flag |= MASK_CONTROL;
	if (GetKeyState(VK_LMENU) < 0 || GetKeyState(VK_RMENU) < 0) _flag |= MASK_ALT;
	if (GetKeyState(VK_LWIN) < 0 || GetKeyState(VK_RWIN) < 0) _flag |= MASK_WIN;
	if (GetKeyState(VK_NUMLOCK) < 0) _flag |= MASK_NUMLOCK;
	if (GetKeyState(VK_CAPITAL) == 1) _flag |= MASK_CAPITAL;
	if (GetKeyState(VK_SCROLL) < 0) _flag |= MASK_SCROLL;

	// === Phase 3b: Load binary data from ConfigManager (TOML) ===
	// Macros - always use ConfigManager (no registry fallback)
	auto macros = config.getMacros();
	initMacrosFromList(macros);
	
	// Smart Switch data - always use ConfigManager
	auto smartSwitchData = config.getSmartSwitchData();
	initSmartSwitchKeyFromMap(smartSwitchData);
	
	// English-only apps - always use ConfigManager
	auto englishOnlyApps = config.getStringArray("excludedApps", "list");
	initEnglishOnlyAppsFromList(englishOnlyApps);
	
	// Clipboard apps - per-app injection method configuration
	reloadClipboardApps();

	//init hook
	HINSTANCE hInstance = GetModuleHandle(NULL);
	hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboardHookProcess, hInstance, 0);
	hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, mouseHookProcess, hInstance, 0);
	hSystemEvent = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, NULL, winEventProcCallback, 0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
}

void saveSmartSwitchKeyData() {
	// Only update RAM cache - disk save happens on app exit or explicit save
	// This prevents excessive I/O on every app switch
	auto smartSwitchData = getSmartSwitchKeyAsMap();
	ConfigManager::instance().setSmartSwitchData(smartSwitchData);
	// NOTE: Do NOT call save() here - it's called frequently during app switching
}

static vector<Byte> savedEnglishOnlyAppsData;
void saveEnglishOnlyAppsData() {
	// Update RAM cache and save to disk (excluded apps changes are infrequent)
	vector<string> apps;
	getAllEnglishOnlyApps(apps);
	ConfigManager::instance().setStringArray("excludedApps", "list", apps);
	ConfigManager::instance().save();  // OK to save - happens rarely
}

static void InsertKeyLength(const Uint8& len) {
	_syncKey.push_back(len);
}

static inline void prepareKeyEvent(INPUT& input, const Uint16& keycode, const bool& isPress, const DWORD& flag=0) {
	input.type = INPUT_KEYBOARD;
	input.ki.dwFlags = isPress ? flag : flag|KEYEVENTF_KEYUP;
	input.ki.wVk = keycode;
	input.ki.wScan = 0;
	input.ki.time = 0;
	input.ki.dwExtraInfo = OPENKEY_EXTRA_INFO;
}

static inline void prepareUnicodeEvent(INPUT& input, const Uint16& unicode, const bool& isPress) {
	input.type = INPUT_KEYBOARD;
	input.ki.wVk = 0;
	input.ki.wScan = unicode;
	input.ki.time = 0;
	input.ki.dwFlags = (isPress ? 0 : KEYEVENTF_KEYUP) | KEYEVENTF_UNICODE;
	input.ki.dwExtraInfo = OPENKEY_EXTRA_INFO;
}

static void SendCombineKey(const Uint16& key1, const Uint16& key2, const DWORD& flagKey1=0, const DWORD& flagKey2 = 0) {
	prepareKeyEvent(keyEvent[0], key1, true, flagKey1);
	SendInput(1, keyEvent, sizeof(INPUT));

	prepareKeyEvent(keyEvent[0], key2, true, flagKey2);
	prepareKeyEvent(keyEvent[1], key2, false, flagKey2);
	SendInput(2, keyEvent, sizeof(INPUT));

	prepareKeyEvent(keyEvent[0], key1, false, flagKey1);
	SendInput(1, keyEvent, sizeof(INPUT));
}

static void SendKeyCode(Uint32 data) {
	_newChar = (Uint16)data;
	if (!(data & CHAR_CODE_MASK)) {
		if (IS_DOUBLE_CODE(vCodeTable)) //VNI
			InsertKeyLength(1);

		_newChar = keyCodeToCharacter(data);
		if (_newChar == 0) {
			_newChar = (Uint16)data;
			prepareKeyEvent(keyEvent[0], _newChar, true);
			prepareKeyEvent(keyEvent[1], _newChar, false);
			SendInput(2, keyEvent, sizeof(INPUT));
		} else {
			prepareUnicodeEvent(keyEvent[0], _newChar, true);
			prepareUnicodeEvent(keyEvent[1], _newChar, false);
			SendInput(2, keyEvent, sizeof(INPUT));
		}
	} else {
		if (vCodeTable == 0) { //unicode 2 bytes code
			prepareUnicodeEvent(keyEvent[0], _newChar, true);
			prepareUnicodeEvent(keyEvent[1], _newChar, false);
			SendInput(2, keyEvent, sizeof(INPUT));
		} else if (vCodeTable == 1 || vCodeTable == 2 || vCodeTable == 4) { //others such as VNI Windows, TCVN3: 1 byte code
			_newCharHi = HIBYTE(_newChar);
			_newChar = LOBYTE(_newChar);

			prepareUnicodeEvent(keyEvent[0], _newChar, true);
			prepareUnicodeEvent(keyEvent[1], _newChar, false);
			SendInput(2, keyEvent, sizeof(INPUT));

			if (_newCharHi > 32) {
				if (vCodeTable == 2) //VNI
					InsertKeyLength(2);
				prepareUnicodeEvent(keyEvent[0], _newCharHi, true);
				prepareUnicodeEvent(keyEvent[1], _newCharHi, false);
				SendInput(2, keyEvent, sizeof(INPUT));
			} else {
				if (vCodeTable == 2) //VNI
					InsertKeyLength(1);
			}
		} else if (vCodeTable == 3) { //Unicode Compound
			_newCharHi = (_newChar >> 13);
			_newChar &= 0x1FFF;
			_uniChar[0] = _newChar;
			_uniChar[1] = _newCharHi > 0 ? (_unicodeCompoundMark[_newCharHi - 1]) : 0;
			InsertKeyLength(_newCharHi > 0 ? 2 : 1);
			prepareUnicodeEvent(keyEvent[0], _uniChar[0], true);
			prepareUnicodeEvent(keyEvent[1], _uniChar[0], false);
			SendInput(2, keyEvent, sizeof(INPUT));
			if (_newCharHi > 0) {
				prepareUnicodeEvent(keyEvent[0], _uniChar[1], true);
				prepareUnicodeEvent(keyEvent[1], _uniChar[1], false);
				SendInput(2, keyEvent, sizeof(INPUT));
			}
		}
	}
}

static void SendBackspace() {
	SendInput(2, backspaceEvent, sizeof(INPUT));
	if (vSupportMetroApp && OpenKeyHelper::getLastAppExecuteName().compare("ApplicationFrameHost.exe") == 0) {//Metro App
		SendMessage(HWND_BROADCAST, WM_CHAR, VK_BACK, 0L);
		SendMessage(HWND_BROADCAST, WM_CHAR, VK_BACK, 0L);
	}
	if (IS_DOUBLE_CODE(vCodeTable)) { //VNI or Unicode Compound
		if (_syncKey.back() > 1) {
			/*if (!(vCodeTable == 3 && containUnicodeCompoundApp(FRONT_APP))) {
				SendInput(2, backspaceEvent, sizeof(INPUT));
			}*/
			SendInput(2, backspaceEvent, sizeof(INPUT));
			if (vSupportMetroApp && OpenKeyHelper::getLastAppExecuteName().compare("ApplicationFrameHost.exe") == 0) {//Metro App
				SendMessage(HWND_BROADCAST, WM_CHAR, VK_BACK, 0L);
				SendMessage(HWND_BROADCAST, WM_CHAR, VK_BACK, 0L);
			}
		}
		_syncKey.pop_back();
	}
}

static void SendEmptyCharacter() {
	if (IS_DOUBLE_CODE(vCodeTable)) //VNI or Unicode Compound
		InsertKeyLength(1);

	_newChar = 0x202F; //empty char

	prepareUnicodeEvent(keyEvent[0], _newChar, true);
	prepareUnicodeEvent(keyEvent[1], _newChar, false);
	SendInput(2, keyEvent, sizeof(INPUT));
}

static void SendNewCharString(const bool& dataFromMacro = false) {
	PERF_START_SECTION(sendstr);  // Track clipboard/paste performance
	_j = 0;
	_newCharSize = dataFromMacro ? (Uint16)pData->macroData.size() : pData->newCharCount;
	// Pre-allocate extra space: hi-byte characters (TCVN3, VNI, Unicode Compound) 
	// may add extra chars during fill, so allocate 2x to be safe
	size_t requiredSize = static_cast<size_t>(_newCharSize) * 2 + 16;
	if (_newCharString.size() < requiredSize) {
		_newCharString.resize(requiredSize);
	}
	_willSendControlKey = false;
	
	if (_newCharSize > 0) {
		for (_k = dataFromMacro ? 0 : pData->newCharCount - 1;
			dataFromMacro ? _k < pData->macroData.size() : _k >= 0;
			dataFromMacro ? _k++ : _k--) {

			_tempChar = DYNA_DATA(dataFromMacro, _k);
			if (_tempChar & PURE_CHARACTER_MASK) {
				_newCharString[_j++] = _tempChar;
				if (IS_DOUBLE_CODE(vCodeTable)) {
					InsertKeyLength(1);
				}
			} else if (!(_tempChar & CHAR_CODE_MASK)) {
				if (IS_DOUBLE_CODE(vCodeTable)) //VNI
					InsertKeyLength(1);
				_newCharString[_j++] = keyCodeToCharacter(_tempChar);
			} else {
				_newChar = _tempChar;
				if (vCodeTable == 0) {  //unicode 2 bytes code
					_newCharString[_j++] = _newChar;
				} else if (vCodeTable == 1 || vCodeTable == 2 || vCodeTable == 4) { //others such as VNI Windows, TCVN3: 1 byte code
					_newCharHi = HIBYTE(_newChar);
					_newChar = LOBYTE(_newChar);
					_newCharString[_j++] = _newChar;

					if (_newCharHi > 32) {
						if (vCodeTable == 2) //VNI
							InsertKeyLength(2);
						_newCharString[_j++] = _newCharHi;
						_newCharSize++;
					}
					else {
						if (vCodeTable == 2) //VNI
							InsertKeyLength(1);
					}
				} else if (vCodeTable == 3) { //Unicode Compound
					_newCharHi = (_newChar >> 13);
					_newChar &= 0x1FFF;

					InsertKeyLength(_newCharHi > 0 ? 2 : 1);
					_newCharString[_j++] = _newChar;
					if (_newCharHi > 0) {
						_newCharSize++;
						_newCharString[_j++] = _unicodeCompoundMark[_newCharHi - 1];
					}

				}
			}
		}//end for
	}

	if (pData->code == vRestore || pData->code == vRestoreAndStartNewSession) { //if is restore
		if (keyCodeToCharacter(_keycode) != 0) {
			_newCharSize++;
			_newCharString[_j++] = keyCodeToCharacter(_keycode | ((_flag & MASK_SHIFT) || (_flag & MASK_CAPITAL) ? CAPS_MASK : 0));
		} else {
			_willSendControlKey = true;
		}
	}
	if (pData->code == vRestoreAndStartNewSession) {
		startNewSession();
	}

	// DEBUG: Log clipboard timing details
	LARGE_INTEGER clipStart, clipEnd, pasteStart, pasteEnd, freq;
	
	// Get per-app injection method FIRST (before clipboard ops)
	int perAppDelayMs = 0;
	int injectionMethod = getClipboardMethodForCurrentApp(perAppDelayMs);
	
	// Method: -1 = not in config (use SendInputKey default), 0 = ShiftInsert, 1 = CtrlV, 2 = SendInputKey with delay
	// SAFETY: Use actual vector size for bounds, not _newCharSize which may have changed during fill
	size_t charCount = (std::min)(static_cast<size_t>(_newCharSize), _newCharString.size());
	
	if (PerformanceLogger::isEnabled()) {
		QueryPerformanceCounter(&pasteStart);
	}
	
	if (injectionMethod == -1) {
		// App NOT in config → Use SendInputKey (default mode, no clipboard needed)
		for (size_t i = 0; i < charCount; i++) {
			Uint16 ch = _newCharString[i];
			if (ch == 0) break;
			
			INPUT inputs[2] = {0};
			inputs[0].type = INPUT_KEYBOARD;
			inputs[0].ki.wVk = 0;
			inputs[0].ki.wScan = ch;
			inputs[0].ki.dwFlags = KEYEVENTF_UNICODE;
			inputs[0].ki.dwExtraInfo = OPENKEY_EXTRA_INFO;
			
			inputs[1].type = INPUT_KEYBOARD;
			inputs[1].ki.wVk = 0;
			inputs[1].ki.wScan = ch;
			inputs[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
			inputs[1].ki.dwExtraInfo = OPENKEY_EXTRA_INFO;
			
			SendInput(2, inputs, sizeof(INPUT));
		}
	} else if (injectionMethod == 2) {
		// SendInputKey mode with delay (for games/sensitive apps, no clipboard needed)
		int delayMs = perAppDelayMs > 0 ? perAppDelayMs : SMART_BATCH_DELAY_MS;
		Sleep(delayMs);
		
		for (size_t i = 0; i < charCount; i++) {
			Uint16 ch = _newCharString[i];
			if (ch == 0) break;
			
			INPUT inputs[2] = {0};
			inputs[0].type = INPUT_KEYBOARD;
			inputs[0].ki.wVk = 0;
			inputs[0].ki.wScan = ch;
			inputs[0].ki.dwFlags = KEYEVENTF_UNICODE;
			inputs[0].ki.dwExtraInfo = OPENKEY_EXTRA_INFO;
			
			inputs[1].type = INPUT_KEYBOARD;
			inputs[1].ki.wVk = 0;
			inputs[1].ki.wScan = ch;
			inputs[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
			inputs[1].ki.dwExtraInfo = OPENKEY_EXTRA_INFO;
			
			SendInput(2, inputs, sizeof(INPUT));
		}
	} else {
		// Clipboard modes (0 = Shift+Insert, 1 = Ctrl+V) - need to set clipboard first
		if (PerformanceLogger::isEnabled()) {
			QueryPerformanceCounter(&clipStart);
		}
		
		// setClipboardText returns bool based on OpenClipboard/SetClipboardData success
		bool clipboardSuccess = OpenKeyHelper::setClipboardText((LPCTSTR)_newCharString.data(), _newCharSize + 1, CF_UNICODETEXT);
		
		if (PerformanceLogger::isEnabled()) {
			QueryPerformanceCounter(&clipEnd);
			QueryPerformanceFrequency(&freq);
			double clipMs = (double)(clipEnd.QuadPart - clipStart.QuadPart) * 1000.0 / freq.QuadPart;
			
			char logBuf[256];
			sprintf_s(logBuf, "CLIPBOARD_SET[App=%s,Chars=%d,BS=%d,OK=%d] %.3fms", 
				OpenKeyHelper::getLastAppExecuteName().c_str(), _newCharSize, pData->backspaceCount, clipboardSuccess ? 1 : 0, clipMs);
			PerformanceLogger::log(logBuf, clipMs);
		}
		
		if (injectionMethod == 1) {
			// Ctrl+V mode
			SendCombineKey(VK_CONTROL, 'V', 0, 0);
		} else {
			// ShiftInsert mode (injectionMethod == 0)
			SendCombineKey(KEY_LEFT_SHIFT, VK_INSERT, 0, KEYEVENTF_EXTENDEDKEY);
		}
		
		if (perAppDelayMs > 0) {
			Sleep(perAppDelayMs);
		}
		
		// Step 3: Clipboard feedback loop - track failures and downgrade if needed
		HWND clipboardHwnd = GetForegroundWindow();
		RuntimeProfile* clipProfile = getProfileForHwnd(clipboardHwnd);
		if (clipProfile && clipProfile->injectionMethod >= 0) {
			if (!clipboardSuccess) {
				clipProfile->failureCount++;
				if (clipProfile->failureCount >= 3) {
					// Downgrade: clipboard → SendInput after 3 failures
					clipProfile->injectionMethod = -1;
					if (PerformanceLogger::isEnabled()) {
						PerformanceLogger::log("PROFILE_DOWNGRADE", 0);
					}
				}
			} else {
				// Success - gradually reduce failure count
				if (clipProfile->failureCount > 0) {
					clipProfile->failureCount--;
				}
			}
		}
	}
	
	if (PerformanceLogger::isEnabled()) {
		QueryPerformanceCounter(&pasteEnd);
		QueryPerformanceFrequency(&freq);
		double pasteMs = (double)(pasteEnd.QuadPart - pasteStart.QuadPart) * 1000.0 / freq.QuadPart;
		
		char logBuf[256];
		const char* methodName = (injectionMethod == -1) ? "SENDINPUT_DEFAULT" :
		                         (injectionMethod == 2) ? "SENDINPUT_DELAY" : 
		                         (injectionMethod == 1) ? "CTRL_V" : "SHIFT_INSERT";
		sprintf_s(logBuf, "[%s] PASTE_%s[delay=%d] %.3fms", 
			OpenKeyHelper::getLastAppExecuteName().c_str(), methodName, perAppDelayMs, pasteMs);
		PerformanceLogger::log(logBuf, pasteMs);
	}
	
	// Milestone 2: Latency probe for auto-classification
	// Use injection latency to classify profile (NativeRichTSF vs QtElectronLike)
	{
		HWND probeHwnd = GetForegroundWindow();
		RuntimeProfile* probeProfile = getProfileForHwnd(probeHwnd);
		if (probeProfile && !probeProfile->isProbeComplete) {
			// Calculate injection latency (reuse timing if perf logger enabled, else measure now)
			QueryPerformanceCounter(&pasteEnd);
			QueryPerformanceFrequency(&freq);
			double latencyMs = (double)(pasteEnd.QuadPart - pasteStart.QuadPart) * 1000.0 / freq.QuadPart;
			
			// Track MIN latency to avoid false positives from background load
			uint16_t latency = (uint16_t)latencyMs;
			if (latency < probeProfile->minLatencyMs) {
				probeProfile->minLatencyMs = latency;
			}
			probeProfile->probeCount++;
			
			// Classify after 3 probes for stability
			if (probeProfile->probeCount >= 3) {
				classifyProfile(probeProfile);
			}
		}
	}
	
	//the case when hCode is vRestore or vRestoreAndStartNewSession,
	//the word is invalid and last key is control key such as TAB, LEFT ARROW, RIGHT ARROW,...
	if (_willSendControlKey) {
		SendKeyCode(_keycode);
	}
	
	// Log total time
	if(PerformanceLogger::isEnabled()) {
		QueryPerformanceCounter(&_perfEnd_sendstr);
		QueryPerformanceFrequency(&_perfFreq_sendstr);
		double _ms_sendstr = (double)(_perfEnd_sendstr.QuadPart - _perfStart_sendstr.QuadPart) * 1000.0 / _perfFreq_sendstr.QuadPart;
		char debugTag[256];
		sprintf_s(debugTag, "[%s] SEND_STRING_TOTAL[Chars=%d]", 
			OpenKeyHelper::getLastAppExecuteName().c_str(), _newCharSize);
		PerformanceLogger::log(debugTag, _ms_sendstr);
	}
}


bool checkHotKey(int hotKeyData, bool checkKeyCode = true) {
	if ((hotKeyData & (~0x8000)) == EMPTY_HOTKEY)
		return false;
	if (HAS_CONTROL(hotKeyData) ^ GET_BOOL(_lastFlag & MASK_CONTROL))
		return false;
	if (HAS_OPTION(hotKeyData) ^ GET_BOOL(_lastFlag & MASK_ALT))
		return false;
	if (HAS_COMMAND(hotKeyData) ^ GET_BOOL(_lastFlag & MASK_WIN))
		return false;
	if (HAS_SHIFT(hotKeyData) ^ GET_BOOL(_lastFlag & MASK_SHIFT))
		return false;
	if (checkKeyCode) {
		if (GET_SWITCH_KEY(hotKeyData) != _keycode)
			return false;
	}
	return true;
}

void switchLanguage() {
	// Block language switching for English-only apps
	if (vExcludeApps && isEnglishOnlyApp(OpenKeyHelper::getFrontMostAppExecuteName())) {
		// Beep to indicate switch is blocked
		if (HAS_BEEP(vSwitchKeyStatus))
			MessageBeep(MB_ICONWARNING);
		return;
	}
	
	if (vLanguage == 0)
		vLanguage = 1;
	else
		vLanguage = 0;
	
	// Reset IME cache when language changes (new session context)
	resetImeSessionCache();
	
	if (HAS_BEEP(vSwitchKeyStatus))
		MessageBeep(MB_OK);
	AppDelegate::getInstance()->onInputMethodChangedFromHotKey();
	if (vUseSmartSwitchKey) {
		setAppInputMethodStatus(OpenKeyHelper::getFrontMostAppExecuteName(), vLanguage | (vCodeTable << 1));
		saveSmartSwitchKeyData();
	}
	startNewSession();
}

static void SendPureCharacter(const Uint16& ch) {
	if (ch < 128)
		SendKeyCode(ch);
	else {
		prepareUnicodeEvent(keyEvent[0], ch, true);
		prepareUnicodeEvent(keyEvent[1], ch, false);
		SendInput(2, keyEvent, sizeof(INPUT));
		if (IS_DOUBLE_CODE(vCodeTable)) {
			InsertKeyLength(1);
		}
	}
}

static void handleMacro() {
	PERF_START_SECTION(macro);  // Track macro performance
	int macroLen = (int)pData->macroData.size();
	
	//fix autocomplete
	if (vFixRecommendBrowser) {
		SendEmptyCharacter();
		pData->backspaceCount++;
	}

	//send backspace
	if (pData->backspaceCount > 0) {
		for (int i = 0; i < pData->backspaceCount; i++) {
			SendBackspace();
		}
	}
	//send real data
	if (!vSendKeyStepByStep) {
		SendNewCharString(true);
	} else {
		for (int i = 0; i < pData->macroData.size(); i++) {
			if (pData->macroData[i] & PURE_CHARACTER_MASK) {
				SendPureCharacter(pData->macroData[i]);
			} else {
				SendKeyCode(pData->macroData[i]);
			}
		}
	}
	// Only send trigger key in Vietnamese mode
	// Vietnamese mode (vLanguage != 0): needs trigger key sent (replacing VN text)
	// English mode (vLanguage == 0): trigger already consumed by hook, sending again creates duplicate
	if (vLanguage != 0) {
		SendKeyCode(_keycode | (_flag & MASK_SHIFT ? CAPS_MASK : 0));
	}
	
	// Log with macro length context
	if(PerformanceLogger::isEnabled()) {
		QueryPerformanceCounter(&_perfEnd_macro);
		QueryPerformanceFrequency(&_perfFreq_macro);
		double _ms_macro = (double)(_perfEnd_macro.QuadPart - _perfStart_macro.QuadPart) * 1000.0 / _perfFreq_macro.QuadPart;
		if(_ms_macro > PERF_LOG_THRESHOLD_MS) {
			char debugTag[64];
			sprintf_s(debugTag, "MACRO[Len=%d]", macroLen);
			PerformanceLogger::log(debugTag, _ms_macro);
		}
	}
}

static bool SetModifierMask(const Uint16& vkCode) {
	// For caps lock case, toggling the flag isn't enough. We need to check the actual state, which should be done before each key press.
	// Example: the caps lock state can be changed without the key being pressed, or the key toggle is made with admin privilege, making the app not able to detect the change.
	if (GetKeyState(VK_CAPITAL) == 1) _flag |= MASK_CAPITAL;
	else _flag &= ~MASK_CAPITAL;

	if (vkCode == VK_LSHIFT || vkCode == VK_RSHIFT) _flag |= MASK_SHIFT;
	else if (vkCode == VK_LCONTROL || vkCode == VK_RCONTROL) _flag |= MASK_CONTROL;
	else if (vkCode == VK_LMENU || vkCode == VK_RMENU) _flag |= MASK_ALT;
	else if (vkCode == VK_LWIN || vkCode == VK_RWIN) _flag |= MASK_WIN;
	else if (vkCode == VK_NUMLOCK) _flag |= MASK_NUMLOCK;
	else if (vkCode == VK_SCROLL) _flag |= MASK_SCROLL;
	else { 
		_isFlagKey = false;
		return false; 
	}
	_isFlagKey = true;
	return true;
}

static bool UnsetModifierMask(const Uint16& vkCode) {
	if (vkCode == VK_LSHIFT || vkCode == VK_RSHIFT) _flag &= ~MASK_SHIFT;
	else if (vkCode == VK_LCONTROL || vkCode == VK_RCONTROL) _flag &= ~MASK_CONTROL;
	else if (vkCode == VK_LMENU || vkCode == VK_RMENU) _flag &= ~MASK_ALT;
	else if (vkCode == VK_LWIN || vkCode == VK_RWIN) _flag &= ~MASK_WIN;
	else if (vkCode == VK_NUMLOCK) _flag &= ~MASK_NUMLOCK;
	else if (vkCode == VK_SCROLL) _flag &= ~MASK_SCROLL;
	else { 
		_isFlagKey = false;
		return false; 
	}
	_isFlagKey = true;
	return true;
}

LRESULT CALLBACK keyboardHookProcess(int nCode, WPARAM wParam, LPARAM lParam) {
	
	keyboardData = (KBDLLHOOKSTRUCT *)lParam;
	//ignore my event (check for OpenKey magic number)
	if (keyboardData->dwExtraInfo == OPENKEY_EXTRA_INFO) {
		return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
	}
	
	// === TRIPWIRE E: App switch with stale buffer ===
	extern Byte _index;  // Engine index
	if (_appJustSwitched && _index > 0) {
		captureDebugSnapshot("TRIPWIRE_E: STALE_WORD_AFTER_SWITCH", 
			OpenKeyHelper::getLastAppExecuteName().c_str(), 0, -1);
	}
	_appJustSwitched = false;  // Reset after check
	
	// CRITICAL: Always update modifier state regardless of language mode
	// This ensures proper state tracking when switching between EN/VN modes
	if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
		SetModifierMask((Uint16)keyboardData->vkCode);
	} else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
		UnsetModifierMask((Uint16)keyboardData->vkCode);
	}
	if (!_isFlagKey && wParam != WM_KEYUP && wParam != WM_SYSKEYUP)
		_keycode = (Uint16)keyboardData->vkCode;
	
	// ESC key: Skip macro for next word (when enabled)
	// User presses ESC before typing a word that would trigger macro → macro skipped
	if (vTempOffMacro && _keycode == VK_ESCAPE && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
		vSetTempSkipMacro(true);  // Set flag in Engine
		// Don't consume ESC - let it pass through for other uses (close dialogs, etc.)
	}
	
	// OPTIMIZATION P1.1 (REVISED): Early exit for English mode
	// Checked AFTER modifier state update to ensure state consistency
	if (vLanguage == 0) {
		// Still need to handle language switch hotkey in English mode
		if ((wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) && !_isFlagKey && _keycode != 0) {
			if (GET_SWITCH_KEY(vSwitchKeyStatus) == _keycode && 
				checkHotKey(vSwitchKeyStatus, GET_SWITCH_KEY(vSwitchKeyStatus) != 0xFE)) {
				switchLanguage();
				_hasJustUsedHotKey = true;
				_keycode = 0;
				return -1; // Block key event
			}
			// Also handle convert tool hotkey
			DEBUG_LOG_FMT("QC_HOTKEY_CHECK", "English keydown: keycode=%d(0x%02X), hotkey=0x%08X, expected=%d", 
				_keycode, _keycode, convertToolHotKey, GET_SWITCH_KEY(convertToolHotKey));
			if (GET_SWITCH_KEY(convertToolHotKey) == _keycode && 
				checkHotKey(convertToolHotKey, GET_SWITCH_KEY(convertToolHotKey) != 0xFE)) {
				DEBUG_LOG("QC_HOTKEY", "English mode keydown: triggering onQuickConvert");
				AppDelegate::getInstance()->onQuickConvert();
				_hasJustUsedHotKey = true;
				_keycode = 0;
				return -1; // Block key event
			}
		} else if (_isFlagKey) {
			// Handle flag key combos for hotkeys (e.g., release Ctrl after Ctrl+Shift+Z)
			if (_lastFlag == 0 || _lastFlag < _flag)
				_lastFlag = _flag;
			else if (_lastFlag > _flag) {
				// Check switch on flag key release
				// IMPORTANT: Only switch if no other hotkey was just triggered (e.g., Ctrl+Shift+X)
				// Prevents Ctrl+Shift from triggering when Ctrl+Shift+X was intended
				if (!_hasJustUsedHotKey && checkHotKey(vSwitchKeyStatus, GET_SWITCH_KEY(vSwitchKeyStatus) != 0xFE)) {
					switchLanguage();
					_hasJustUsedHotKey = true;
				}
				if (checkHotKey(convertToolHotKey, GET_SWITCH_KEY(convertToolHotKey) != 0xFE)) {
					if (!_hasJustUsedHotKey) {  // Prevent duplicate if already triggered on keydown
						DEBUG_LOG("QC_HOTKEY", "English mode flag release: triggering onQuickConvert");
						AppDelegate::getInstance()->onQuickConvert();
					}
					_hasJustUsedHotKey = true;
				}
				_lastFlag = _flag;
				_hasJustUsedHotKey = false;
			}
			_keycode = 0;
			return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
		}
		
		// Handle macro in English mode if enabled
		if (vUseMacro && vUseMacroInEnglishMode && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
			vEnglishMode(((wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) ? vKeyEventState::KeyDown : vKeyEventState::MouseDown),
				_keycode,
				(_flag & MASK_SHIFT) || (_flag & MASK_CAPITAL),
				OTHER_CONTROL_KEY);

			if (pData->code == vReplaceMaro) { //handle macro in english mode
				handleMacro();
				return NULL;
			}
		}
		return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
	}
	
	// OPTIMIZATION: IME check only at START of composing session (not every keystroke)
	// This avoids expensive SendMessage() call in hot path
	// Check if IME pad is open when typing Japanese/Chinese...
	
	// Performance optimization: Check timeout to prevent blocking on GPU-heavy apps
	uint64_t currentTime = GetTickCount64();
	bool shouldCheckIME = !_imeCheckedThisSession || 
	                     (currentTime - _lastImeCheckTime) >= IME_CHECK_TIMEOUT_MS;
	
	// Debug: Log when cache prevents IME check (helps verify optimization works)
	if (!shouldCheckIME && PerformanceLogger::isEnabled()) {
		PerformanceLogger::log("IME_CHECK_CACHE_HIT", 0);
	}
	
	// Only query IME state once per session OR after timeout (500ms)
	if (shouldCheckIME) {
		PERF_START_SECTION(ime);
		HWND hWnd = GetForegroundWindow();
		HWND hIME = ImmGetDefaultIMEWnd(hWnd);
		_cachedImeState = false;
		
		// Only call SendMessage if IME window exists, use timeout to avoid blocking
		if (hIME != NULL) {
			DWORD_PTR dwResult = 0;
			if (SendMessageTimeout(hIME, WM_IME_CONTROL, IMC_GETOPENSTATUS, 0, 
			                       SMTO_ABORTIFHUNG | SMTO_BLOCK, 10, &dwResult)) {
				_cachedImeState = (dwResult != 0);
			}
		}
		_imeCheckedThisSession = true;
		_lastImeCheckTime = currentTime; // Update timestamp after successful check
		
		// Debug: Log IME check (should appear once per word, not per keystroke)
		if(PerformanceLogger::isEnabled()) {
			QueryPerformanceCounter(&_perfEnd_ime);
			QueryPerformanceFrequency(&_perfFreq_ime);
			double _ms_ime_debug = (double)(_perfEnd_ime.QuadPart - _perfStart_ime.QuadPart) * 1000.0 / _perfFreq_ime.QuadPart;
			if(_ms_ime_debug > PERF_LOG_THRESHOLD_MS) {
				char debugTag[64];
				sprintf_s(debugTag, "IME_CHECK[Mode=%s]", vLanguage == 0 ? "E" : "V");
				PerformanceLogger::log(debugTag, _ms_ime_debug);
			}
		}
	}
	
	// Use cached IME state (skip Vietnamese processing if IME is active)
	// Skip IME check for MS Office apps that falsely report IME as ON
	if (_cachedImeState && !shouldSkipImeCheck()) {
		return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
	}
	
	// Vietnamese mode processing (only when vLanguage != 0)
	//switch language shortcut; convert hotkey
	if ((wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) && !_isFlagKey && _keycode != 0) {
		if (GET_SWITCH_KEY(vSwitchKeyStatus) != _keycode && GET_SWITCH_KEY(convertToolHotKey) != _keycode) {
			_lastFlag = 0;
		} else {
			if (GET_SWITCH_KEY(vSwitchKeyStatus) == _keycode && checkHotKey(vSwitchKeyStatus, GET_SWITCH_KEY(vSwitchKeyStatus) != 0xFE)) {
				switchLanguage();
				_hasJustUsedHotKey = true;
				_keycode = 0;
				return -1;
			}
			if (GET_SWITCH_KEY(convertToolHotKey) == _keycode && checkHotKey(convertToolHotKey, GET_SWITCH_KEY(convertToolHotKey) != 0xFE)) {
				DEBUG_LOG("QC_HOTKEY", "Vietnamese mode keydown: triggering onQuickConvert");
				AppDelegate::getInstance()->onQuickConvert();
				_hasJustUsedHotKey = true;
				_keycode = 0;
				return -1;
			}
		}
		_hasJustUsedHotKey = _lastFlag != 0;
	} else if (_isFlagKey) {
		if (_lastFlag == 0 || _lastFlag < _flag)
			_lastFlag = _flag;
		else if (_lastFlag > _flag) {
			// Check switch - only if no other hotkey was just triggered
			// Prevents Ctrl+Shift from triggering when Ctrl+Shift+X (convert) was intended
			if (!_hasJustUsedHotKey && checkHotKey(vSwitchKeyStatus, GET_SWITCH_KEY(vSwitchKeyStatus) != 0xFE)) {
				switchLanguage();
				_hasJustUsedHotKey = true;
			}
			if (checkHotKey(convertToolHotKey, GET_SWITCH_KEY(convertToolHotKey) != 0xFE)) {
				if (!_hasJustUsedHotKey) {  // Prevent duplicate if already triggered on keydown
					DEBUG_LOG("QC_HOTKEY", "Vietnamese mode flag release: triggering onQuickConvert");
					AppDelegate::getInstance()->onQuickConvert();
				}
				_hasJustUsedHotKey = true;
			}
			//check temporarily turn off spell checking
			if (vTempOffSpelling && !_hasJustUsedHotKey && _lastFlag & MASK_CONTROL) {
				vTempOffSpellChecking();
			}
			// Double-tap Alt to temporarily disable Vietnamese (avoids conflicts with app menus)
			// Old behavior: hold Alt. New behavior: double-tap Alt within 400ms window.
			if (vTempOffOpenKey && !_hasJustUsedHotKey && _lastFlag & MASK_ALT) {
				DWORD now = GetTickCount();
				if (now - _lastAltReleaseTime < DOUBLE_TAP_THRESHOLD_MS) {
					// Double-tap detected: activate temp disable
					vTempOffEngine();
					_lastAltReleaseTime = 0;  // Reset after activation
				} else {
					// First tap: record time for next tap
					_lastAltReleaseTime = now;
				}
			}
			_lastFlag = _flag;
			_hasJustUsedHotKey = false;
		}
		_keycode = 0;
		return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
	}

	//if is in english mode
	if (vLanguage == 0) {
		if (vUseMacro && vUseMacroInEnglishMode && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
			vEnglishMode(((wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) ? vKeyEventState::KeyDown : vKeyEventState::MouseDown),
				_keycode,
				(_flag & MASK_SHIFT) || (_flag & MASK_CAPITAL),
				OTHER_CONTROL_KEY);

			if (pData->code == vReplaceMaro) { //handle macro in english mode
				handleMacro();
				return NULL;
			}
		}
		return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
	}

	//handle keyboard
	if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
		//send event signal to Engine
		PERF_START_SECTION(engine);  // Time engine processing
		vKeyHandleEvent(vKeyEvent::Keyboard,
						vKeyEventState::KeyDown,
						_keycode,
						(_flag & MASK_SHIFT && _flag & MASK_CAPITAL) ? 0 : (_flag & MASK_SHIFT ? 1 : (_flag & MASK_CAPITAL ? 2 : 0)),
						OTHER_CONTROL_KEY);
		PERF_END_SECTION(engine, "ENGINE_PROCESS");
		if (pData->code == vDoNothing) { //do nothing
			if (IS_DOUBLE_CODE(vCodeTable)) { //VNI
				if (pData->extCode == 1) { //break key - session ended
					resetImeSessionCache();  // Reset IME cache for new session
					_syncKey.clear();
				} else if (pData->extCode == 2) { //delete key
					if (_syncKey.size() > 0) {
						if (_syncKey.back() > 1 && (vCodeTable == 2 || vCodeTable == 3)) {
							//send one more backspace
							SendInput(2, backspaceEvent, sizeof(INPUT));
						}
						_syncKey.pop_back();
					}
				} else if (pData->extCode == 3) { //normal key
					InsertKeyLength(1);
				}
			}
			return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
		} else if (pData->code == vWillProcess || pData->code == vRestore || pData->code == vRestoreAndStartNewSession) { //handle result signal
			//fix autocomplete
			if (vFixRecommendBrowser && pData->extCode != 4) {
				// OPTIMIZED: Use RuntimeProfile flags - NO strToLower/std::find in hot path!
				HWND focusHwnd = GetForegroundWindow();
				RuntimeProfile* profile = getProfileForHwnd(focusHwnd);
				
				// Skip empty char for Qt/Electron apps (causes Input Context init lag)
				bool skipEmptyChar = profile && profile->hasFlag(ProfileFlags::SkipEmptyChar);
				
				if (!skipEmptyChar) {
					// Check if BrowserLike (Chromium) - uses Shift+Left instead of empty char
					bool isBrowserLike = profile && (profile->type == ProfileType::BrowserLike);
					
					if (vFixChromiumBrowser && isBrowserLike) {
						SendCombineKey(KEY_LEFT_SHIFT, KEY_LEFT, 0, KEYEVENTF_EXTENDEDKEY);
						if (pData->backspaceCount == 1)
							pData->backspaceCount--;
					} else {
						SendEmptyCharacter();
						pData->backspaceCount++;
					}
				}
				// Qt/Electron apps: skip empty char entirely (prevent lag from Input Context init)
			}
			
			//send backspace
			if (pData->backspaceCount > 0 && pData->backspaceCount < MAX_BUFF) {
				PERF_START_SECTION(backspace);
				int bsCount = pData->backspaceCount;  // Save for logging
				for (_i = 0; _i < pData->backspaceCount; _i++) {
					SendBackspace();
				}
				// Log with backspace count context
				if(PerformanceLogger::isEnabled()) {
					QueryPerformanceCounter(&_perfEnd_backspace);
					QueryPerformanceFrequency(&_perfFreq_backspace);
					double _ms_bs = (double)(_perfEnd_backspace.QuadPart - _perfStart_backspace.QuadPart) * 1000.0 / _perfFreq_backspace.QuadPart;
					if(_ms_bs > PERF_LOG_THRESHOLD_MS) {
						char debugTag[64];
						sprintf_s(debugTag, "SEND_BACKSPACE[Count=%d]", bsCount);
						PerformanceLogger::log(debugTag, _ms_bs);
					}
				}
			}

			//send new character
			if (!vSendKeyStepByStep) {
				SendNewCharString();
			} else {
				// Step-by-step mode: log timing for each character
				LARGE_INTEGER stepStart, stepEnd, freq;
				if (PerformanceLogger::isEnabled()) {
					QueryPerformanceCounter(&stepStart);
				}
				
				if (pData->newCharCount > 0 && pData->newCharCount <= MAX_BUFF) {
					for (int i = pData->newCharCount - 1; i >= 0; i--) {
						SendKeyCode(pData->charData[i]);
					}
				}
				if (pData->code == vRestore || pData->code == vRestoreAndStartNewSession) {
					SendKeyCode(_keycode | ((_flag & MASK_CAPITAL) || (_flag & MASK_SHIFT) ? CAPS_MASK : 0));
				}
				if (pData->code == vRestoreAndStartNewSession) {
					startNewSession();
				}
				
				// Log step-by-step timing
				if (PerformanceLogger::isEnabled()) {
					QueryPerformanceCounter(&stepEnd);
					QueryPerformanceFrequency(&freq);
					double stepMs = (double)(stepEnd.QuadPart - stepStart.QuadPart) * 1000.0 / freq.QuadPart;
					
					char logBuf[256];
					sprintf_s(logBuf, "[%s] STEP_BY_STEP[Chars=%d,BS=%d] %.3fms", 
						OpenKeyHelper::getLastAppExecuteName().c_str(), pData->newCharCount, pData->backspaceCount, stepMs);
					PerformanceLogger::log(logBuf, stepMs);
				}
			}
		} else if (pData->code == vReplaceMaro) { //MACRO
			handleMacro();
		}
		return -1; //consume event

	}
	return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}

LRESULT CALLBACK mouseHookProcess(int nCode, WPARAM wParam, LPARAM lParam) {
	mouseData = (MSLLHOOKSTRUCT *)lParam;
	switch (wParam) {
	case WM_LBUTTONDOWN:
		// Reset IME cache on left mouse click - user may have changed focus/input
		resetImeSessionCache();
		// fall through
	
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_XBUTTONDOWN:
	case WM_NCXBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	case WM_XBUTTONUP:
	case WM_NCXBUTTONUP:
		//send event signal to Engine
		vKeyHandleEvent(vKeyEvent::Mouse, vKeyEventState::MouseDown, 0);
		if (IS_DOUBLE_CODE(vCodeTable)) { //VNI
			_syncKey.clear();
		}
		break;
	}
	return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
}

VOID CALLBACK winEventProcCallback(HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime) {
	PERF_START();  // Track app switch performance
	const char* appName = "unknown";  // For logging
	
	// Reset IME session cache on focus change (new app = new session)
	// This ensures fresh IME check when switching to GPU-heavy apps like games/Chrome Cast
	resetImeSessionCache();
	
	// Tripwire E: Mark app switch for stale buffer detection
	_appJustSwitched = true;
	
	// ALWAYS create/update RuntimeProfile for this HWND
	// This applies user overrides regardless of SmartSwitchKey setting
	string& exe = OpenKeyHelper::getFrontMostAppExecuteName();
	appName = exe.c_str();  // Save for logging
	
	RuntimeProfile& profile = getOrCreateProfile(hwnd, exe);
	
	// Trigger periodic cleanup of stale HWND entries
	triggerCleanupIfNeeded();
	
	//smart switch key
	if (vUseSmartSwitchKey || vRememberCode) {
		if (exe.compare("explorer.exe") == 0) //dont apply with windows explorer
			return;
		
	// Check if this app is in English-only list
		if (vExcludeApps && isEnglishOnlyApp(exe)) {
			// Save language state BEFORE forcing E mode (only if not already saved)
			if (_languageBeforeExcludedApp == -1) {
				_languageBeforeExcludedApp = vLanguage;
			}
			// Force English mode for excluded apps
			if (vLanguage != 0) {
				vLanguage = 0;
				AppDelegate::getInstance()->onInputMethodChangedFromHotKey();
			}
			startNewSession();
			vTempOffEngine(false);
			// Don't save to SmartSwitchKey for excluded apps
			return;
		}
		
		// Determine default language for new apps
		// If coming from excluded app, use the saved state instead of current vLanguage (which is E mode)
		int defaultLanguageForNewApp = vLanguage;
		if (_languageBeforeExcludedApp != -1) {
			defaultLanguageForNewApp = _languageBeforeExcludedApp;
			_languageBeforeExcludedApp = -1;  // Reset after use
		}
		
		_languageTemp = getAppInputMethodStatus(exe, defaultLanguageForNewApp | (vCodeTable << 1));
		vTempOffEngine(false);
		if (vUseSmartSwitchKey && (_languageTemp & 0x01) != vLanguage) {
			if (_languageTemp != -1) {
				vLanguage = _languageTemp;
				AppDelegate::getInstance()->onInputMethodChangedFromHotKey();
			} else {
				// New app: use defaultLanguageForNewApp (already saved by getAppInputMethodStatus)
				if (defaultLanguageForNewApp != vLanguage) {
					vLanguage = defaultLanguageForNewApp;
					AppDelegate::getInstance()->onInputMethodChangedFromHotKey();
				}
				saveSmartSwitchKeyData();
			}
		}
		startNewSession();
		if (vRememberCode && (_languageTemp >> 1) != vCodeTable) { //for remember table code feature
			if (_languageTemp != -1) {
				AppDelegate::getInstance()->onTableCode(_languageTemp >> 1);
			} else {
				saveSmartSwitchKeyData();
			}
		}
		if (vSupportMetroApp && exe.compare("ApplicationFrameHost.exe") == 0) {//Metro App
			SendMessage(HWND_BROADCAST, WM_CHAR, VK_BACK, 0L);
			SendMessage(HWND_BROADCAST, WM_CHAR, VK_BACK, 0L);
		}
	}
	// Log with app name context
	if(PerformanceLogger::isEnabled()) {
		QueryPerformanceCounter(&_perfEnd);
		QueryPerformanceFrequency(&_perfFreq);
		double _ms_app = (double)(_perfEnd.QuadPart - _perfStart.QuadPart) * 1000.0 / _perfFreq.QuadPart;
		if(_ms_app > PERF_LOG_THRESHOLD_MS) {
			char debugTag[128];
			sprintf_s(debugTag, "APP_SWITCH[%s]", appName);
			PerformanceLogger::log(debugTag, _ms_app);
		}
	}
}
