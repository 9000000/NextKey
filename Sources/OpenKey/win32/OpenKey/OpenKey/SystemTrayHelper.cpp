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
#include "SystemTrayHelper.h"
#include "AppDelegate.h"
#include "SettingsDialog.h"
#include "OpenKeyManager.h"
#include "ConfigManager.h"
#include "SharedState.h"
#include "ConfigIntent.h"
#include "PerformanceLogger.h"
#include "RuntimeProfile.h"
#include <Wtsapi32.h>
#include <CommCtrl.h>

#pragma comment(lib, "Wtsapi32.lib")
#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Gdiplus.lib")
#pragma comment(lib, "D2d1.lib")
#pragma comment(lib, "Dwrite.lib")
#pragma comment(lib, "windowscodecs.lib")

#include <gdiplus.h>
#include <d2d1.h>
#include <dwrite.h>
#include <initguid.h>
#include <wincodec.h>

using namespace Gdiplus;

// Extern declaration for macro engine function
extern void initMacroMap(const Byte* pData, const int& size);
extern void initMacrosFromList(const std::vector<std::pair<std::string, std::string>>& macros);
extern void initEnglishOnlyApps(const Byte* pData, const int& size);
extern void initEnglishOnlyAppsFromList(const std::vector<std::string>& apps);
extern void initSmartSwitchKey(const Byte* pData, const int& size);
extern void initSmartSwitchKeyFromMap(const std::map<std::string, int>& data);
extern void vSetCheckSpelling();  // Engine state sync for spell checking

#define TIMER_REINSTALL_HOOKS 1001
#define TIMER_CONFIG_SAVE 1002
#define CONFIG_SAVE_DEBOUNCE_MS 1500

// Central Writer state - config dirty flag for debounced save
static bool s_configDirty = false;

#define WM_TRAYMESSAGE (WM_USER + 1)
// TRAY_ICON_ID is now defined in SystemTrayHelper.h

#define POPUP_VIET_ON_OFF 900
#define POPUP_SPELLING 901
#define POPUP_SMART_SWITCH 902
#define POPUP_USE_MACRO 903

#define POPUP_TELEX 910
#define POPUP_VNI 911
#define POPUP_SIMPLE_TELEX_1 912
#define POPUP_SIMPLE_TELEX_2 913

#define POPUP_UNICODE 930
#define POPUP_TCVN3 931
#define POPUP_VNI_WINDOWS 932
#define POPUP_UNICODE_COMPOUND 933
#define POPUP_VN_LOCALE_1258 934

#define POPUP_CONVERT_TOOL 980
#define POPUP_QUICK_CONVERT 981

#define POPUP_MACRO_TABLE 990

#define POPUP_CONTROL_PANEL 1000
#define POPUP_ABOUT_OPENKEY 1010
#define POPUP_OPENKEY_EXIT 2000

#define MODIFY_MENU(MENU, COMMAND, DATA) ModifyMenu(MENU, COMMAND, \
											MF_BYCOMMAND | (DATA ? MF_CHECKED : MF_UNCHECKED), \
											COMMAND, \
											menuData[COMMAND]);

static HMENU popupMenu;
//static HMENU menuInputType;
static HMENU otherCode;

static NOTIFYICONDATA nid;
static ULONGLONG lastUnlockTime = 0;

#define SESSION_UNLOCK_DEBOUNCE_MS 2000

