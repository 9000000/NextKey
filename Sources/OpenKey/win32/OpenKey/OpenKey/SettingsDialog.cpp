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
-----------------------------------------------------------*/
#include "stdafx.h"
#include "SettingsDialog.h"
#include "OpenKeyHelper.h"
#include "OpenKeyManager.h"
#include "PerformanceLogger.h"
#include "ConfigManager.h"
#include "SharedState.h"
#include "../../../engine/Engine.h"
#include <shellapi.h>
#include <dwmapi.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>  // For ChooseColor dialog
#include "sciter-x-dom.hpp"
using namespace sciter::dom;  // For ELEMENT_AREAS enum (SELF_RELATIVE, CONTENT_BOX, etc.)
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")  // For ChooseColor

extern int vExcludeApps;  // Defined in AppDelegate.cpp
extern int vShowAdvancedSettings;  // Defined in AppDelegate.cpp
extern int vBackgroundOpacity;  // Defined in AppDelegate.cpp
extern int vEnablePerfLog;  // Defined in AppDelegate.cpp

#define TIMER_RESIZE_WINDOW 1001
#define TIMER_SHAREDSTATE_POLL 1002
#define TIMER_AUTOSAVE 1003
#define AUTOSAVE_INTERVAL_MS 30000  // 30 seconds
#define SHAREDSTATE_POLL_INTERVAL 32  // ~30fps for smooth sync

// Dirty flag for debounced save
static bool s_isDirty = false;

SettingsDialog::SettingsDialog()
	: sciter::window(SW_POPUP | SW_ALPHA | SW_ENABLE_DEBUG, RECT{0, 0, 350, 460}) {
	
	// Load settings from ConfigManager (subprocess starts fresh, reads from config.toml)
	auto& config = ConfigManager::instance();
	config.init();
	
	// General Tab
	vLanguage = config.getInt("general", "language", 1);
	vInputType = config.getInt("general", "inputType", 0);
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
	
	// System Tab (Hệ thống)
	vRunWithWindows = config.getBool("system", "runWithWindows", false) ? 1 : 0;
	vRunAsAdmin = config.getBool("system", "runAsAdmin", false) ? 1 : 0;
	vCheckNewVersion = config.getBool("system", "checkNewVersion", false) ? 1 : 0;
	vCreateDesktopShortcut = config.getBool("system", "createDesktopShortcut", false) ? 1 : 0;
	vSupportMetroApp = config.getBool("system", "supportMetroApp", false) ? 1 : 0;
	vFixChromiumBrowser = config.getBool("system", "fixChromiumBrowser", false) ? 1 : 0;
	vSendKeyStepByStep = config.getBool("system", "useClipboard", true) ? 0 : 1;  // Inverted!
	vUseGrayIcon = config.getInt("system", "iconStyle", 0);  // 0=Color, 1=Dark, 2=Light, 3=Custom
	vShowOnStartUp = config.getBool("system", "showOnStartup", false) ? 1 : 0;
	vShowAdvancedSettings = config.getInt("system", "showAdvancedSettings", 0);
	vBackgroundOpacity = config.getInt("system", "backgroundOpacity", 80);
	
	// Excluded Apps
	vExcludeApps = config.getBool("excludedApps", "enabled", false) ? 1 : 0;
	
	// Debug
	vEnablePerfLog = config.getBool("debug", "enablePerfLog", false) ? 1 : 0;
	
	// Load HTML
#ifdef NDEBUG
	// Release: load from embedded resources (packed by packfolder.exe)
	if (!load(WSTR("this://app/settings/settings.html"))) {
		MessageBoxW(NULL, L"Failed to load settings.html from resources", L"Error", MB_OK | MB_ICONERROR);
		return;
	}
#else
	// Debug: load from file (allows hot-reload during development)
	WCHAR exePath[MAX_PATH];
	GetModuleFileNameW(NULL, exePath, MAX_PATH);
	WCHAR* lastSlash = wcsrchr(exePath, L'\\');
	if (lastSlash) *lastSlash = L'\0';
	
	WCHAR htmlPath[MAX_PATH];
	swprintf_s(htmlPath, MAX_PATH, L"%s\\Resources\\Sciter\\settings\\settings.html", exePath);
	
	if (!load(htmlPath)) {
		MessageBoxW(NULL, htmlPath, L"Failed to load settings.html", MB_OK | MB_ICONERROR);
		return;
	}
#endif

	// Show the window
	expand();
	
	// Subclass window for dragging and close
	SetWindowSubclass(get_hwnd(), SettingsDialog::SubclassProc, 1, (DWORD_PTR)this);
	
	// Set window title for anti-spam detection
	SetWindowTextW(get_hwnd(), L"NextKey Settings");
	
	// Auto-fit window to content size from HTML
	sciter::dom::element rootEl = this->root();
	sciter::dom::element container = rootEl.find_first(".container");
	if (container) {
		// Use get_location for initial compact size
		RECT contentRect = container.get_location(CONTENT_BOX);
		int contentWidth = max(contentRect.right - contentRect.left, 350);
		int contentHeight = max(contentRect.bottom - contentRect.top, 200);
		
		// Resize window to fit content
		SetWindowPos(get_hwnd(), NULL, 0, 0, contentWidth, contentHeight, SWP_NOMOVE | SWP_NOZORDER);
	}
	
	// Center window on screen
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	RECT rc;
	GetWindowRect(get_hwnd(), &rc);
	int winWidth = rc.right - rc.left;
	int winHeight = rc.bottom - rc.top;
	int x = (screenWidth - winWidth) / 2;
	int y = (screenHeight - winHeight) / 2;
	SetWindowPos(get_hwnd(), HWND_NOTOPMOST, x, y, 0, 0, SWP_NOSIZE);  // Remove topmost
	
	// Enable Acrylic blur effect (after resize)
	enableAcrylicEffect();
	
	// Initialize SharedState (subprocess = opens existing, doesn't create)
	if (SharedState::instance().init(false)) {
		// Start polling timer for version-based sync (32ms ~ 30fps)
		SetTimer(get_hwnd(), TIMER_SHAREDSTATE_POLL, SHAREDSTATE_POLL_INTERVAL, NULL);
		m_lastSharedStateVersion = SharedState::instance().getVersion();
		
		// CRITICAL: Override runtime state from SharedState (not config.toml)
		// Config.toml has persistent settings, but runtime state (V/E) may have changed
		vLanguage = SharedState::instance().getLanguage();
		vInputType = SharedState::instance().getInputType();
		vCodeTable = SharedState::instance().getCodeTable();
		LOG(L"[SettingsDialog] Loaded runtime state from SharedState: lang=%d, input=%d, code=%d\n", 
			vLanguage, vInputType, vCodeTable);
	} else {
		LOG(L"[SettingsDialog] SharedState init failed - falling back to message-based sync\n");
	}
	
	// Load settings from registry
	loadSettings();
}

SettingsDialog::~SettingsDialog() {
	if (get_hwnd()) {
		KillTimer(get_hwnd(), TIMER_SHAREDSTATE_POLL);
		RemoveWindowSubclass(get_hwnd(), SettingsDialog::SubclassProc, 1);
	}
}

// --- DWM Acrylic Effect Implementation ---

struct ACCENT_POLICY {
	int AccentState;
	int AccentFlags;
	int GradientColor;  // ABGR format
	int AnimationId;
};

struct WINDOWCOMPOSITIONATTRIBDATA {
	int Attrib;
	void* pvData;
	size_t cbData;
};

enum ACCENT_STATE {
	ACCENT_DISABLED = 0,
	ACCENT_ENABLE_GRADIENT = 1,
	ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
	ACCENT_ENABLE_BLURBEHIND = 3,
	ACCENT_ENABLE_ACRYLICBLURBEHIND = 4,  // Windows 10 1803+
	ACCENT_ENABLE_HOSTBACKDROP = 5         // Windows 11 (Mica)
};