map<UINT, LPCTSTR> menuData = {
	{POPUP_VIET_ON_OFF, _T("Bật Tiếng Việt")},
	{POPUP_SPELLING, _T("Bật kiểm tra chính tả")},
	{POPUP_SMART_SWITCH, _T("Bật loại trừ ứng dụng thông minh")},
	{POPUP_USE_MACRO, _T("Bật gõ tắt")},
	{POPUP_TELEX, _T("Kiểu gõ Telex")},
	{POPUP_VNI, _T("Kiểu gõ VNI")},
	{POPUP_SIMPLE_TELEX_1, _T("Kiểu gõ Simple Telex 1")},
	{POPUP_SIMPLE_TELEX_2, _T("Kiểu gõ Simple Telex 2")},
	{POPUP_UNICODE, _T("Unicode dựng sẵn")},
	{POPUP_TCVN3, _T("TCVN3 (ABC)")},
	{POPUP_VNI_WINDOWS, _T("VNI Windows")},
	{POPUP_UNICODE_COMPOUND, _T("Unicode tổ hợp")},
	{POPUP_VN_LOCALE_1258, _T("Vietnamese locale CP 1258")},
	{POPUP_CONVERT_TOOL, _T("Công cụ chuyển mã...")},
	{POPUP_QUICK_CONVERT, _T("Chuyển mã nhanh")},
	{POPUP_MACRO_TABLE, _T("Cấu hình gõ tắt...")},
	{POPUP_CONTROL_PANEL, _T("Bảng điều khiển...")},
	{POPUP_ABOUT_OPENKEY, _T("Giới thiệu NextKey")},
	{POPUP_OPENKEY_EXIT, _T("Thoát")},
};

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	static UINT taskbarCreated;

	switch (message) {
	case WM_CREATE:
		taskbarCreated = RegisterWindowMessage(_T("TaskbarCreated"));
		
		// Register session notification for lock/unlock detection
		if (!WTSRegisterSessionNotification(hWnd, NOTIFY_FOR_THIS_SESSION)) {
			OutputDebugString(_T("OpenKey: Failed to register session notification\n"));
		} else {
			OutputDebugString(_T("OpenKey: Session notification registered successfully\n"));
		}
		break;
	case WM_USER+2019:
		AppDelegate::getInstance()->onControlPanel();
		break;
	
	// Handle deferred AboutDialog destruction (posted from WM_CLOSE handler)
	case WM_USER+100:
		AppDelegate::getInstance()->closeAboutDialog();
		break;
	
	// Handle settings reload notification from SettingsDialog subprocess
	case WM_USER+101: {
		LOG(L"[Main] WM_USER+101 received - reloading settings from config.toml\n");
		
		// Reload config from disk (subprocess may have modified it)
		auto& config = ConfigManager::instance();
		config.load();
		
		// === Load settings from ConfigManager (RAM) with camelCase keys ===
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
		vTempOffMacro = config.getBool("macro", "tempOffMacroEsc", false) ? 1 : 0;
		
		// System Tab (Hệ thống)
		vRunWithWindows = config.getBool("system", "runWithWindows", false) ? 1 : 0;
		vRunAsAdmin = config.getBool("system", "runAsAdmin", false) ? 1 : 0;
		vCheckNewVersion = config.getBool("system", "checkNewVersion", false) ? 1 : 0;
		vCreateDesktopShortcut = config.getBool("system", "createDesktopShortcut", false) ? 1 : 0;
		vSupportMetroApp = config.getBool("system", "supportMetroApp", false) ? 1 : 0;
		vFixChromiumBrowser = config.getBool("system", "fixChromiumBrowser", false) ? 1 : 0;
		vSendKeyStepByStep = config.getBool("system", "useClipboard", true) ? 0 : 1;
		vUseGrayIcon = config.getInt("system", "iconStyle", 0);
		vTrayIconColorV = (COLORREF)config.getInt("system", "customColorV", 0);
		vTrayIconColorE = (COLORREF)config.getInt("system", "customColorE", 0);
		vShowOnStartUp = config.getBool("system", "showOnStartup", false) ? 1 : 0;
		SettingsDialog::s_blurMode = (SettingsDialog::BlurMode)config.getInt("ui", "blurMode", 0);
		LOG(L"[Main] Loaded settings - BlurMode: %d, colors: V=0x%08X, E=0x%08X\n", (int)SettingsDialog::s_blurMode, vTrayIconColorV, vTrayIconColorE);
		
		// Excluded Apps
		vExcludeApps = config.getBool("excludedApps", "enabled", false) ? 1 : 0;
		
		// Convert Tool
		convertToolHotKey = config.getInt("convertTool", "hotkey", 0);
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
		
		// Reload macro data from ConfigManager (TOML)
		{
			auto macros = config.getMacros();
			initMacrosFromList(macros);  // Always use ConfigManager - no registry fallback
		}
		
		// Reload English-only apps data from ConfigManager (TOML)
		{
			auto excludedApps = config.getStringArray("excludedApps", "list");
			initEnglishOnlyAppsFromList(excludedApps);  // Always use ConfigManager - no registry fallback
		}
		
		// Reload smart switch data from ConfigManager (TOML)
		{
			auto smartSwitchData = config.getSmartSwitchData();
			if (!smartSwitchData.empty()) {
				initSmartSwitchKeyFromMap(smartSwitchData);
			}
		}
		
		// Reload special apps lists (Qt/Electron and Skip IME apps)
		{
			extern void reloadSpecialAppsLists();
			reloadSpecialAppsLists();
		}
		
		// Reload clipboard apps (per-app injection method)
		{
			extern void reloadClipboardApps();
			reloadClipboardApps();
		}
		
		// Clear RuntimeProfile cache so new app overrides take effect
		// This forces getOrCreateProfile to re-apply user overrides on next focus
		g_hwndProfileCache.clear();
		
		// Refresh tray icon and menu to reflect new settings
		SystemTrayHelper::updateData();
	}
		break;
		
	// Handle RAM-only settings sync from SettingsDialog (debounced save)
	// This updates the tray icon/menu without reloading from disk
	case WM_USER+102: {
		// Sync RAM settings ( ConfigManager cache is already updated by subprocess via SharedState 
		// if we used it, BUT here we assume subprocess updated ConfigManager RAM? 
		// WAIT - ConfigManager is separate per process! 
		// We need to use SendMessage/SharedMem to sync values if avoiding disk!
		// But in our current plan, we rely on vLanguage/vInputType globals being updated?
		// Subprocess CANNOT update Main process globals directly!
		
		// CORRECTION: Since ConfigManager is PER-PROCESS, sharing via RAM only works if
		// we use SharedState or pass data via Message. 
		// Currently vLanguage IS synced via IPC messages but other settings are not.
		
		// However, the requirement was "RAM sync only". 
		// If we don't reload from disk, main process won't see changes until disk save!
		// BUT: Subprocess handles UI. Main process handles typing.
		// Main process only needs critical settings (InputType, etc).
		
		// For now, let's strictly follow the plan:
		// "Just update tray icon to reflect changes"
		// This assumes critical changes (like InputType) might still need disk load OR 
		// utilize existing SharedState/IPC if available.
		
		// Actually, SystemTrayHelper::updateData() reads from GLOBAL variables (vLanguage, etc.)
		// Since we didn't update globals from disk (no load()), they might be stale!
		// WE NEED TO RELOAD FROM DISK TO GET VALUES if we don't have another mechanism.
		// UNLESS: The "debounce" implies main process continues with OLD settings until save?
		// NO, that would be bad (user changes input type, expects immediate effect).
		
		// RE-EVALUATION: To support "RAM sync without disk load", we need to pass data.
		// But passing 50+ settings is hard.
		// Maybe we should just let WM_USER+102 do a "Partial Load" or...
		
		// User said: "dùng SharedState cho critical values" in option 4/5 discussion?
		// Let's check SharedState implementation.
		// If SharedState has vLanguage/vInputType/vCodeTable, we are good for criticals.
		
		SharedState& state = SharedState::instance();
		if (state.isValid()) {
			vLanguage = state.getLanguage();
			vInputType = state.getInputType(); // Assuming these exist
			vCodeTable = state.getCodeTable();
			// ... sync other criticals if available ...
		}
		
		SystemTrayHelper::updateData();
	}
		break;
	
	// Handle macro table open request from SettingsDialog subprocess
	case WM_USER+103:
		AppDelegate::getInstance()->onMacroTable();
		break;
	
	// Handle language-only change from subprocess (no disk reload needed)
	case WM_USER+108: {
		// wParam contains new language value
		vLanguage = (int)wParam;
		
		// Update smart switch if enabled
		if (vUseSmartSwitchKey) {
			extern void setAppInputMethodStatus(const std::string& bundleId, const int& language);
			extern void saveSmartSwitchKeyData();
			std::string exe = OpenKeyHelper::getFrontMostAppExecuteName();
			setAppInputMethodStatus(exe, vLanguage | (vCodeTable << 1));
			saveSmartSwitchKeyData();  // Updates RAM cache only
		}
		
		// Sync to SharedState
		SharedState::instance().setLanguage(vLanguage);
		
		// Update tray icon
		SystemTrayHelper::updateData();
	}
		break;
	
	// Handle excluded apps dialog open request from SettingsDialog subprocess
	case WM_USER+104:
		AppDelegate::getInstance()->onSpawnExcludedAppsSciter();
		break;
	
	// Handle manual update check request from SettingsDialog subprocess
	case WM_USER+105:
		AppDelegate::getInstance()->onCheckUpdate();
		break;
	
	// Handle app overrides dialog open request from SettingsDialog subprocess
	case WM_USER+110:
		AppDelegate::getInstance()->onSpawnAppOverrides();
		break;
		
	// ============================================================
	// Central Writer: Handle config intents from subprocess dialogs
	// ============================================================
	case WM_COPYDATA: {
		COPYDATASTRUCT* cds = (COPYDATASTRUCT*)lParam;
		ConfigIntentType type = static_cast<ConfigIntentType>(cds->dwData);
		const uint8_t* data = static_cast<const uint8_t*>(cds->lpData);
		size_t size = cds->cbData;
		
		LOG(L"[CentralWriter] Received intent type=%d, size=%zu\n", (int)type, size);
		
		// Validate minimum payload size
		if (size < sizeof(IntentHeader)) {
			LOG(L"[CentralWriter] Invalid payload: too small\n");
			return FALSE;
		}
		
		auto& config = ConfigManager::instance();
		bool handled = false;
		
		switch (type) {
		case ConfigIntentType::UPDATE_MACROS: {
			std::vector<std::pair<std::string, std::string>> macros;
			if (deserializeMacros(data, size, macros)) {
				config.setMacros(macros);
				initMacrosFromList(macros);  // Update engine
				LOG(L"[CentralWriter] Updated %zu macros\n", macros.size());
				handled = true;
			}
			break;
		}
		
		case ConfigIntentType::UPDATE_SETTINGS: {
			SettingsPayload settings;
			if (deserializeSettings(data, size, settings)) {
				// Apply settings to ConfigManager
				config.setInt("general", "language", settings.language);
				config.setInt("general", "inputType", settings.inputType);
				config.setInt("general", "codeTable", settings.codeTable);
				config.setInt("general", "switchKey", settings.switchKey);
				config.setBool("general", "smartSwitch", settings.smartSwitch != 0);
				
				config.setBool("typing", "checkSpelling", settings.checkSpelling != 0);
				config.setBool("typing", "restoreWrongSpelling", settings.restoreWrongSpelling != 0);
				config.setBool("typing", "modernOrthography", settings.modernOrthography != 0);
				config.setBool("typing", "fixRecommendBrowser", settings.fixRecommendBrowser != 0);
				config.setBool("typing", "upperCaseFirstChar", settings.upperCaseFirstChar != 0);
				config.setBool("typing", "allowZwfj", settings.allowZwfj != 0);
				config.setBool("typing", "tempOffSpellingCtrl", settings.tempOffSpelling != 0);
				config.setBool("typing", "tempOffOpenKeyAlt", settings.tempOffOpenKey != 0);
				config.setBool("typing", "rememberCode", settings.rememberCode != 0);
				
				config.setBool("macro", "enabled", settings.macroEnabled != 0);
				config.setBool("macro", "useInEnglishMode", settings.macroInEnglish != 0);
				config.setBool("macro", "autoCaps", settings.autoCapsMacro != 0);
				config.setBool("macro", "quickTelex", settings.quickTelex != 0);
				config.setBool("macro", "quickStartConsonant", settings.quickStartConsonant != 0);
				config.setBool("macro", "quickEndConsonant", settings.quickEndConsonant != 0);
				config.setBool("macro", "tempOffMacroEsc", settings.tempOffMacro != 0);
				
				config.setBool("system", "runWithWindows", settings.runWithWindows != 0);
				config.setBool("system", "runAsAdmin", settings.runAsAdmin != 0);
				config.setBool("system", "checkNewVersion", settings.checkNewVersion != 0);
				config.setBool("system", "createDesktopShortcut", settings.createDesktopShortcut != 0);
				config.setBool("system", "supportMetroApp", settings.supportMetroApp != 0);
				config.setBool("system", "fixChromiumBrowser", settings.fixChromiumBrowser != 0);
				config.setBool("system", "useClipboard", settings.useClipboard != 0);
				config.setInt("system", "iconStyle", settings.iconStyle);
				config.setInt("system", "customColorV", settings.customColorV);
				config.setInt("system", "customColorE", settings.customColorE);
				config.setBool("system", "showOnStartup", settings.showOnStartup != 0);
				config.setInt("system", "showAdvancedSettings", settings.showAdvancedSettings);
				config.setInt("system", "backgroundOpacity", settings.backgroundOpacity);
				config.setInt("ui", "blurMode", settings.blurMode);
				
				config.setBool("excludedApps", "enabled", settings.excludeAppsEnabled != 0);
				config.setBool("debug", "enablePerfLog", settings.enablePerfLog != 0);
				
				// Update global variables for engine
				vLanguage = settings.language;
				vInputType = settings.inputType;
				vCodeTable = settings.codeTable;
				vSwitchKeyStatus = settings.switchKey;
				vUseSmartSwitchKey = settings.smartSwitch;
				vCheckSpelling = settings.checkSpelling;
				vSetCheckSpelling();  // CRITICAL: Sync engine state for spell checking
				vUseMacro = settings.macroEnabled;
				SettingsDialog::s_blurMode = (SettingsDialog::BlurMode)settings.blurMode;
				LOG(L"[Main] ConfigIntent: Sync BlurMode = %d\n", (int)SettingsDialog::s_blurMode);
				
				// ADD: Missing typing settings
				vRestoreIfWrongSpelling = settings.restoreWrongSpelling;
				vUseModernOrthography = settings.modernOrthography;
				vFixRecommendBrowser = settings.fixRecommendBrowser;
				vUpperCaseFirstChar = settings.upperCaseFirstChar;
				vAllowConsonantZFWJ = settings.allowZwfj;
				vTempOffSpelling = settings.tempOffSpelling;
				vTempOffOpenKey = settings.tempOffOpenKey;
				vRememberCode = settings.rememberCode;
				
				// ADD: Missing macro settings
				vUseMacroInEnglishMode = settings.macroInEnglish;
				vAutoCapsMacro = settings.autoCapsMacro;
				vQuickTelex = settings.quickTelex;
				vQuickStartConsonant = settings.quickStartConsonant;
				vQuickEndConsonant = settings.quickEndConsonant;
				vTempOffMacro = settings.tempOffMacro;
				
				// ADD: Missing system settings
				vSupportMetroApp = settings.supportMetroApp;
				vFixChromiumBrowser = settings.fixChromiumBrowser;
				vSendKeyStepByStep = settings.useClipboard ? 0 : 1;  // Inverted!
				vUseGrayIcon = settings.iconStyle;
				vTrayIconColorV = settings.customColorV;
				vTrayIconColorE = settings.customColorE;
				
				// ADD: Excluded apps + debug
				vExcludeApps = settings.excludeAppsEnabled;
				vEnablePerfLog = settings.enablePerfLog;
				
				// Update PerformanceLogger state
				PerformanceLogger::setEnabled(vEnablePerfLog != 0);
				
				LOG(L"[CentralWriter] Updated settings\n");
				handled = true;
			}
			break;
		}
		
	case ConfigIntentType::UPDATE_CONVERT_TOOL: {
			ConvertToolPayload payload;
			if (deserializeConvertTool(data, size, payload)) {
				config.setInt("convertTool", "hotkey", payload.hotkey);
				config.setInt("convertTool", "fromCode", payload.fromCode);
				config.setInt("convertTool", "toCode", payload.toCode);
				config.setBool("convertTool", "toAllCaps", payload.toAllCaps != 0);
				config.setBool("convertTool", "toAllNonCaps", payload.toAllNonCaps != 0);
				config.setBool("convertTool", "removeMark", payload.removeMark != 0);
				config.setBool("convertTool", "toCapsEachWord", payload.toCapsEachWord != 0);
				config.setBool("convertTool", "toCapsFirstLetter", payload.toCapsFirstLetter != 0);
				config.setBool("convertTool", "dontAlertCompleted", payload.dontAlertCompleted != 0);
				config.setBool("convertTool", "autoPasteReselect", payload.autoPasteReselect != 0);
				config.setBool("convertTool", "sequentialMode", payload.sequentialMode != 0);
				
				// Update global variables - ALL of them!
				convertToolHotKey = payload.hotkey;
				convertToolFromCode = payload.fromCode;
				convertToolToCode = payload.toCode;
				convertToolToAllCaps = payload.toAllCaps;
				convertToolToAllNonCaps = payload.toAllNonCaps;
				convertToolRemoveMark = payload.removeMark;
				convertToolToCapsEachWord = payload.toCapsEachWord;
				convertToolToCapsFirstLetter = payload.toCapsFirstLetter;
				convertToolDontAlertWhenCompleted = payload.dontAlertCompleted;
				vQuickConvertAutoPaste = payload.autoPasteReselect;
				vQuickConvertSequential = payload.sequentialMode;
				
				LOG(L"[CentralWriter] Updated convert tool settings\n");
				handled = true;
			}
			break;
		}
		
		case ConfigIntentType::UPDATE_EXCLUDED_APPS: {
			std::vector<std::string> apps;
			std::map<std::string, int> smartSwitchData;
			if (deserializeExcludedApps(data, size, apps, smartSwitchData)) {
				config.setStringArray("excludedApps", "list", apps);
				config.setSmartSwitchData(smartSwitchData);
				
				// Update engine
				initEnglishOnlyAppsFromList(apps);
				initSmartSwitchKeyFromMap(smartSwitchData);
				
				LOG(L"[CentralWriter] Updated %zu excluded apps\n", apps.size());
				handled = true;
			}
			break;
		}
		
		default:
			LOG(L"[CentralWriter] Unhandled intent type=%d\n", (int)type);
			break;
		}
		
		if (handled) {
			// Mark dirty and restart debounce timer
			s_configDirty = true;
			KillTimer(hWnd, TIMER_CONFIG_SAVE);
			SetTimer(hWnd, TIMER_CONFIG_SAVE, CONFIG_SAVE_DEBOUNCE_MS, NULL);
			
			// Update tray icon to reflect changes
			SystemTrayHelper::updateData();
		}
		
		return handled ? TRUE : FALSE;
	}
		
	// Handle session change (lock/unlock)
	case WM_WTSSESSION_CHANGE:
		if (wParam == WTS_SESSION_LOCK) {
			OutputDebugString(_T("OpenKey: Session locked\n"));
		} else if (wParam == WTS_SESSION_UNLOCK) {
			// Debounce: Only process if at least 2 seconds apart
			ULONGLONG now = GetTickCount64();
			if (lastUnlockTime == 0 || now - lastUnlockTime > SESSION_UNLOCK_DEBOUNCE_MS) {
				lastUnlockTime = now;
				OutputDebugString(_T("OpenKey: Session unlocked. Scheduling hook reinstall...\n"));
				
				// Use timer to delay 500ms, then reinstall from main thread
				SetTimer(hWnd, TIMER_REINSTALL_HOOKS, 500, NULL);
			}
		}
		break;
		
	// Handle timer for hook reinstallation and config save
	case WM_TIMER:
		if (wParam == TIMER_REINSTALL_HOOKS) {
			KillTimer(hWnd, TIMER_REINSTALL_HOOKS);
			
			// CRITICAL: Called from main thread (has message loop)
			OutputDebugString(_T("OpenKey: Reinstalling hooks from main thread...\n"));
			OpenKeyManager::reinstallHooks();
		}
		else if (wParam == TIMER_CONFIG_SAVE) {
			KillTimer(hWnd, TIMER_CONFIG_SAVE);
			
			// Central Writer: Debounced save
			if (s_configDirty) {
				ConfigManager::instance().save();
				s_configDirty = false;
				LOG(L"[CentralWriter] Config saved (debounced)\n");
			}
		}
		break;
	case WM_TRAYMESSAGE: {
		if (lParam == WM_LBUTTONDBLCLK) {
			AppDelegate::getInstance()->onControlPanel();
		}
		if (lParam == WM_LBUTTONUP) {
			AppDelegate::getInstance()->onToggleVietnamese();
			SystemTrayHelper::updateData();
			// Notify settings subprocess to update UI if it's open
			HWND settingsWnd = FindWindow(NULL, _T("NextKey Settings"));
			if (settingsWnd) {
				PostMessage(settingsWnd, WM_USER + 102, 0, 0);
			}
		} else if (lParam == WM_RBUTTONDOWN) {
			POINT curPoint;
			GetCursorPos(&curPoint);
			SetForegroundWindow(hWnd);
			UINT commandId = TrackPopupMenu(
				popupMenu,
				TPM_RETURNCMD | TPM_NONOTIFY,
				curPoint.x,
				curPoint.y,
				0,
				hWnd,
				NULL
			);
			switch (commandId) {
			case POPUP_VIET_ON_OFF:
				AppDelegate::getInstance()->onToggleVietnamese();
				break;
			case POPUP_SPELLING:
				AppDelegate::getInstance()->onToggleCheckSpelling();
				break;
			case POPUP_SMART_SWITCH:
				AppDelegate::getInstance()->onToggleUseSmartSwitchKey();
				break;
			case POPUP_USE_MACRO:
				AppDelegate::getInstance()->onToggleUseMacro();
				break;
			case POPUP_MACRO_TABLE:
				AppDelegate::getInstance()->onMacroTable();
				break;
			case POPUP_CONVERT_TOOL:
				AppDelegate::getInstance()->onConvertTool();
				break;
			case POPUP_QUICK_CONVERT:
				AppDelegate::getInstance()->onQuickConvert();
				break;
			case POPUP_TELEX:
				AppDelegate::getInstance()->onInputType(0);
				break;
			case POPUP_VNI:
				AppDelegate::getInstance()->onInputType(1);
				break;
			case POPUP_SIMPLE_TELEX_1:
				AppDelegate::getInstance()->onInputType(2);
				break;
			case POPUP_SIMPLE_TELEX_2:
				AppDelegate::getInstance()->onInputType(3);
				break;
			case POPUP_UNICODE:
				AppDelegate::getInstance()->onTableCode(0);
				break;
			case POPUP_TCVN3:
				AppDelegate::getInstance()->onTableCode(1);
				break;
			case POPUP_VNI_WINDOWS:
				AppDelegate::getInstance()->onTableCode(2);
				break;
			case POPUP_UNICODE_COMPOUND:
				AppDelegate::getInstance()->onTableCode(3);
				break;
			case POPUP_VN_LOCALE_1258:
				AppDelegate::getInstance()->onTableCode(4);
				break;
			case POPUP_CONTROL_PANEL:
				AppDelegate::getInstance()->onControlPanel();
				break;
			case POPUP_ABOUT_OPENKEY:
				AppDelegate::getInstance()->onOpenKeyAbout();
				break;
			case POPUP_OPENKEY_EXIT:
				AppDelegate::getInstance()->onOpenKeyExit();
				break;
			}
			SystemTrayHelper::updateData();
			
			// Notify settings subprocess to update UI if it's open
			HWND settingsWnd = FindWindow(NULL, _T("NextKey Settings"));
			if (settingsWnd) {
				PostMessage(settingsWnd, WM_USER + 102, 0, 0);
			}
		}
	}
	break;
	
	case WM_DESTROY:
		// Kill timers
		KillTimer(hWnd, TIMER_REINSTALL_HOOKS);
		KillTimer(hWnd, TIMER_CONFIG_SAVE);
		
		// Central Writer: Force save on exit
		if (s_configDirty) {
			ConfigManager::instance().save();
			s_configDirty = false;
			LOG(L"[CentralWriter] Config saved (on exit)\n");
		}
		
		// Unregister session notification on destroy
		WTSUnRegisterSessionNotification(hWnd);
		OutputDebugString(_T("OpenKey: Session notification unregistered\n"));
		break;
	
	// Handle Windows shutdown/logoff - force save
	case WM_ENDSESSION:
	case WM_QUERYENDSESSION:
		if (s_configDirty) {
			ConfigManager::instance().save();
			s_configDirty = false;
			LOG(L"[CentralWriter] Config saved (session end)\n");
		}
		if (message == WM_QUERYENDSESSION) return TRUE;
		break;
		
	default:
		// if the taskbar is restarted, add the system tray icon again
		if (message == taskbarCreated) {
			Shell_NotifyIcon(NIM_ADD, &nid);
		}
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

HWND SystemTrayHelper::createFakeWindow(const HINSTANCE & hIns) {
	//create fake window
	WNDCLASSEXW wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = 0;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hIns;
	wcex.hIcon = LoadIcon(hIns, MAKEINTRESOURCE(IDI_APP_ICON));
	wcex.hCursor = NULL;
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = APP_CLASS;
	wcex.hIconSm = NULL;
	ATOM atom = RegisterClassExW(&wcex);
	HWND hWnd = CreateWindowW(APP_CLASS, _T(""), WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hIns, nullptr);
	if (!hWnd) {
		return NULL;
	}
	ShowWindow(hWnd, 0);
	UpdateWindow(hWnd);
	return hWnd;
}

void SystemTrayHelper::createPopupMenu() {
	popupMenu = CreatePopupMenu();
	AppendMenu(popupMenu, MF_CHECKED, POPUP_VIET_ON_OFF, menuData[POPUP_VIET_ON_OFF]);
	AppendMenu(popupMenu, MF_SEPARATOR, 0, 0);
	AppendMenu(popupMenu, MF_CHECKED, POPUP_SPELLING, menuData[POPUP_SPELLING]);
	AppendMenu(popupMenu, MF_CHECKED, POPUP_SMART_SWITCH, menuData[POPUP_SMART_SWITCH]);
	AppendMenu(popupMenu, MF_CHECKED, POPUP_USE_MACRO, menuData[POPUP_USE_MACRO]);
	AppendMenu(popupMenu, MF_SEPARATOR, 0, 0);
	AppendMenu(popupMenu, MF_UNCHECKED, POPUP_MACRO_TABLE, menuData[POPUP_MACRO_TABLE]);
	AppendMenu(popupMenu, MF_UNCHECKED, POPUP_CONVERT_TOOL, menuData[POPUP_CONVERT_TOOL]);
	AppendMenu(popupMenu, MF_UNCHECKED, POPUP_QUICK_CONVERT, menuData[POPUP_QUICK_CONVERT]);
	AppendMenu(popupMenu, MF_SEPARATOR, 0, 0);

	//menuInputType = CreatePopupMenu();
	AppendMenu(popupMenu, MF_CHECKED, POPUP_TELEX, menuData[POPUP_TELEX]);
	AppendMenu(popupMenu, MF_CHECKED, POPUP_VNI, menuData[POPUP_VNI]);
	AppendMenu(popupMenu, MF_CHECKED, POPUP_SIMPLE_TELEX_1, menuData[POPUP_SIMPLE_TELEX_1]);
	AppendMenu(popupMenu, MF_CHECKED, POPUP_SIMPLE_TELEX_2, menuData[POPUP_SIMPLE_TELEX_2]);

	//AppendMenu(popupMenu, MF_POPUP, (UINT_PTR)menuInputType, _T("Kiểu gõ"));
	AppendMenu(popupMenu, MF_SEPARATOR, 0, 0);

	AppendMenu(popupMenu, MF_UNCHECKED, POPUP_UNICODE, menuData[POPUP_UNICODE]);
	AppendMenu(popupMenu, MF_UNCHECKED, POPUP_TCVN3, menuData[POPUP_TCVN3]);
	AppendMenu(popupMenu, MF_UNCHECKED, POPUP_VNI_WINDOWS, menuData[POPUP_VNI_WINDOWS]);

	otherCode = CreatePopupMenu();
	AppendMenu(otherCode, MF_CHECKED, POPUP_UNICODE_COMPOUND, menuData[POPUP_UNICODE_COMPOUND]);
	AppendMenu(otherCode, MF_CHECKED, POPUP_VN_LOCALE_1258, menuData[POPUP_VN_LOCALE_1258]);
	AppendMenu(popupMenu, MF_POPUP, (UINT_PTR)otherCode, _T("Bảng mã khác"));

	AppendMenu(popupMenu, MF_SEPARATOR, 0, 0);

	AppendMenu(popupMenu, MF_STRING, POPUP_CONTROL_PANEL, menuData[POPUP_CONTROL_PANEL]);
	AppendMenu(popupMenu, MF_UNCHECKED, POPUP_ABOUT_OPENKEY, menuData[POPUP_ABOUT_OPENKEY]);
	AppendMenu(popupMenu, MF_SEPARATOR, 0, 0);
	AppendMenu(popupMenu, MF_UNCHECKED, POPUP_OPENKEY_EXIT, menuData[POPUP_OPENKEY_EXIT]);

	SetMenuDefaultItem(popupMenu, POPUP_CONTROL_PANEL, false);
}

// GDI+ initialization token
static ULONG_PTR gdiplusToken = 0;

// Initialize GDI+ (call once at startup)
static void initGdiPlus() {
	if (gdiplusToken == 0) {
		GdiplusStartupInput gdiplusStartupInput;
		GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	}
}

// Use shared default color constants from stdafx.h:
// TRAY_DEFAULT_COLOR_V and TRAY_DEFAULT_COLOR_E

// Create dynamic tray icon with custom font using DirectWrite
// DirectWrite provides better text quality than GDI+
// Parameters:
//   letter: "V" or "E"
//   color: COLORREF for text color
//   fontName: Font face name
// Returns: HICON (caller must DestroyIcon when done)
static HICON createDynamicTrayIconWithFont(const wchar_t* letter, COLORREF color, const wchar_t* fontName) {
	// Get system small icon size (DPI-aware)
	int iconSize = GetSystemMetrics(SM_CXSMICON);
	if (iconSize < 16) iconSize = 16;
	
	// Use supersampling for better quality
	const int scale = 4;
	int renderSize = iconSize * scale;
	
	// Initialize COM (needed for WIC)
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	
	// Create DirectWrite factory
	IDWriteFactory* pDWriteFactory = NULL;
	HRESULT hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, 
		__uuidof(IDWriteFactory), (IUnknown**)&pDWriteFactory);
	if (FAILED(hr)) return NULL;
	
	// Create text format
	IDWriteTextFormat* pTextFormat = NULL;
	hr = pDWriteFactory->CreateTextFormat(
		fontName,
		NULL,  // Font collection (NULL = system fonts)
		DWRITE_FONT_WEIGHT_BOLD,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		(FLOAT)(renderSize * 1.2f),  // Font size - 120% of render size
		L"",  // Locale
		&pTextFormat);
	
	if (FAILED(hr)) {
		// Fallback to Arial
		hr = pDWriteFactory->CreateTextFormat(L"Arial", NULL,
			DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL, (FLOAT)(renderSize * 1.2f), L"", &pTextFormat);
		if (FAILED(hr)) {
			pDWriteFactory->Release();
			return NULL;
		}
	}
	
	// Center text
	pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
	
	// Create WIC factory for bitmap
	IWICImagingFactory* pWICFactory = NULL;
	hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
		IID_IWICImagingFactory, (void**)&pWICFactory);
	if (FAILED(hr)) {
		pTextFormat->Release();
		pDWriteFactory->Release();
		return NULL;
	}
	
	// Create WIC bitmap
	IWICBitmap* pWICBitmap = NULL;
	hr = pWICFactory->CreateBitmap(renderSize, renderSize, 
		GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnDemand, &pWICBitmap);
	if (FAILED(hr)) {
		pWICFactory->Release();
		pTextFormat->Release();
		pDWriteFactory->Release();
		return NULL;
	}
	
	// Create D2D factory
	ID2D1Factory* pD2DFactory = NULL;
	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2DFactory);
	if (FAILED(hr)) {
		pWICBitmap->Release();
		pWICFactory->Release();
		pTextFormat->Release();
		pDWriteFactory->Release();
		return NULL;
	}
	
	// Create D2D render target on WIC bitmap
	D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties(
		D2D1_RENDER_TARGET_TYPE_DEFAULT,
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
	
	ID2D1RenderTarget* pRT = NULL;
	hr = pD2DFactory->CreateWicBitmapRenderTarget(pWICBitmap, rtProps, &pRT);
	if (FAILED(hr)) {
		LOG(L"[DWrite] CreateWicBitmapRenderTarget failed: 0x%08X\n", hr);
		pD2DFactory->Release();
		pWICBitmap->Release();
		pWICFactory->Release();
		pTextFormat->Release();
		pDWriteFactory->Release();
		return NULL;
	}
	
	// Set text antialiasing mode - GRAYSCALE works with transparent background (not ClearType)
	pRT->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
	
	// Create brush with user color
	ID2D1SolidColorBrush* pBrush = NULL;
	D2D1_COLOR_F d2dColor = D2D1::ColorF(
		GetRValue(color) / 255.0f,
		GetGValue(color) / 255.0f,
		GetBValue(color) / 255.0f,
		1.0f);
	hr = pRT->CreateSolidColorBrush(d2dColor, &pBrush);
	if (FAILED(hr)) {
		LOG(L"[DWrite] CreateSolidColorBrush failed: 0x%08X\n", hr);
	}
	
	// Draw text
	pRT->BeginDraw();
	pRT->Clear(D2D1::ColorF(0, 0, 0, 0));  // Transparent background
	
	D2D1_RECT_F layoutRect = D2D1::RectF(0, 0, (FLOAT)renderSize, (FLOAT)renderSize);
	pRT->DrawText(letter, (UINT32)wcslen(letter), pTextFormat, layoutRect, pBrush);
	
	hr = pRT->EndDraw();
	if (FAILED(hr)) {
		LOG(L"[DWrite] EndDraw failed: 0x%08X\n", hr);
	}
	
	// Get bitmap data
	WICRect lockRect = { 0, 0, renderSize, renderSize };
	IWICBitmapLock* pLock = NULL;
	pWICBitmap->Lock(&lockRect, WICBitmapLockRead, &pLock);
	
	UINT stride = 0;
	UINT bufferSize = 0;
	BYTE* pData = NULL;
	pLock->GetStride(&stride);
	pLock->GetDataPointer(&bufferSize, &pData);
	
	// Scale down using GDI+ (high quality bicubic)
	initGdiPlus();
	Bitmap largeBmp(renderSize, renderSize, stride, PixelFormat32bppPARGB, pData);
	
	Bitmap finalBmp(iconSize, iconSize, PixelFormat32bppARGB);
	Graphics finalGfx(&finalBmp);
	finalGfx.SetInterpolationMode(InterpolationModeHighQualityBicubic);
	finalGfx.SetSmoothingMode(SmoothingModeHighQuality);
	finalGfx.SetPixelOffsetMode(PixelOffsetModeHighQuality);
	finalGfx.DrawImage(&largeBmp, 0, 0, iconSize, iconSize);
	
	// Convert to HICON
	HICON hIcon = NULL;
	finalBmp.GetHICON(&hIcon);
	
	// Cleanup
	pLock->Release();
	pBrush->Release();
	pRT->Release();
	pD2DFactory->Release();
	pWICBitmap->Release();
	pWICFactory->Release();
	pTextFormat->Release();
	pDWriteFactory->Release();
	
	return hIcon;
}


// Create custom colored tray icon by colorizing existing icon
// Uses direct Windows API (no GDI+ Bitmap conversion to preserve quality)
// Parameters:
//   baseIconId: Resource ID of base icon (V or E)
//   newColor: COLORREF (RGB value) for new color
// Returns: HICON with new color (caller must DestroyIcon when done)
static HICON createColorizedTrayIcon(int baseIconId, COLORREF newColor) {
	// Load the base icon using LoadIconMetric for DPI-aware size
	HICON hBaseIcon = NULL;
	HRESULT hr = LoadIconMetric(GetModuleHandle(0), MAKEINTRESOURCE(baseIconId), LIM_SMALL, &hBaseIcon);
	if (!SUCCEEDED(hr) || !hBaseIcon) {
		hBaseIcon = LoadIcon(GetModuleHandle(0), MAKEINTRESOURCE(baseIconId));
		if (!hBaseIcon) return NULL;
	}
	
	// Get icon info
	ICONINFO iconInfo;
	if (!GetIconInfo(hBaseIcon, &iconInfo)) {
		DestroyIcon(hBaseIcon);
		return NULL;
	}
	
	// Get color bitmap info
	BITMAP bm;
	if (!GetObject(iconInfo.hbmColor, sizeof(bm), &bm)) {
		DeleteObject(iconInfo.hbmMask);
		DeleteObject(iconInfo.hbmColor);
		DestroyIcon(hBaseIcon);
		return NULL;
	}
	
	// Create DIB section for direct pixel access
	BITMAPINFO bmi = {};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = bm.bmWidth;
	bmi.bmiHeader.biHeight = -bm.bmHeight;  // Top-down
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	
	// Get the color bitmap bits
	HDC hdc = GetDC(NULL);
	BYTE* pixels = new BYTE[bm.bmWidth * bm.bmHeight * 4];
	
	if (GetDIBits(hdc, iconInfo.hbmColor, 0, bm.bmHeight, pixels, &bmi, DIB_RGB_COLORS)) {
		// Get new color components
		BYTE newR = GetRValue(newColor);
		BYTE newG = GetGValue(newColor);
		BYTE newB = GetBValue(newColor);
		
		// Process each pixel - DIB is BGRA format
		for (int i = 0; i < bm.bmWidth * bm.bmHeight; i++) {
			BYTE* pixel = pixels + i * 4;
			BYTE alpha = pixel[3];
			
			// Only change pixels that have some opacity
			if (alpha > 0) {
				pixel[0] = newB;  // Blue
				pixel[1] = newG;  // Green
				pixel[2] = newR;  // Red
				// Keep alpha unchanged
			}
		}
		
		// Set the modified bits back
		SetDIBits(hdc, iconInfo.hbmColor, 0, bm.bmHeight, pixels, &bmi, DIB_RGB_COLORS);
	}
	
	delete[] pixels;
	ReleaseDC(NULL, hdc);
	
	// Create new icon from modified bitmaps
	HICON hNewIcon = CreateIconIndirect(&iconInfo);
	
	// Cleanup
	DeleteObject(iconInfo.hbmMask);
	DeleteObject(iconInfo.hbmColor);
	DestroyIcon(hBaseIcon);
	
	return hNewIcon;
}