void SettingsDialog::enableAcrylicEffect() {
	HWND hwnd = get_hwnd();

	// 1. CRITICAL: Set WS_EX_LAYERED style first
	SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);

	// 2. Try Acrylic (Windows 10 1803+)
	HMODULE hUser = GetModuleHandle(L"user32.dll");
	if (hUser) {
		typedef BOOL(WINAPI* pSetWindowCompositionAttribute)(HWND, WINDOWCOMPOSITIONATTRIBDATA*);
		auto SetWindowCompositionAttribute = 
			(pSetWindowCompositionAttribute)GetProcAddress(hUser, "SetWindowCompositionAttribute");

		if (SetWindowCompositionAttribute) {
		ACCENT_POLICY policy = { 0 };
			policy.AccentState = ACCENT_ENABLE_BLURBEHIND;  // Use BLURBEHIND (3) instead of ACRYLICBLURBEHIND (4) for smoother dragging on Win10
			policy.AccentFlags = 0;
			policy.GradientColor = 0x00000000;  // Fully transparent - let CSS control background
			policy.AnimationId = 0;

			WINDOWCOMPOSITIONATTRIBDATA data = { 0 };
			data.Attrib = 19;  // WCA_ACCENT_POLICY
			data.pvData = &policy;
			data.cbData = sizeof(policy);

			SetWindowCompositionAttribute(hwnd, &data);
		}
		else {
			// Fallback to DWM Blur
			DWM_BLURBEHIND bb = { 0 };
			bb.dwFlags = DWM_BB_ENABLE;
			bb.fEnable = TRUE;
			bb.hRgnBlur = NULL;
			DwmEnableBlurBehindWindow(hwnd, &bb);
		}
	}

	// 3. Fix corners on Windows 11
	typedef enum {
		DWMWCP_DEFAULT = 0,
		DWMWCP_DONOTROUND = 1,
		DWMWCP_ROUND = 2,
		DWMWCP_ROUNDSMALL = 3
	} DWM_WINDOW_CORNER_PREFERENCE;

	DWM_WINDOW_CORNER_PREFERENCE preference = DWMWCP_ROUND;
	DwmSetWindowAttribute(hwnd, 33, &preference, sizeof(preference));
}

LRESULT CALLBACK SettingsDialog::SubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
	if (msg == WM_CLOSE) {
		// Save if dirty before closing
		KillTimer(hwnd, TIMER_AUTOSAVE);
		if (s_isDirty) {
			ConfigManager::instance().save();
			s_isDirty = false;
			
			// Notify main process to reload from disk (ensure persistence)
			HWND mainWnd = FindWindow(APP_CLASS, NULL);
			if (mainWnd) {
				PostMessage(mainWnd, WM_USER + 101, 0, 0);
			}
		}
		
		// NOTE: Must use ExitProcess(0) for Sciter subprocesses!
		// PostQuitMessage(0) causes Sciter reference counting assertion failure
		ExitProcess(0);
		return 0;
	}
	
	if (msg == WM_NCHITTEST) {
		LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
		if (result == HTCLIENT) {
			POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			ScreenToClient(hwnd, &pt);
			
			// Drag zone: title bar height (36px) for easy dragging
			// Exclude close button (last 40px) and pin button (next 28px) on right side
			RECT winRect;
			GetClientRect(hwnd, &winRect);
			int buttonZone = winRect.right - 68;  // 40px close + 28px pin button
			
			if (pt.y < 36 && pt.x < buttonZone) {
				return HTCAPTION;
			}
		}
		return result;
	}
	
	// Handle realtime language state change from main process (Smart Switch, Excluded Apps)
	// This is a fast path - only updates the V/E toggle, not all settings
	if (msg == GetNextKeyUpdateMsg()) {
		bool isVietnamese = (wParam != 0);
		vLanguage = isVietnamese ? 1 : 0;  // Update local state
		
		SettingsDialog* dialog = reinterpret_cast<SettingsDialog*>(dwRefData);
		if (dialog) {
			sciter::dom::element root = dialog->root();
			sciter::dom::element toggleLang = root.find_first("#toggle-language");
			if (toggleLang) {
				if (!isVietnamese) { // English mode = checked
					toggleLang.set_attribute("class", L"toggle-switch checked");
				} else { // Vietnamese mode = unchecked
					toggleLang.set_attribute("class", L"toggle-switch");
				}
			}
		}
		return 0;
	}
	
	// Handle "bring to foreground" message from main process
	// Main process sends this because cross-process SetForegroundWindow often fails
	// Subprocess brings ITSELF to foreground which is more reliable
	if (msg == WM_USER + 107) {
		return OpenKeyHelper::handleIPCForeground(hwnd);
	}
	
	// Handle notification from main process to update UI (bidirectional sync)
	if (msg == WM_USER + 102) {
		// Reload settings from ConfigManager (main process already saved to disk)
		auto& config = ConfigManager::instance();
		config.load();  // Reload from disk to get latest values
		
		vLanguage = config.getInt("general", "language", 1);
		vInputType = config.getInt("general", "inputType", 0);
		vCodeTable = config.getInt("general", "codeTable", 0);
		vSwitchKeyStatus = config.getInt("general", "switchKey", 0);
		vUseSmartSwitchKey = config.getBool("general", "smartSwitch", false) ? 1 : 0;
		vCheckSpelling = config.getBool("typing", "checkSpelling", true) ? 1 : 0;
		vUseMacro = config.getBool("macro", "enabled", false) ? 1 : 0;
		
		// Get the SettingsDialog instance from dwRefData
		SettingsDialog* dialog = reinterpret_cast<SettingsDialog*>(dwRefData);
		if (dialog) {
			// Update UI by refreshing all elements
			sciter::dom::element root = dialog->root();
			
			// Update language toggle (vLanguage=0 means English/checked)
			sciter::dom::element toggleLang = root.find_first("#toggle-language");
			if (toggleLang) {
				if (vLanguage == 0) { // English mode
					toggleLang.set_attribute("class", L"toggle-switch checked");
				} else { // Vietnamese mode
					toggleLang.set_attribute("class", L"toggle-switch");
				}
			}
			
			// Update dropdowns
			sciter::dom::element inputType = root.find_first("#input-type");
			sciter::dom::element bangMa = root.find_first("#bang-ma");
			if (inputType) inputType.set_value(sciter::value(vInputType));
			if (bangMa) bangMa.set_value(sciter::value(vCodeTable));
			
			// Update key toggles
			sciter::dom::element keyCtrl = root.find_first("#key-ctrl");
			sciter::dom::element keyAlt = root.find_first("#key-alt");
			sciter::dom::element keyWin = root.find_first("#key-win");
			sciter::dom::element keyShift = root.find_first("#key-shift");
			
			if (keyCtrl) {
				if (vSwitchKeyStatus & 0x100) keyCtrl.set_attribute("class", L"toggle-switch-small checked");
				else keyCtrl.set_attribute("class", L"toggle-switch-small");
			}
			if (keyAlt) {
				if (vSwitchKeyStatus & 0x200) keyAlt.set_attribute("class", L"toggle-switch-small checked");
				else keyAlt.set_attribute("class", L"toggle-switch-small");
			}
			if (keyWin) {
				if (vSwitchKeyStatus & 0x400) keyWin.set_attribute("class", L"toggle-switch-small checked");
				else keyWin.set_attribute("class", L"toggle-switch-small");
			}
			if (keyShift) {
				if (vSwitchKeyStatus & 0x800) keyShift.set_attribute("class", L"toggle-switch-small checked");
				else keyShift.set_attribute("class", L"toggle-switch-small");
			}
			
			// Update beep toggle
			sciter::dom::element beep = root.find_first("#beep-sound");
			if (beep) {
				if (vSwitchKeyStatus & 0x8000) beep.set_attribute("class", L"toggle-switch-small checked");
				else beep.set_attribute("class", L"toggle-switch-small");
			}
			
			// Update smart switch toggle
			sciter::dom::element smartSwitch = root.find_first("#smart-switch");
			if (smartSwitch) {
				if (vUseSmartSwitchKey) smartSwitch.set_attribute("class", L"toggle-switch-small checked");
				else smartSwitch.set_attribute("class", L"toggle-switch-small");
			}
			
			// Update exclude apps toggle
			sciter::dom::element excludeApps = root.find_first("#exclude-apps");
			if (excludeApps) {
				if (vExcludeApps) excludeApps.set_attribute("class", L"toggle-switch-small checked");
				else excludeApps.set_attribute("class", L"toggle-switch-small");
			}
		}
		
		return 0;
	}
	
	// Handle timer for window resize after animation
	if (msg == WM_TIMER && wParam == TIMER_RESIZE_WINDOW) {
		KillTimer(hwnd, TIMER_RESIZE_WINDOW);
		SettingsDialog* dialog = reinterpret_cast<SettingsDialog*>(dwRefData);
		if (dialog) {
			dialog->recalcWindowSize();
		}
		return 0;
	}
	
	// Handle SharedState polling timer (version-based change detection)
	if (msg == WM_TIMER && wParam == TIMER_SHAREDSTATE_POLL) {
		SettingsDialog* dialog = reinterpret_cast<SettingsDialog*>(dwRefData);
		if (dialog && SharedState::instance().isValid()) {
			int currentVersion = SharedState::instance().getVersion();
			if (currentVersion != dialog->m_lastSharedStateVersion) {
				dialog->m_lastSharedStateVersion = currentVersion;
				
				// Read updated state from shared memory
				int newLanguage = SharedState::instance().getLanguage();
				
				// Only update UI if language actually changed
				if (newLanguage != vLanguage) {
					vLanguage = newLanguage;
					
					// Update language toggle in UI
					sciter::dom::element root = dialog->root();
					sciter::dom::element toggleLang = root.find_first("#toggle-language");
					if (toggleLang) {
						if (vLanguage == 0) { // English mode
							toggleLang.set_attribute("class", L"toggle-switch checked");
						} else { // Vietnamese mode
							toggleLang.set_attribute("class", L"toggle-switch");
						}
					}
				}
			}
		}
		return 0;
	}

	// Handle autosave timer
	if (msg == WM_TIMER && wParam == TIMER_AUTOSAVE) {
		KillTimer(hwnd, TIMER_AUTOSAVE);
		
		if (s_isDirty) {
			ConfigManager::instance().save();
			s_isDirty = false;
			
			// Notify main process to reload from disk
			HWND mainWnd = FindWindow(APP_CLASS, NULL);
			if (mainWnd) {
				PostMessage(mainWnd, WM_USER + 101, 0, 0);
			}
			// Debug logging
			LOG(L"[SettingsDialog] Autosave triggered after 30s idle\n");
		}
		return 0;
	}
	
	// Handle Windows theme change (real-time dark/light mode sync)
	// WM_SETTINGCHANGE is broadcast when user changes Windows personalization settings
	if (msg == WM_SETTINGCHANGE) {
		// Check if it's a theme-related change
		if (lParam && wcscmp((LPCWSTR)lParam, L"ImmersiveColorSet") == 0) {
			SettingsDialog* dialog = reinterpret_cast<SettingsDialog*>(dwRefData);
			if (dialog) {
				bool isDarkMode = OpenKeyHelper::isWindowsDarkMode();
				// Apply theme via DOM manipulation
				sciter::dom::element root = dialog->root();
				sciter::dom::element body = root.find_first("body");
				if (body) {
					if (isDarkMode) {
						body.set_attribute("class", L"dark");
					} else {
						body.remove_attribute("class");
					}
				}
				// Update container background for proper opacity (was missing!)
				sciter::dom::element container = root.find_first("#main-container");
				if (container) {
					wchar_t bgColor[64];
					double opacity = vBackgroundOpacity / 100.0;
					if (isDarkMode) {
						swprintf_s(bgColor, L"rgba(18, 20, 28, %.2f)", opacity * 0.9);
					} else {
						swprintf_s(bgColor, L"rgba(255, 255, 255, %.2f)", opacity);
					}
					container.set_style_attribute("background-color", bgColor);
				}
			}
		}
		return 0;
	}
	
	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void SettingsDialog::show() {
	ShowWindow(get_hwnd(), SW_SHOW);
	SetForegroundWindow(get_hwnd());
}