static void loadTrayIcon() {
	// Update tooltip based on language
	if (vLanguage) {
		LoadString(GetModuleHandle(0), IDS_TRAY_TITLE_2, nid.szTip, 128);
	} else {
		LoadString(GetModuleHandle(0), IDS_TRAY_TITLE, nid.szTip, 128);
	}
	
	// Auto-save default colors when Custom mode is selected but colors are not set
	// This handles edge case: user upgraded app or registry was reset while vUseGrayIcon=3
	if (vUseGrayIcon == 3 && vTrayIconColorV == 0 && vTrayIconColorE == 0) {
		vTrayIconColorV = TRAY_DEFAULT_COLOR_V;
		vTrayIconColorE = TRAY_DEFAULT_COLOR_E;
		APP_SET_DATA(vTrayIconColorV, vTrayIconColorV);
		APP_SET_DATA(vTrayIconColorE, vTrayIconColorE);
		LOG(L"[loadTrayIcon] Auto-saved default colors for Custom mode: V=0x%08X, E=0x%08X\n", 
			vTrayIconColorV, vTrayIconColorE);
	}
	
	// Check if custom color mode is selected (vUseGrayIcon == 3 means Custom)
	// Now colors are guaranteed to be set if mode is Custom
	bool useCustomColor = (vUseGrayIcon == 3);
	LOG(L"[loadTrayIcon] vUseGrayIcon=%d, useCustomColor=%d, ColorV=0x%08X, ColorE=0x%08X\n", 
		vUseGrayIcon, useCustomColor ? 1 : 0, vTrayIconColorV, vTrayIconColorE);
	
	HICON hIcon = NULL;
	
	if (useCustomColor) {
		// Use colorized version of base icon (best quality - preserves original icon)
		int baseIcon = vLanguage ? IDI_ICON_STATUS_VIET : IDI_ICON_STATUS_ENG;
		COLORREF customColor = vLanguage 
			? (vTrayIconColorV != 0 ? vTrayIconColorV : TRAY_DEFAULT_COLOR_V)
			: (vTrayIconColorE != 0 ? vTrayIconColorE : TRAY_DEFAULT_COLOR_E);
		
		LOG(L"[loadTrayIcon] Using colorized icon, baseIcon=%d, color=0x%08X\n", baseIcon, customColor);
		hIcon = createColorizedTrayIcon(baseIcon, customColor);
	} else {
		// Fall back to static icons based on vUseGrayIcon setting
		int icon = 0;
		if (vLanguage) {
			switch (vUseGrayIcon) {
				case 1: icon = IDI_ICON_STATUS_VIET_10; break;     // White
				case 2: icon = IDI_ICON_STATUS_VIET_BLACK; break;  // Black
				default: icon = IDI_ICON_STATUS_VIET; break;       // Color
			}
		} else {
			switch (vUseGrayIcon) {
				case 1: icon = IDI_ICON_STATUS_ENG_10; break;     // White
				case 2: icon = IDI_ICON_STATUS_ENG_BLACK; break;  // Black
				default: icon = IDI_ICON_STATUS_ENG; break;       // Color
			}
		}
		
		// Use LoadIconMetric for better High DPI scaling
		HRESULT hr = LoadIconMetric(GetModuleHandle(0), MAKEINTRESOURCE(icon), LIM_SMALL, &hIcon);
		if (!SUCCEEDED(hr)) {
			// Fallback to LoadIcon if LoadIconMetric fails
			hIcon = LoadIcon(GetModuleHandle(0), MAKEINTRESOURCE(icon));
		}
	}
	
	// Update the icon
	if (hIcon) {
		// Destroy old icon if any
		if (nid.hIcon) {
			DestroyIcon(nid.hIcon);
		}
		nid.hIcon = hIcon;
	}
}

void SystemTrayHelper::updateData() {
	loadTrayIcon();
	Shell_NotifyIcon(NIM_MODIFY, &nid);

	MODIFY_MENU(popupMenu, POPUP_VIET_ON_OFF, vLanguage);
	MODIFY_MENU(popupMenu, POPUP_SPELLING, vCheckSpelling);
	MODIFY_MENU(popupMenu, POPUP_SMART_SWITCH, vUseSmartSwitchKey);
	MODIFY_MENU(popupMenu, POPUP_USE_MACRO, vUseMacro);
	MODIFY_MENU(popupMenu, POPUP_TELEX, vInputType == 0);
	MODIFY_MENU(popupMenu, POPUP_VNI, vInputType == 1);
	MODIFY_MENU(popupMenu, POPUP_SIMPLE_TELEX_1, vInputType == 2);
	MODIFY_MENU(popupMenu, POPUP_SIMPLE_TELEX_2, vInputType == 3);
	MODIFY_MENU(popupMenu, POPUP_UNICODE, vCodeTable == 0);
	MODIFY_MENU(popupMenu, POPUP_TCVN3, vCodeTable == 1);
	MODIFY_MENU(popupMenu, POPUP_VNI_WINDOWS, vCodeTable == 2);
	MODIFY_MENU(otherCode, POPUP_UNICODE_COMPOUND, vCodeTable == 3);
	MODIFY_MENU(otherCode, POPUP_VN_LOCALE_1258, vCodeTable == 4);

	wstring hotkey = L"";
	bool hasAdd = false;
	if (convertToolHotKey & 0x100) {
		hotkey += L"Ctrl";
		hasAdd = true;
	}
	if (convertToolHotKey & 0x200) {
		if (hasAdd)
			hotkey += L" + ";
		hotkey += L"Alt";
		hasAdd = true;
	}
	if (convertToolHotKey & 0x400) {
		if (hasAdd)
			hotkey += L" + ";
		hotkey += L"Win";
		hasAdd = true;
	}
	if (convertToolHotKey & 0x800) {
		if (hasAdd)
			hotkey += L" + ";
		hotkey += L"Shift";
		hasAdd = true;
	}

	unsigned short k = ((convertToolHotKey >> 24) & 0xFF);
	if (k != 0xFE) {
		if (hasAdd)
			hotkey += L" + ";
		if (k == VK_SPACE)
			hotkey += L"Space";
		else
			hotkey += (wchar_t)k;
	}

	wstring hotKeyString = menuData[POPUP_QUICK_CONVERT];
	if (hasAdd) {
		hotKeyString += L" - [";
		hotKeyString += hotkey;
		hotKeyString += L"]";
	}
	ModifyMenu(popupMenu, POPUP_QUICK_CONVERT, MF_BYCOMMAND | MF_UNCHECKED, POPUP_QUICK_CONVERT, hotKeyString.c_str());
}