void SettingsDialog::loadSettings() {
	// Note: Settings are loaded directly in constructor via handle_event
	// JavaScript handles initial values via HTML
}

void SettingsDialog::saveSettings() {
	// Settings are saved immediately when changed via handle_event
}

// Sync all global settings to ConfigManager cache
// Called before save to ensure all changes are captured
static void syncSettingsToConfig() {
	auto& config = ConfigManager::instance();
	
	// General Tab
	config.setInt("general", "language", vLanguage);
	config.setInt("general", "inputType", vInputType);
	config.setInt("general", "codeTable", vCodeTable);
	config.setInt("general", "switchKey", vSwitchKeyStatus);
	config.setBool("general", "smartSwitch", vUseSmartSwitchKey != 0);
	
	// Typing Tab (Bộ gõ)
	config.setBool("typing", "checkSpelling", vCheckSpelling != 0);
	config.setBool("typing", "restoreWrongSpelling", vRestoreIfWrongSpelling != 0);
	config.setBool("typing", "modernOrthography", vUseModernOrthography != 0);
	config.setBool("typing", "fixRecommendBrowser", vFixRecommendBrowser != 0);
	config.setBool("typing", "upperCaseFirstChar", vUpperCaseFirstChar != 0);
	config.setBool("typing", "allowZwfj", vAllowConsonantZFWJ != 0);
	config.setBool("typing", "tempOffSpellingCtrl", vTempOffSpelling != 0);
	config.setBool("typing", "tempOffOpenKeyAlt", vTempOffOpenKey != 0);
	config.setBool("typing", "rememberCode", vRememberCode != 0);
	
	// Macro Tab (Gõ tắt)
	config.setBool("macro", "enabled", vUseMacro != 0);
	config.setBool("macro", "useInEnglishMode", vUseMacroInEnglishMode != 0);
	config.setBool("macro", "autoCaps", vAutoCapsMacro != 0);
	config.setBool("macro", "quickTelex", vQuickTelex != 0);
	config.setBool("macro", "quickStartConsonant", vQuickStartConsonant != 0);
	config.setBool("macro", "quickEndConsonant", vQuickEndConsonant != 0);
	
	// System Tab (Hệ thống)
	config.setBool("system", "runWithWindows", vRunWithWindows != 0);
	config.setBool("system", "runAsAdmin", vRunAsAdmin != 0);
	config.setBool("system", "checkNewVersion", vCheckNewVersion != 0);
	config.setBool("system", "createDesktopShortcut", vCreateDesktopShortcut != 0);
	config.setBool("system", "supportMetroApp", vSupportMetroApp != 0);
	config.setBool("system", "fixChromiumBrowser", vFixChromiumBrowser != 0);
	config.setBool("system", "useClipboard", vSendKeyStepByStep == 0);  // Inverted!
	config.setInt("system", "iconStyle", vUseGrayIcon);  // 0=Color, 1=Dark, 2=Light, 3=Custom
	config.setInt("system", "customColorV", vTrayIconColorV);
	config.setInt("system", "customColorE", vTrayIconColorE);
	config.setBool("system", "showOnStartup", vShowOnStartUp != 0);
	config.setInt("system", "showAdvancedSettings", vShowAdvancedSettings);
	config.setInt("system", "backgroundOpacity", vBackgroundOpacity);
	
	// Excluded Apps
	config.setBool("excludedApps", "enabled", vExcludeApps != 0);
	
	// Debug
	config.setBool("debug", "enablePerfLog", vEnablePerfLog != 0);
	
	// Convert Tool
	config.setInt("convertTool", "hotkey", convertToolHotKey);
	config.setInt("convertTool", "fromCode", convertToolFromCode);
	config.setInt("convertTool", "toCode", convertToolToCode);
	config.setBool("convertTool", "toAllCaps", convertToolToAllCaps != 0);
	config.setBool("convertTool", "toAllNonCaps", convertToolToAllNonCaps != 0);
	config.setBool("convertTool", "removeMark", convertToolRemoveMark != 0);
	config.setBool("convertTool", "toCapsEachWord", convertToolToCapsEachWord != 0);
	config.setBool("convertTool", "toCapsFirstLetter", convertToolToCapsFirstLetter != 0);
	config.setBool("convertTool", "dontAlertCompleted", convertToolDontAlertWhenCompleted != 0);
}

// Notify main process to sync RAM settings (debounce disk save)
// ONLY use for settings that change rapidly (opacity slider, color picker)
static void notifyMainProcessDebounced() {
	// Sync ALL settings to ConfigManager cache (handles any APP_SET_DATA that was called)
	syncSettingsToConfig();
	
	// Mark as dirty and restart autosave timer
	s_isDirty = true;
	
	HWND hwnd = FindWindow(NULL, L"NextKey Settings");
	if (hwnd) {
		KillTimer(hwnd, TIMER_AUTOSAVE);
		SetTimer(hwnd, TIMER_AUTOSAVE, AUTOSAVE_INTERVAL_MS, NULL);
	}
	
	// Notify main process to sync RAM (no disk reload)
	HWND mainWnd = FindWindow(APP_CLASS, NULL);
	if (mainWnd) {
		PostMessage(mainWnd, WM_USER + 102, 0, 0);
	}
}

// Notify main process and SAVE IMMEDIATELY to disk
// Default for toggle switches and dropdowns (user expects immediate effect)
static void notifyMainProcess() {
	// Sync ALL settings to ConfigManager cache
	syncSettingsToConfig();
	
	// Save immediately to disk
	ConfigManager::instance().save();
	s_isDirty = false;
	
	// Cancel any pending autosave timer
	HWND hwnd = FindWindow(NULL, L"NextKey Settings");
	if (hwnd) {
		KillTimer(hwnd, TIMER_AUTOSAVE);
	}
	
	// Notify main process to reload from disk (WM_USER+101 = full reload)
	HWND mainWnd = FindWindow(APP_CLASS, NULL);
	if (mainWnd) {
		PostMessage(mainWnd, WM_USER + 101, 0, 0);
	}
}

// Notify main process WITHOUT saving to disk (for language toggle - runtime state only)
// Uses SharedState for cross-process sync instead of disk file
static void notifyMainProcessLanguageOnly() {
	// Sync language to SharedState for main process to read
	SharedState::instance().setLanguage(vLanguage);
	
	// Send specific message for language-only update (no disk reload needed)
	// WM_USER+108 = language change from subprocess
	HWND mainWnd = FindWindow(APP_CLASS, NULL);
	if (mainWnd) {
		PostMessage(mainWnd, WM_USER + 108, (WPARAM)vLanguage, 0);
	}
}