static HINSTANCE ins;
static int recreateCount = 0;

void SystemTrayHelper::_createSystemTrayIcon(const HINSTANCE& hIns) {
	HWND hWnd = createFakeWindow(ins);
	
	if (hWnd == NULL) { //Use timer to create
		if (recreateCount >= 5) {
			PostQuitMessage(0);
			return;
		}
		ins = hIns;
		SetTimer(NULL, 0, 1000 * 3, (TIMERPROC)&WaitToCreateFakeWindow);
		++recreateCount;
		return;
	}
	createPopupMenu();

	//create system tray
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hWnd;
	nid.uID = TRAY_ICON_ID;
	nid.uVersion = NOTIFYICON_VERSION;
	nid.uCallbackMessage = WM_TRAYMESSAGE;
	loadTrayIcon();
	LoadString(ins, IDS_APP_TITLE, nid.szTip, 128);
	nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;

	// Shell_NotifyIcon may fail if the system tray icon is not fully initialized
	const int maxRetries = 5;
	for (int attempt = 0; attempt < maxRetries; ++attempt) {
		if (Shell_NotifyIcon(NIM_ADD, &nid)) {
			break;
		}
		Sleep(1000);
	}
}


void CALLBACK SystemTrayHelper::WaitToCreateFakeWindow(HWND hwnd, UINT uMsg, UINT timerId, DWORD dwTime) {
	_createSystemTrayIcon(ins);
	KillTimer(0, timerId);
}

void SystemTrayHelper::createSystemTrayIcon(const HINSTANCE& hIns) {
	_createSystemTrayIcon(hIns);
}

void SystemTrayHelper::removeSystemTray() {
	// Clean up icon resource
	if (nid.hIcon) {
		DestroyIcon(nid.hIcon);
		nid.hIcon = NULL;
	}
	Shell_NotifyIcon(NIM_DELETE, &nid);
}

HWND SystemTrayHelper::getHwnd() {
	return nid.hWnd;
}