bool SettingsDialog::handle_event(HELEMENT he, BEHAVIOR_EVENT_PARAMS& params) {
	// Debug: Log all events with target info for click events
	sciter::dom::element targetEl(params.heTarget);
	std::wstring targetId = targetEl.get_attribute("id");
	std::wstring targetClass = targetEl.get_attribute("class");
	std::string targetTag = targetEl.get_tag();
	

	if (sciter::window::handle_event(he, params))
		return true;
	
	// Handle DOCUMENT_READY to set initial values from registry
	if (params.cmd == DOCUMENT_READY) {

		sciter::dom::element root = this->root();
		
		// Set version dynamically from OpenKeyHelper
		std::wstring versionStr = OpenKeyHelper::getVersionString();
		sciter::dom::element titleText = root.find_first(".title-text");
		sciter::dom::element appVersion = root.find_first(".app-version");
		if (titleText) {
			std::wstring titleVersion = L"NeXTKey v" + versionStr;
			titleText.set_text(titleVersion.c_str());
		}
		if (appVersion) {
			std::wstring aboutVersion = L"Phi\u00EAn b\u1EA3n: " + versionStr + L" (Sciter Edition)";
			appVersion.set_text(aboutVersion.c_str());
		}
		
		// Set dropdown values
		sciter::dom::element inputType = root.find_first("#input-type");
		sciter::dom::element bangMa = root.find_first("#bang-ma");
		
		if (inputType) inputType.set_value(sciter::value(vInputType));
		if (bangMa) bangMa.set_value(sciter::value(vCodeTable));
		
		// Set toggle states by adding/removing 'checked' class (div-based toggles)
		// Language toggle: vLanguage=0 means English (checked), vLanguage=1 means Vietnamese (unchecked)
		sciter::dom::element toggleLang = root.find_first("#toggle-language");
		if (toggleLang) {
			if (vLanguage == 0) { // English mode
				toggleLang.set_attribute("class", L"toggle-switch checked");
			} else { // Vietnamese mode
				toggleLang.set_attribute("class", L"toggle-switch");
			}
		}
		
		// Switch key toggles
		sciter::dom::element keyCtrl = root.find_first("#key-ctrl");
		sciter::dom::element keyAlt = root.find_first("#key-alt");
		sciter::dom::element keyWin = root.find_first("#key-win");
		sciter::dom::element keyShift = root.find_first("#key-shift");
		
		if (keyCtrl) {
			if (vSwitchKeyStatus & 0x100) keyCtrl.set_attribute("class", L"toggle-switch-small checked");
			else keyCtrl.set_attribute("class", L"toggle-switch-small");
		}
		if (keyAlt) {
			if (vSwitchKeyStatus & 0x200) keyAlt.set_attribute("class", L"toggle-switch-small checked");
			else keyAlt.set_attribute("class", L"toggle-switch-small");
		}
		if (keyWin) {
			if (vSwitchKeyStatus & 0x400) keyWin.set_attribute("class", L"toggle-switch-small checked");
			else keyWin.set_attribute("class", L"toggle-switch-small");
		}
		if (keyShift) {
			if (vSwitchKeyStatus & 0x800) keyShift.set_attribute("class", L"toggle-switch-small checked");
			else keyShift.set_attribute("class", L"toggle-switch-small");
		}
		
		// Custom key input - get character from high byte
		sciter::dom::element keyChar = root.find_first("#switch-key-char");
		if (keyChar) {
			int charCode = (vSwitchKeyStatus >> 24) & 0xFF;
			if (charCode > 0) {
				std::wstring charStr;
				if (charCode == 32) {
					charStr = L"Space";  // Display "Space" for space key
				} else {
					charStr = std::wstring(1, (wchar_t)charCode);
				}
				keyChar.set_value(sciter::value(charStr));
			}
		}
		
		// Beep toggle
		sciter::dom::element beep = root.find_first("#beep-sound");
		if (beep) {
			if (vSwitchKeyStatus & 0x8000) beep.set_attribute("class", L"toggle-switch-small checked");
			else beep.set_attribute("class", L"toggle-switch-small");
		}
		
		// Smart switch toggle
		sciter::dom::element smartSwitch = root.find_first("#smart-switch");
		if (smartSwitch) {
			if (vUseSmartSwitchKey) smartSwitch.set_attribute("class", L"toggle-switch-small checked");
			else smartSwitch.set_attribute("class", L"toggle-switch-small");
		}
		
		// Exclude apps toggle
		sciter::dom::element excludeApps = root.find_first("#exclude-apps");
		if (excludeApps) {
			if (vExcludeApps) excludeApps.set_attribute("class", L"toggle-switch-small checked");
			else excludeApps.set_attribute("class", L"toggle-switch-small");
		}
		
		// === Bộ gõ tab toggles ===
		auto setToggleState = [&root](const char* selector, int value) {
			sciter::dom::element el = root.find_first(selector);
			if (el) {
				if (value) el.set_attribute("class", L"toggle-switch-small checked");
				else el.set_attribute("class", L"toggle-switch-small");
			}
		};
		
		setToggleState("#modern-ortho", vUseModernOrthography);
		setToggleState("#fix-recommend", vFixRecommendBrowser);
		setToggleState("#auto-caps", vUpperCaseFirstChar);
		setToggleState("#remember-code", vRememberCode);
		setToggleState("#spell-check", vCheckSpelling);
		setToggleState("#restore-key", vRestoreIfWrongSpelling);
		setToggleState("#allow-zwjf", vAllowConsonantZFWJ);
		setToggleState("#temp-off-spell", vTempOffSpelling);
		setToggleState("#temp-off-openkey", vTempOffOpenKey);
		
		// Gõ tắt tab toggles
		setToggleState("#use-macro", vUseMacro);
		setToggleState("#macro-english", vUseMacroInEnglishMode);
		setToggleState("#auto-caps-macro", vAutoCapsMacro);
		setToggleState("#quick-telex", vQuickTelex);
		setToggleState("#quick-start", vQuickStartConsonant);
		setToggleState("#quick-end", vQuickEndConsonant);
		
		// Hệ thống (System) tab toggles
		setToggleState("#metro-support", vSupportMetroApp);
		setToggleState("#desktop-shortcut", vCreateDesktopShortcut);
		setToggleState("#run-startup", vRunWithWindows);
		setToggleState("#show-on-startup", vShowOnStartUp);
		setToggleState("#check-update", vCheckNewVersion);
		// Set modern icon dropdown value (0=Color, 1=Dark, 2=Light, 3=Custom)
		sciter::dom::element modernIcon = root.find_first("#modern-icon");
		if (modernIcon) modernIcon.set_value(sciter::value(vUseGrayIcon));
		
		// Show custom color row if Custom mode (value=3) is selected
		sciter::dom::element colorRow = root.find_first("#custom-color-row");
		if (colorRow) {
			colorRow.set_style_attribute("display", vUseGrayIcon == 3 ? L"flex" : L"none");
		}
		
		// Auto-save default colors if Custom mode is already selected but colors not set
		// This ensures custom icons work immediately when dialog opens
		if (vUseGrayIcon == 3) {
			auto& config = ConfigManager::instance();
			COLORREF colorV = (COLORREF)config.getInt("ui", "tray_icon_color_v", 0);
			COLORREF colorE = (COLORREF)config.getInt("ui", "tray_icon_color_e", 0);
			bool needsNotify = false;
			
			if (colorV == 0) {
				vTrayIconColorV = TRAY_DEFAULT_COLOR_V; 
				needsNotify = true;
			}
			if (colorE == 0) {
				vTrayIconColorE = TRAY_DEFAULT_COLOR_E;
				needsNotify = true;
			}
			if (needsNotify) {
				notifyMainProcess();
			}
		}
		
		// Set custom icon color swatch buttons from saved values
		{
			// Load colors from ConfigManager (not Registry)
			auto& config = ConfigManager::instance();
			COLORREF colorV = (COLORREF)config.getInt("ui", "tray_icon_color_v", 0);
			COLORREF colorE = (COLORREF)config.getInt("ui", "tray_icon_color_e", 0);
			
			// Convert COLORREF to rgb() format for CSS
			auto colorrefToRgb = [](COLORREF color, COLORREF defaultColor) -> std::wstring {
				if (color == 0) color = defaultColor;
				wchar_t rgb[32];
				swprintf_s(rgb, L"rgb(%d,%d,%d)", GetRValue(color), GetGValue(color), GetBValue(color));
				return rgb;
			};
			
			sciter::dom::element btnV = root.find_first("#btn-color-v");
			sciter::dom::element btnE = root.find_first("#btn-color-e");
			
			if (btnV) {
				std::wstring rgbV = colorrefToRgb(colorV, TRAY_DEFAULT_COLOR_V);  // RGB(243,98,103) - Pink
				btnV.set_style_attribute("background-color", rgbV.c_str());
			}
			if (btnE) {
				std::wstring rgbE = colorrefToRgb(colorE, TRAY_DEFAULT_COLOR_E);  // RGB(47,175,218) - Blue
				btnE.set_style_attribute("background-color", rgbE.c_str());
			}
		}
		
		setToggleState("#chromium-fix", vFixChromiumBrowser);
		setToggleState("#run-admin", vRunAsAdmin);
		setToggleState("#use-clipboard", !vSendKeyStepByStep);  // clipboard = NOT step-by-step
		
		// Show advanced settings toggle
		setToggleState("#show-advanced", vShowAdvancedSettings);
		
		// Performance logging toggle
		setToggleState("#perf-log", vEnablePerfLog);
		
		// Set custom opacity slider position via DOM
		wchar_t percentStr[16];
		swprintf_s(percentStr, L"%d%%", vBackgroundOpacity);
		
		sciter::dom::element thumb = root.find_first("#bg-opacity-thumb");
		if (thumb) {
			thumb.set_style_attribute("left", percentStr);
		}
		sciter::dom::element fill = root.find_first("#bg-opacity-fill");
		if (fill) {
			fill.set_style_attribute("width", percentStr);
		}
		sciter::dom::element opacityLabel = root.find_first("#bg-opacity-value");
		if (opacityLabel) {
			opacityLabel.set_text(percentStr);
		}
		// Apply background opacity to container
		sciter::dom::element mainContainer = root.find_first("#main-container");
		if (mainContainer) {
			double opacity = vBackgroundOpacity / 100.0;
			wchar_t colorStr[64];
			swprintf_s(colorStr, L"rgba(255, 255, 255, %.2f)", opacity);
			mainContainer.set_style_attribute("background-color", colorStr);
		}
		
		// Auto-expand advanced section if saved preference is ON
		if (vShowAdvancedSettings) {
			sciter::dom::element container = root.find_first("#main-container");
			if (container) {
				container.set_attribute("class", L"container expanded");
				m_isExpanded = true;
				// Resize window after content is rendered
				SetTimer(get_hwnd(), TIMER_RESIZE_WINDOW, 100, NULL);
			}
		}
		
		// Apply Windows dark/light theme via DOM manipulation
		bool isDarkMode = OpenKeyHelper::isWindowsDarkMode();
		sciter::dom::element body = root.find_first("body");
		if (body) {
			if (isDarkMode) {
				body.set_attribute("class", L"dark");
			} else {
				body.remove_attribute("class");
			}
		}
		// Also update container background for proper opacity
		sciter::dom::element container = root.find_first("#main-container");
		if (container) {
			wchar_t bgColor[64];
			double opacity = vBackgroundOpacity / 100.0;
			if (isDarkMode) {
				// Deep dark blue-gray for glass effect (matches CSS: rgba(18, 20, 28))
				swprintf_s(bgColor, L"rgba(18, 20, 28, %.2f)", opacity * 0.9);
			} else {
				swprintf_s(bgColor, L"rgba(255, 255, 255, %.2f)", opacity);
			}
			container.set_style_attribute("background-color", bgColor);
		}
		
		return true;
	}
	
	// Handle button clicks
	if (params.cmd == BUTTON_CLICK) {
		sciter::dom::element el(params.heTarget);
		std::wstring id = el.get_attribute("id");
		std::wstring className = el.get_attribute("class");
		std::string tagName = el.get_tag();
		

		if (id == L"btn-close") {
			// Close the window
			PostMessage(get_hwnd(), WM_CLOSE, 0, 0);
			return true;
		}
		
		if (id == L"btn-pin") {
			// Toggle always-on-top (pin) state
			m_isPinned = !m_isPinned;
			
			// Update window Z-order
			SetWindowPos(get_hwnd(), m_isPinned ? HWND_TOPMOST : HWND_NOTOPMOST, 
				0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			
			// Update button class to show correct icon
			sciter::dom::element root = this->root();
			sciter::dom::element pinBtn = root.find_first("#btn-pin");
			if (pinBtn) {
				if (m_isPinned) {
					pinBtn.set_attribute("class", L"btn-pin pinned");
				} else {
					pinBtn.set_attribute("class", L"btn-pin");
				}
			}
			return true;
		}
		
		if (id == L"btn-macro-table") {
			// First check if Macro window already exists - focus it directly
			// Settings dialog has foreground privileges so SetForegroundWindow works
			HWND existingWnd = FindWindowW(NULL, L"Gõ tắt");
			if (existingWnd) {
				SetWindowPos(existingWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
				SetForegroundWindow(existingWnd);
				return true;
			}
			// Window doesn't exist - ask main process to spawn it
			HWND mainWnd = FindWindow(APP_CLASS, NULL);
			if (mainWnd) {
				PostMessage(mainWnd, WM_USER + 103, 0, 0);
			}
			return true;
		}
		
		if (id == L"btn-excluded-apps") {
			// First check if Excluded Apps window already exists - focus it directly
			HWND existingWnd = FindWindowW(NULL, L"\u1EE8ng d\u1EE5ng lo\u1EA1i tr\u1EEB");
			if (existingWnd) {
				SetWindowPos(existingWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
				SetForegroundWindow(existingWnd);
				return true;
			}
			// Window doesn't exist - ask main process to spawn it
			HWND mainWnd = FindWindow(APP_CLASS, NULL);
			if (mainWnd) {
				PostMessage(mainWnd, WM_USER + 104, 0, 0);
			}
			return true;
		}
		
		if (id == L"btn-special-apps") {
			// First check if Special Apps window already exists - focus it directly
			HWND existingWnd = FindWindowW(NULL, L"\u1EE8ng d\u1EE5ng \u0111\u1EB7c bi\u1EC7t");
			if (existingWnd) {
				SetWindowPos(existingWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
				SetForegroundWindow(existingWnd);
				return true;
			}
			// Window doesn't exist - ask main process to spawn it
			HWND mainWnd = FindWindow(APP_CLASS, NULL);
			if (mainWnd) {
				PostMessage(mainWnd, WM_USER + 106, 0, 0);
			}
			return true;
		}
		
		if (id == L"btn-clipboard-apps") {
			// First check if Clipboard Apps window already exists - focus it directly
			HWND existingWnd = FindWindowW(NULL, L"C\u1EA5u h\u00ECnh Clipboard");
			if (existingWnd) {
				SetWindowPos(existingWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
				SetForegroundWindow(existingWnd);
				return true;
			}
			// Window doesn't exist - ask main process to spawn it
			HWND mainWnd = FindWindow(APP_CLASS, NULL);
			if (mainWnd) {
				PostMessage(mainWnd, WM_USER + 109, 0, 0);  // WM_USER+109 = spawn ClipboardAppsDialog
			}
			return true;
		}
		
		// Handle color swatch V button - open Windows color picker
		if (id == L"btn-color-v") {
			auto& cfg = ConfigManager::instance();
			COLORREF currentColor = (COLORREF)cfg.getInt("ui", "tray_icon_color_v", TRAY_DEFAULT_COLOR_V);
			static COLORREF acrCustClr[16] = {0};  // Custom colors storage
			
			CHOOSECOLOR cc = {0};
			cc.lStructSize = sizeof(cc);
			cc.hwndOwner = get_hwnd();
			cc.lpCustColors = acrCustClr;
			cc.rgbResult = currentColor;
			cc.Flags = CC_FULLOPEN | CC_RGBINIT;
			
			if (ChooseColor(&cc)) {
				vTrayIconColorV = cc.rgbResult;
				
				// Update button background color
				sciter::dom::element root_el = this->root();
				sciter::dom::element btn = root_el.find_first("#btn-color-v");
				if (btn) {
					wchar_t colorStr[32];
					swprintf_s(colorStr, L"rgb(%d,%d,%d)", 
						GetRValue(cc.rgbResult), GetGValue(cc.rgbResult), GetBValue(cc.rgbResult));
					btn.set_style_attribute("background-color", colorStr);
				}
				
				notifyMainProcess();
			}
			return true;
		}
		
		// Handle color swatch E button - open Windows color picker
		if (id == L"btn-color-e") {
			auto& cfg = ConfigManager::instance();
			COLORREF currentColor = (COLORREF)cfg.getInt("ui", "tray_icon_color_e", TRAY_DEFAULT_COLOR_E);
			static COLORREF acrCustClr[16] = {0};  // Custom colors storage
			
			CHOOSECOLOR cc = {0};
			cc.lStructSize = sizeof(cc);
			cc.hwndOwner = get_hwnd();
			cc.lpCustColors = acrCustClr;
			cc.rgbResult = currentColor;
			cc.Flags = CC_FULLOPEN | CC_RGBINIT;
			
			if (ChooseColor(&cc)) {
				vTrayIconColorE = cc.rgbResult;
				
				// Update button background color
				sciter::dom::element root_el = this->root();
				sciter::dom::element btn = root_el.find_first("#btn-color-e");
				if (btn) {
					wchar_t colorStr[32];
					swprintf_s(colorStr, L"rgb(%d,%d,%d)", 
						GetRValue(cc.rgbResult), GetGValue(cc.rgbResult), GetBValue(cc.rgbResult));
					btn.set_style_attribute("background-color", colorStr);
				}
				
				notifyMainProcess();
			}
			return true;
		}
		
		// Handle reset colors button
		if (id == L"btn-reset-colors") {
			vTrayIconColorV = 0;  // 0 means use default
			vTrayIconColorE = 0;
			
			// Reset button backgrounds to default colors
			sciter::dom::element root_el = this->root();
			sciter::dom::element btnV = root_el.find_first("#btn-color-v");
			sciter::dom::element btnE = root_el.find_first("#btn-color-e");
			if (btnV) btnV.set_style_attribute("background-color", L"#F36267");
			if (btnE) btnE.set_style_attribute("background-color", L"#2FAFDA");
			
			notifyMainProcess();
			return true;
		}
		
		// Handle Open Log Folder button
		if (id == L"btn-open-log-folder") {
			PerformanceLogger::openLogFolder();
			return true;
		}
		
		// Handle Check Update button - send message to main process
		if (id == L"btn-check-update") {
			HWND mainWnd = FindWindow(APP_CLASS, NULL);
			if (mainWnd) {
				PostMessage(mainWnd, WM_USER + 105, 0, 0);  // Custom message for manual update check
			}
			return true;
		}
		
		// Note: Advanced settings is now handled by toggle switch #show-advanced
		// via VALUE_CHANGED handler for val-show-advanced
	}
	// Handle VALUE_CHANGED for dropdowns AND checkboxes
	else if (params.cmd == VALUE_CHANGED) {
		sciter::dom::element el(params.heTarget);
		std::wstring id = el.get_attribute("id");
		

		
		// Handle dropdowns by ID
		if (id == L"input-type") {
			sciter::value val = el.get_value();
			int value = 0;
			if (val.is_int()) value = val.get<int>();
			else if (val.is_string()) value = _wtoi(val.get<std::wstring>().c_str());
			onInputTypeChange(value);
			return true;
		}
		else if (id == L"bang-ma") {
			sciter::value val = el.get_value();
			int value = 0;
			if (val.is_int()) value = val.get<int>();
			else if (val.is_string()) value = _wtoi(val.get<std::wstring>().c_str());
			onCodeTableChange(value);
			return true;
		}
		else if (id == L"switch-key-char") {
			// Handle custom switch key character input
			sciter::value val = el.get_value();
			if (val.is_string()) {
				std::wstring str = val.get<std::wstring>();
				if (str.length() > 0) {
					int charCode;
					// UI displays "Space" but we need actual space char (32)
					if (str == L"Space") {
						charCode = 32;  // Space character
					} else {
						charCode = (int)str[0];
					}
					// Convert character to virtual key code for proper matching
					// The engine uses GET_SWITCH_KEY(data) = (data & 0xFF) to check keycode
					// We need to store the VK code in low byte and char in high byte
					SHORT vkResult = VkKeyScanW((WCHAR)charCode);
					BYTE vkCode = LOBYTE(vkResult);  // Get virtual key code
					
					// Store: low byte = VK code (for engine matching), high byte = char (for UI display)
					vSwitchKeyStatus &= 0x00FFFF00;  // Clear both low byte and high byte
					vSwitchKeyStatus |= vkCode;      // Set low byte to VK code
					vSwitchKeyStatus |= ((unsigned int)charCode << 24);  // Set high byte to char for display
					APP_SET_DATA(vSwitchKeyStatus, vSwitchKeyStatus);
					notifyMainProcess();
				} else {
					// Empty string - clear custom key (0xFE means no additional key)
					vSwitchKeyStatus &= 0x00FFFF00;  // Clear both low byte and high byte
					vSwitchKeyStatus |= 0xFE;        // Special value: no custom key
					APP_SET_DATA(vSwitchKeyStatus, vSwitchKeyStatus);
					notifyMainProcess();
				}
			}
			return true;
		}
		// Handle hidden inputs (toggle values with val- prefix)
		else if (id == L"val-toggle-language") {
			sciter::value val = el.get_value();
			std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
			bool checked = (strVal == L"1");

			vLanguage = checked ? 0 : 1;  // checked = English (0)
			APP_SET_DATA(vLanguage, vLanguage);
			notifyMainProcessLanguageOnly();  // Don't save disk for language toggle
			return true;
		}
		else if (id == L"val-key-ctrl") {
			sciter::value val = el.get_value();
			std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
			bool checked = (strVal == L"1");
			if (checked) vSwitchKeyStatus |= 0x100;
			else vSwitchKeyStatus &= ~0x100;
			APP_SET_DATA(vSwitchKeyStatus, vSwitchKeyStatus);
			notifyMainProcess();
			return true;
		}
		else if (id == L"val-key-alt") {
			sciter::value val = el.get_value();
			std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
			bool checked = (strVal == L"1");
			if (checked) vSwitchKeyStatus |= 0x200;
			else vSwitchKeyStatus &= ~0x200;
			APP_SET_DATA(vSwitchKeyStatus, vSwitchKeyStatus);
			notifyMainProcess();
			return true;
		}
		else if (id == L"val-key-win") {
			sciter::value val = el.get_value();
			std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
			bool checked = (strVal == L"1");
			if (checked) vSwitchKeyStatus |= 0x400;
			else vSwitchKeyStatus &= ~0x400;
			APP_SET_DATA(vSwitchKeyStatus, vSwitchKeyStatus);
			notifyMainProcess();
			return true;
		}
		else if (id == L"val-key-shift") {
			sciter::value val = el.get_value();
			std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
			bool checked = (strVal == L"1");
			if (checked) vSwitchKeyStatus |= 0x800;
			else vSwitchKeyStatus &= ~0x800;
			APP_SET_DATA(vSwitchKeyStatus, vSwitchKeyStatus);
			notifyMainProcess();
			return true;
		}
		else if (id == L"val-beep-sound") {
			sciter::value val = el.get_value();
			std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
			bool checked = (strVal == L"1");
			if (checked) vSwitchKeyStatus |= 0x8000;
			else vSwitchKeyStatus &= ~0x8000;
			APP_SET_DATA(vSwitchKeyStatus, vSwitchKeyStatus);
			notifyMainProcess();
			return true;
		}
		else if (id == L"val-smart-switch") {
			sciter::value val = el.get_value();
			std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
			bool checked = (strVal == L"1");
			vUseSmartSwitchKey = checked ? 1 : 0;
			APP_SET_DATA(vUseSmartSwitchKey, vUseSmartSwitchKey);
			notifyMainProcess();
			return true;
		}
		else if (id == L"val-exclude-apps") {
			sciter::value val = el.get_value();
			std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
			bool checked = (strVal == L"1");
			vExcludeApps = checked ? 1 : 0;
			APP_SET_DATA(vExcludeApps, vExcludeApps);
			notifyMainProcess();
			return true;
		}
		else if (id == L"val-show-advanced") {
			sciter::value val = el.get_value();
			std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
			bool checked = (strVal == L"1");

			// Save preference to registry
			vShowAdvancedSettings = checked ? 1 : 0;
			APP_SET_DATA(vShowAdvancedSettings, vShowAdvancedSettings);
			notifyMainProcess();  // Save to config.toml
			
			// Toggle expanded class on container
			sciter::dom::element root = this->root();
			sciter::dom::element container = root.find_first("#main-container");
			if (container) {
				container.set_attribute("class", checked ? L"container expanded" : L"container");
			}
			m_isExpanded = checked;
			
			// Resize window immediately (no animation)
			recalcWindowSize();
			return true;
		}
		else if (id == L"val-tab-change") {
			// Tab switched in advanced section - recalculate window size
			if (m_isExpanded) {
				// Small delay to let tab content render
				SetTimer(get_hwnd(), TIMER_RESIZE_WINDOW, 50, NULL);
			}
			return true;
		}
		// === Bộ gõ tab VALUE_CHANGED handlers ===
		else if (id == L"val-modern-ortho") {
			sciter::value val = el.get_value();
			std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
			vUseModernOrthography = (strVal == L"1") ? 1 : 0;
			APP_SET_DATA(vUseModernOrthography, vUseModernOrthography);
			notifyMainProcess();
			return true;
		}
		else if (id == L"val-fix-recommend") {
			sciter::value val = el.get_value();
			std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
			vFixRecommendBrowser = (strVal == L"1") ? 1 : 0;
			APP_SET_DATA(vFixRecommendBrowser, vFixRecommendBrowser);
			notifyMainProcess();
			return true;
		}
		else if (id == L"val-auto-caps") {
			sciter::value val = el.get_value();
			std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
			vUpperCaseFirstChar = (strVal == L"1") ? 1 : 0;
			APP_SET_DATA(vUpperCaseFirstChar, vUpperCaseFirstChar);
			notifyMainProcess();
			return true;
		}
		else if (id == L"val-remember-code") {
			sciter::value val = el.get_value();
			std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
			vRememberCode = (strVal == L"1") ? 1 : 0;
			APP_SET_DATA(vRememberCode, vRememberCode);
			notifyMainProcess();
			return true;
		}
		else if (id == L"val-spell-check") {
			sciter::value val = el.get_value();
			std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
			vCheckSpelling = (strVal == L"1") ? 1 : 0;
			APP_SET_DATA(vCheckSpelling, vCheckSpelling);
			notifyMainProcess();
			return true;
		}
		else if (id == L"val-restore-key") {
			sciter::value val = el.get_value();
			std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
			vRestoreIfWrongSpelling = (strVal == L"1") ? 1 : 0;
			APP_SET_DATA(vRestoreIfWrongSpelling, vRestoreIfWrongSpelling);
			notifyMainProcess();
			return true;
		}
		else if (id == L"val-allow-zwjf") {
			sciter::value val = el.get_value();
			std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
			vAllowConsonantZFWJ = (strVal == L"1") ? 1 : 0;
			APP_SET_DATA(vAllowConsonantZFWJ, vAllowConsonantZFWJ);
			notifyMainProcess();
			return true;
		}
		else if (id == L"val-temp-off-spell") {
			sciter::value val = el.get_value();
			std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
			vTempOffSpelling = (strVal == L"1") ? 1 : 0;
			APP_SET_DATA(vTempOffSpelling, vTempOffSpelling);
			notifyMainProcess();
			return true;
		}
		else if (id == L"val-temp-off-openkey") {
			sciter::value val = el.get_value();
			std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
			vTempOffOpenKey = (strVal == L"1") ? 1 : 0;
			APP_SET_DATA(vTempOffOpenKey, vTempOffOpenKey);
			notifyMainProcess();
			return true;
		}
		// === Gõ tắt tab VALUE_CHANGED handlers ===
		else if (id == L"val-use-macro") {
			sciter::value val = el.get_value();
			std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
			vUseMacro = (strVal == L"1") ? 1 : 0;
			APP_SET_DATA(vUseMacro, vUseMacro);
			notifyMainProcess();
			return true;
		}
		else if (id == L"val-macro-english") {
			sciter::value val = el.get_value();
			std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
			vUseMacroInEnglishMode = (strVal == L"1") ? 1 : 0;
			APP_SET_DATA(vUseMacroInEnglishMode, vUseMacroInEnglishMode);
			notifyMainProcess();
			return true;
		}
		else if (id == L"val-auto-caps-macro") {
			sciter::value val = el.get_value();
			std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
			vAutoCapsMacro = (strVal == L"1") ? 1 : 0;
			APP_SET_DATA(vAutoCapsMacro, vAutoCapsMacro);
			notifyMainProcess();
			return true;
		}
		else if (id == L"val-quick-telex") {
			sciter::value val = el.get_value();
			std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
			vQuickTelex = (strVal == L"1") ? 1 : 0;
			APP_SET_DATA(vQuickTelex, vQuickTelex);
			notifyMainProcess();
			return true;
		}
		else if (id == L"val-quick-start") {
			sciter::value val = el.get_value();
			std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
			vQuickStartConsonant = (strVal == L"1") ? 1 : 0;
			APP_SET_DATA(vQuickStartConsonant, vQuickStartConsonant);
			notifyMainProcess();
			return true;
		}
		else if (id == L"val-quick-end") {
			sciter::value val = el.get_value();
			std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
			vQuickEndConsonant = (strVal == L"1") ? 1 : 0;
			APP_SET_DATA(vQuickEndConsonant, vQuickEndConsonant);
			notifyMainProcess();
			return true;
		}
		// === Hệ thống (System) tab VALUE_CHANGED handlers ===
		else if (id == L"val-metro-support") {
			sciter::value val = el.get_value();
			std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
			vSupportMetroApp = (strVal == L"1") ? 1 : 0;
			APP_SET_DATA(vSupportMetroApp, vSupportMetroApp);
			notifyMainProcess();

			return true;
		}
		else if (id == L"val-desktop-shortcut") {
			sciter::value val = el.get_value();
			std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
			vCreateDesktopShortcut = (strVal == L"1") ? 1 : 0;
			APP_SET_DATA(vCreateDesktopShortcut, vCreateDesktopShortcut);
			// Create or delete shortcut based on toggle state
			if (vCreateDesktopShortcut) {
				OpenKeyManager::createDesktopShortcut();
			} else {
				OpenKeyManager::deleteDesktopShortcut();
			}
			notifyMainProcess();

			return true;
		}
		else if (id == L"val-run-startup") {
			sciter::value val = el.get_value();
			std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
			vRunWithWindows = (strVal == L"1") ? 1 : 0;
			APP_SET_DATA(vRunWithWindows, vRunWithWindows);
			OpenKeyHelper::registerRunOnStartup(vRunWithWindows);

			return true;
		}
		else if (id == L"val-show-on-startup") {
			sciter::value val = el.get_value();
			std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
			vShowOnStartUp = (strVal == L"1") ? 1 : 0;
			APP_SET_DATA(vShowOnStartUp, vShowOnStartUp);
			notifyMainProcess();  // Save to config.toml
			return true;
		}
		else if (id == L"val-check-update") {
			sciter::value val = el.get_value();
			std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
			vCheckNewVersion = (strVal == L"1") ? 1 : 0;
			APP_SET_DATA(vCheckNewVersion, vCheckNewVersion);
			notifyMainProcess();
			return true;
		}
		else if (id == L"modern-icon") {
			sciter::value val = el.get_value();
			int value = 0;
			if (val.is_int()) value = val.get<int>();
			else if (val.is_string()) value = _wtoi(val.get<std::wstring>().c_str());
			vUseGrayIcon = value;  // 0=Color, 1=White, 2=Black, 3=Custom
			APP_SET_DATA(vUseGrayIcon, vUseGrayIcon);
			
			// When switching to Custom mode, auto-save default colors if not set
			// This ensures the custom color condition (colorV != 0 || colorE != 0) is met
			if (value == 3) {
				auto& cfg = ConfigManager::instance();
				COLORREF colorV = (COLORREF)cfg.getInt("ui", "tray_icon_color_v", 0);
				COLORREF colorE = (COLORREF)cfg.getInt("ui", "tray_icon_color_e", 0);
				
				if (colorV == 0) {
					vTrayIconColorV = TRAY_DEFAULT_COLOR_V;  // RGB(243,98,103) - Pink for V
				}
				if (colorE == 0) {
					vTrayIconColorE = TRAY_DEFAULT_COLOR_E;  // RGB(47,175,218) - Blue for E
				}
			}
			
			notifyMainProcess();

			return true;
		}
		else if (id == L"val-chromium-fix") {
			sciter::value val = el.get_value();
			std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
			vFixChromiumBrowser = (strVal == L"1") ? 1 : 0;
			APP_SET_DATA(vFixChromiumBrowser, vFixChromiumBrowser);
			notifyMainProcess();

			return true;
		}
		else if (id == L"val-run-admin") {
			sciter::value val = el.get_value();
			std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
			vRunAsAdmin = (strVal == L"1") ? 1 : 0;
			APP_SET_DATA(vRunAsAdmin, vRunAsAdmin);
			if (vRunWithWindows) {
				OpenKeyHelper::registerRunOnStartup(vRunWithWindows);
			}

			return true;
		}
		else if (id == L"val-use-clipboard") {
			sciter::value val = el.get_value();
			std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
			// checked = use clipboard, so vSendKeyStepByStep = 0
			vSendKeyStepByStep = (strVal == L"1") ? 0 : 1;
			APP_SET_DATA(vSendKeyStepByStep, vSendKeyStepByStep);
			notifyMainProcess();

			return true;
		}
		else if (id == L"val-bg-opacity") {
			sciter::value val = el.get_value();
			std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"80";
			vBackgroundOpacity = _wtoi(strVal.c_str());
			if (vBackgroundOpacity < 0) vBackgroundOpacity = 0;
			if (vBackgroundOpacity > 100) vBackgroundOpacity = 100;
			APP_SET_DATA(vBackgroundOpacity, vBackgroundOpacity);
			notifyMainProcessDebounced();  // Use debounce - slider changes rapidly during drag
			return true;
		}
		// Performance logging toggle
		else if (id == L"val-perf-log") {
			sciter::value val = el.get_value();
			std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
			vEnablePerfLog = (strVal == L"1") ? 1 : 0;
			APP_SET_DATA(vEnablePerfLog, vEnablePerfLog);
			PerformanceLogger::setEnabled(vEnablePerfLog != 0);
			notifyMainProcess();
			return true;
		}
	}
	
	// Handle HYPERLINK_CLICK events for toggle switches
	// Note: JavaScript already toggles the 'checked' class, we just need to detect the change
	// Note: Advanced settings toggle (show-advanced) is handled via VALUE_CHANGED
	if (params.cmd == HYPERLINK_CLICK || params.cmd == BUTTON_CLICK) {
		sciter::dom::element el(params.heTarget);
		std::wstring elId = el.get_attribute("id");
		std::wstring className = el.get_attribute("class");
		
		// Check if this is a toggle element (has toggle-switch or toggle-switch-small class)
		bool isToggle = (!className.empty() && (
			className.find(L"toggle-switch") != std::wstring::npos ||
			className.find(L"toggle-thumb") != std::wstring::npos
		));
		
		if (isToggle) {
			// Find the actual toggle container (might have clicked on thumb)
			sciter::dom::element toggleEl = el;
			if (className.find(L"thumb") != std::wstring::npos) {
				toggleEl = el.parent();  // Get parent toggle-switch element
			}
			
			std::wstring id = toggleEl.get_attribute("id");
			if (id.empty()) return false;
			
			// After JS click handler runs, the class will be toggled
			// We need to check the CURRENT state (after toggle)
			std::wstring toggleClass = toggleEl.get_attribute("class");
			bool isChecked = (toggleClass.find(L"checked") != std::wstring::npos);
			
			// === COMPACT SECTION TOGGLES ===
			// All compact section toggles are handled via val-* VALUE_CHANGED handlers
			// Do not add duplicate handling here to avoid double-toggle issues
			// (especially for toggle-language which has inverting logic: checked = English = 0)
			// === TAB 3: SYSTEM SETTINGS (Hệ thống) ===
			// All System tab toggles are handled via val-* VALUE_CHANGED handlers
			// Do not add duplicate handling here to avoid double-toggle issues
			// (especially for toggles with inverting logic like modern-icon, use-clipboard)
		}
		
		// Handle Reset Settings button
		if (elId == L"btn-reset-settings") {
			// "Bạn có chắc muốn đặt lại tất cả cài đặt về mặc định?\n\nỨng dụng sẽ tự đóng sau khi reset."
			int result = MessageBoxW(get_hwnd(), 
				L"B\u1EA1n c\u00F3 ch\u1EAFc mu\u1ED1n \u0111\u1EB7t l\u1EA1i t\u1EA5t c\u1EA3 c\u00E0i \u0111\u1EB7t v\u1EC1 m\u1EB7c \u0111\u1ECBnh?\n\n\u1EE8ng d\u1EE5ng s\u1EBD t\u1EF1 \u0111\u00F3ng sau khi reset.",
				L"X\u00E1c nh\u1EADn Reset",  // "Xác nhận Reset"
				MB_YESNO | MB_ICONWARNING);
			
			if (result == IDYES) {
				OpenKeyHelper::resetAllSettings();
				// "Đã đặt lại cài đặt thành công!\n\nVui lòng khởi động lại ứng dụng."
				MessageBoxW(get_hwnd(), 
					L"\u0110\u00E3 \u0111\u1EB7t l\u1EA1i c\u00E0i \u0111\u1EB7t th\u00E0nh c\u00F4ng!\n\nVui l\u00F2ng kh\u1EDFi \u0111\u1ED9ng l\u1EA1i \u1EE9ng d\u1EE5ng.",
					L"Ho\u00E0n t\u1EA5t",  // "Hoàn tất"
					MB_OK | MB_ICONINFORMATION);
				// Hard exit since settings were reset - user must restart
				// Using ExitProcess here is intentional as we want to force restart
				ExitProcess(0);
			}
			return true;
		}
	}
	
	return false;
}

void SettingsDialog::onLanguageToggle(bool isEnglish) {

	vLanguage = isEnglish ? 0 : 1;
	APP_SET_DATA(vLanguage, vLanguage);
	notifyMainProcess();
}

void SettingsDialog::onInputTypeChange(int value) {
	vInputType = value;
	APP_SET_DATA(vInputType, vInputType);
	SharedState::instance().setInputType(vInputType);  // Update SharedState for cross-process sync
	notifyMainProcess();
}

void SettingsDialog::onCodeTableChange(int value) {
	vCodeTable = value;
	APP_SET_DATA(vCodeTable, vCodeTable);
	SharedState::instance().setCodeTable(vCodeTable);  // Update SharedState for cross-process sync
	notifyMainProcess();
}

void SettingsDialog::onSwitchKeyCtrlChange(bool enabled) {
	if (enabled) {
		vSwitchKeyStatus |= 0x100;
	} else {
		vSwitchKeyStatus &= ~0x100;
	}
	APP_SET_DATA(vSwitchKeyStatus, vSwitchKeyStatus);
	notifyMainProcess();
}

void SettingsDialog::onSwitchKeyAltChange(bool enabled) {
	if (enabled) {
		vSwitchKeyStatus |= 0x200;
	} else {
		vSwitchKeyStatus &= ~0x200;
	}
	APP_SET_DATA(vSwitchKeyStatus, vSwitchKeyStatus);
	notifyMainProcess();
}

void SettingsDialog::onSwitchKeyWinChange(bool enabled) {
	if (enabled) {
		vSwitchKeyStatus |= 0x400;
	} else {
		vSwitchKeyStatus &= ~0x400;
	}
	APP_SET_DATA(vSwitchKeyStatus, vSwitchKeyStatus);
	notifyMainProcess();
}

void SettingsDialog::onSwitchKeyShiftChange(bool enabled) {
	if (enabled) {
		vSwitchKeyStatus |= 0x800;
	} else {
		vSwitchKeyStatus &= ~0x800;
	}
	APP_SET_DATA(vSwitchKeyStatus, vSwitchKeyStatus);
	notifyMainProcess();
}

void SettingsDialog::onSwitchKeyCharChange(int charCode) {
	// Store the character in the high byte (bits 24-31)
	vSwitchKeyStatus &= 0x00FFFFFF;  // Clear high byte
	vSwitchKeyStatus |= ((unsigned int)charCode << 24);
	APP_SET_DATA(vSwitchKeyStatus, vSwitchKeyStatus);
	notifyMainProcess();
}

void SettingsDialog::onBeepChange(bool enabled) {
	if (enabled) {
		vSwitchKeyStatus |= 0x8000;
	} else {
		vSwitchKeyStatus &= ~0x8000;
	}
	APP_SET_DATA(vSwitchKeyStatus, vSwitchKeyStatus);
	notifyMainProcess();
}

void SettingsDialog::onSmartSwitchChange(bool enabled) {
	vUseSmartSwitchKey = enabled ? 1 : 0;
	APP_SET_DATA(vUseSmartSwitchKey, vUseSmartSwitchKey);
	notifyMainProcess();
}

void SettingsDialog::onOpenAdvancedSettings() {
	openAdvancedSettings();
}

void SettingsDialog::openAdvancedSettings() {
	// Toggle expansion state - handled by JavaScript
	// Just call the JS function if we need to programmatically toggle

}

void SettingsDialog::onExpandChange(bool isExpanded) {
	m_isExpanded = isExpanded;
	
	// Set timer to resize window after CSS transition (300ms + 50ms buffer)
	SetTimer(get_hwnd(), TIMER_RESIZE_WINDOW, 350, NULL);
}

void SettingsDialog::recalcWindowSize() {
	// Get current window position
	RECT rc;
	GetWindowRect(get_hwnd(), &rc);
	int x = rc.left;
	int y = rc.top;
	
	sciter::dom::element rootEl = this->root();
	
	// Constants for layout calculation (from CSS)
	const int TITLE_BAR_HEIGHT = 36;
	const int TAB_HEADER_HEIGHT = 40;
	const int TAB_BODY_PADDING = 32;
	
	// Get compact section height (left panel)
	sciter::dom::element compactSection = rootEl.find_first(".compact-section");
	int compactHeight = 0;
	if (compactSection) {
		RECT compactRect = compactSection.get_location(CONTENT_BOX);
		compactHeight = compactRect.bottom - compactRect.top;
	}
	
	int newWidth = 350;
	int newHeight = TITLE_BAR_HEIGHT + compactHeight;
	
	if (m_isExpanded) {
		newWidth = 750;
		
		// Measure active tab content height
		sciter::dom::element activeTabBody = rootEl.find_first(".tab-panel.active .tab-body");
		int advancedHeight = TITLE_BAR_HEIGHT + TAB_HEADER_HEIGHT;
		
		if (activeTabBody) {
			RECT tabBodyRect = activeTabBody.get_location(CONTENT_BOX);
			advancedHeight += (tabBodyRect.bottom - tabBodyRect.top) + TAB_BODY_PADDING;
		}
		
		// Use max of left and right panel heights
		newHeight = max(newHeight, advancedHeight);
	}
	
	// Apply minimum constraints
	newWidth = max(newWidth, 350);
	newHeight = max(newHeight, 200);
	
	// Resize window
	SetWindowPos(get_hwnd(), NULL, x, y, newWidth, newHeight, SWP_NOZORDER);
}

