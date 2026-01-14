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
#include "SpecialAppsDialogSciter.h"
#include "stdafx.h"
#include "OpenKeyHelper.h"
#include "ConfigManager.h"
#include <dwmapi.h>
#include <CommCtrl.h>
#include <windowsx.h>
#include <set>
#include <TlHelp32.h>
#include <sstream>
#include <cctype>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "comctl32.lib")

#include "ScaleHelper.h"

extern int vBackgroundOpacity;

#ifndef OCR_NORMAL
#define OCR_NORMAL 32512
#endif

// Helper function to convert UTF-8 to wide string
extern std::wstring utf8ToWideString(const std::string& utf8str);
// Helper function to convert wide string to UTF-8
extern std::string wideStringToUtf8(const std::wstring& wstr);

// Global special app lists (extern to OpenKey.cpp)
extern std::vector<std::string> _qtElectronApps;
extern std::vector<std::string> _skipImeCheckApps;

// Debug logging macro - only active in debug builds
#ifdef _DEBUG
#define DEBUG_LOG(msg) OutputDebugStringA(msg)
#else
#define DEBUG_LOG(msg) ((void)0)
#endif

// ConfigManager keys for user-added apps and deleted defaults
static const char* CFG_SECTION_SPECIAL_APPS = "specialApps";
static const char* CFG_QT_ELECTRON_APPS = "qtElectronApps";
static const char* CFG_SKIP_IME_APPS = "skipImeCheckApps";
static const char* CFG_DELETED_DEFAULTS = "deletedDefaults";

// Helper function to force window to foreground
static void forceForegroundWindow(HWND hwnd) {
    HWND hForeground = GetForegroundWindow();
    if (hForeground == hwnd) return;
    
    DWORD dwCurrentThread = GetCurrentThreadId();
    DWORD dwForegroundThread = GetWindowThreadProcessId(hForeground, NULL);
    
    if (dwCurrentThread != dwForegroundThread) {
        AttachThreadInput(dwCurrentThread, dwForegroundThread, TRUE);
    }
    
    SetForegroundWindow(hwnd);
    BringWindowToTop(hwnd);
    SetActiveWindow(hwnd);
    
    if (dwCurrentThread != dwForegroundThread) {
        AttachThreadInput(dwCurrentThread, dwForegroundThread, FALSE);
    }
    
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);
}

// Acrylic blur structures
struct ACCENT_POLICY_SPECIAL {
    int AccentState;
    int AccentFlags;
    int GradientColor;
    int AnimationId;
};

struct WINDOWCOMPOSITIONATTRIBDATA_SPECIAL {
    int Attrib;
    void* pvData;
    size_t cbData;
};

enum ACCENT_STATE_SPECIAL {
    ACCENT_DISABLED_SPECIAL = 0,
    ACCENT_ENABLE_BLURBEHIND_SPECIAL = 3,
    ACCENT_ENABLE_ACRYLICBLURBEHIND_SPECIAL = 4
};

// Default hardcoded lists - MUST match OpenKey.cpp (lowercase for case-insensitive matching)
static std::vector<std::string> _defaultQtElectronApps = {
    "notepadnext.exe",    // NotepadNext (Qt)
    "code.exe",           // VSCode (Electron)
    "sublime_text.exe",   // Sublime Text
    "atom.exe",           // Atom (Electron)
    "discord.exe",        // Discord (Electron)
    "slack.exe"           // Slack (Electron)
};

static std::vector<std::string> _defaultSkipImeCheckApps = {
    "powerpnt.exe",   // Microsoft PowerPoint
    "winword.exe",    // Microsoft Word
    "excel.exe"       // Microsoft Excel
};

// ===== Helper Functions =====

std::string SpecialAppsDialogSciter::toLower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(), 
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

bool SpecialAppsDialogSciter::isDuplicate(const std::string& exeName) {
    std::string lowerName = toLower(exeName);
    for (const auto& entry : m_appsList) {
        if (toLower(entry.exeName) == lowerName) {
            return true;
        }
    }
    return false;
}

// ===== Constructor =====

SpecialAppsDialogSciter::SpecialAppsDialogSciter() 
    : sciter::window(SW_POPUP | SW_ALPHA | SW_ENABLE_DEBUG, RECT{ 0, 0, 420, 520 }) {
    
    // Load HTML
#ifdef NDEBUG
    if (!load(WSTR("this://app/specialapps/specialapps.html"))) {
        MessageBoxW(NULL, L"Failed to load specialapps.html from resources", L"Error", MB_OK | MB_ICONERROR);
        return;
    }
#else
    WCHAR exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    WCHAR* lastSlash = wcsrchr(exePath, L'\\');
    if (lastSlash) *lastSlash = L'\0';
    
    WCHAR htmlPath[MAX_PATH];
    swprintf_s(htmlPath, MAX_PATH, L"%s\\Resources\\Sciter\\specialapps\\specialapps.html", exePath);
    
    if (!load(htmlPath)) {
        MessageBoxW(NULL, htmlPath, L"Failed to load specialapps.html", MB_OK | MB_ICONERROR);
        return;
    }
#endif
    
    expand();
    
    // "Ứng dụng đặc biệt" = "\u1EE8ng d\u1EE5ng \u0111\u1EB7c bi\u1EC7t"
    SetWindowTextW(get_hwnd(), L"\u1EE8ng d\u1EE5ng \u0111\u1EB7c bi\u1EC7t");
    
    int scaledWidth, scaledHeight;
    ScaleHelper::getScaledSize(400, 450, scaledWidth, scaledHeight);
    SetWindowPos(get_hwnd(), NULL, 0, 0, scaledWidth, scaledHeight, SWP_NOMOVE | SWP_NOZORDER);
    
    // Center window
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    RECT rc;
    GetWindowRect(get_hwnd(), &rc);
    int x = (screenWidth - (rc.right - rc.left)) / 2;
    int y = (screenHeight - (rc.bottom - rc.top)) / 2;
    SetWindowPos(get_hwnd(), HWND_NOTOPMOST, x, y, 0, 0, SWP_NOSIZE);
    
    enableAcrylicEffect();
    SetWindowSubclass(get_hwnd(), SpecialAppsDialogSciter::SubclassProc, 1, (DWORD_PTR)this);
}

SpecialAppsDialogSciter::~SpecialAppsDialogSciter() {
}

void SpecialAppsDialogSciter::show() {
    ShowWindow(get_hwnd(), SW_SHOW);
    SetForegroundWindow(get_hwnd());
}

void SpecialAppsDialogSciter::enableAcrylicEffect() {
    HWND hwnd = get_hwnd();
    SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);

    HMODULE hUser = GetModuleHandle(L"user32.dll");
    if (hUser) {
        typedef BOOL(WINAPI* pSetWindowCompositionAttribute)(HWND, WINDOWCOMPOSITIONATTRIBDATA_SPECIAL*);
        auto SetWindowCompositionAttribute = 
            (pSetWindowCompositionAttribute)GetProcAddress(hUser, "SetWindowCompositionAttribute");

        if (SetWindowCompositionAttribute) {
            ACCENT_POLICY_SPECIAL policy = { 0 };
            policy.AccentState = ACCENT_ENABLE_BLURBEHIND_SPECIAL;
            policy.AccentFlags = 0;
            policy.GradientColor = 0x00000000;
            policy.AnimationId = 0;

            WINDOWCOMPOSITIONATTRIBDATA_SPECIAL data = { 0 };
            data.Attrib = 19;
            data.pvData = &policy;
            data.cbData = sizeof(policy);

            SetWindowCompositionAttribute(hwnd, &data);
        }
        else {
            DWM_BLURBEHIND bb = { 0 };
            bb.dwFlags = DWM_BB_ENABLE;
            bb.fEnable = TRUE;
            bb.hRgnBlur = NULL;
            DwmEnableBlurBehindWindow(hwnd, &bb);
        }
    }

    // Round corners on Windows 11
    int preference = 2;
    DwmSetWindowAttribute(hwnd, 33, &preference, sizeof(preference));
}

LRESULT CALLBACK SpecialAppsDialogSciter::SubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    SpecialAppsDialogSciter* dialog = reinterpret_cast<SpecialAppsDialogSciter*>(dwRefData);
    
    if (msg == WM_CLOSE) {
        // NOTE: Must use ExitProcess(0) for Sciter subprocesses!
        // PostQuitMessage(0) causes Sciter reference counting assertion failure
        // because sciter::window destructor expects ref_cntr == 0
        ExitProcess(0);
        return 0;
    }
    
    // IPC: Bring window to foreground (sent from main process when window already exists)
    if (msg == WM_USER + 107) {
        return OpenKeyHelper::handleIPCForeground(hwnd);
    }
    
    // Window Picker: Handle mouse click
    if (msg == WM_LBUTTONUP && dialog && dialog->m_isPickingWindow) {
        POINT pt;
        GetCursorPos(&pt);
        HWND targetWnd = WindowFromPoint(pt);
        
        if (targetWnd) {
            targetWnd = GetAncestor(targetWnd, GA_ROOT);
        }
        
        if (targetWnd && targetWnd != hwnd) {
            std::string exeName = dialog->getExeNameFromWindow(targetWnd);
            if (!exeName.empty()) {
                dialog->stopWindowPicking();
                dialog->onAddPickedApp(exeName);
                return 0;
            }
        }
        dialog->stopWindowPicking();
        return 0;
    }
    
    // Window Picker: ESC to cancel
    if (msg == WM_KEYDOWN && wParam == VK_ESCAPE && dialog && dialog->m_isPickingWindow) {
        dialog->stopWindowPicking();
        return 0;
    }
    
    if (msg == WM_NCHITTEST) {
        LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
        if (result == HTCLIENT) {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(hwnd, &pt);
            
            RECT winRect;
            GetClientRect(hwnd, &winRect);
            int closeButtonZone = winRect.right - 40;
            
            if (pt.y < 40 && pt.x < closeButtonZone) {
                return HTCAPTION;
            }
        }
        return result;
    }
    
    if (msg == WM_SETCURSOR && dialog && dialog->m_isPickingWindow) {
        SetCursor(LoadCursor(NULL, IDC_CROSS));
        return TRUE;
    }
    
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

// ===== Data Management =====

void SpecialAppsDialogSciter::loadAppsList() {
    m_appsList.clear();
    
    auto& config = ConfigManager::instance();
    config.init();
    
    // Load list of deleted defaults from config
    std::set<std::string> deletedDefaults;
    auto deletedList = config.getStringArray(CFG_SECTION_SPECIAL_APPS, CFG_DELETED_DEFAULTS);
    for (const auto& app : deletedList) {
        deletedDefaults.insert(toLower(app));
    }
    
    // Load defaults - Qt/Electron apps (skip if deleted)
    for (const auto& app : _defaultQtElectronApps) {
        if (deletedDefaults.find(toLower(app)) != deletedDefaults.end()) {
            continue;  // User deleted this default
        }
        SpecialAppEntry entry;
        entry.exeName = app;
        entry.type = SpecialAppType::QtElectron;
        entry.isDefault = true;
        if (!isDuplicate(entry.exeName)) {
            m_appsList.push_back(entry);
        }
    }
    
    // Load defaults - Skip IME Check apps (skip if deleted)
    DEBUG_LOG("=== Loading Skip IME Check defaults ===\n");
    for (const auto& app : _defaultSkipImeCheckApps) {
        DEBUG_LOG(("  - " + app + "\n").c_str());
        if (deletedDefaults.find(toLower(app)) != deletedDefaults.end()) {
            DEBUG_LOG("    SKIPPED (deleted by user)\n");
            continue;  // User deleted this default
        }
        SpecialAppEntry entry;
        entry.exeName = app;
        entry.type = SpecialAppType::SkipImeCheck;
        entry.isDefault = true;
        if (!isDuplicate(entry.exeName)) {
            m_appsList.push_back(entry);
            DEBUG_LOG("    ADDED with type=SkipImeCheck\n");
        } else {
            DEBUG_LOG("    SKIPPED (duplicate)\n");
        }
    }
    DEBUG_LOG(("=== Total apps in list: " + std::to_string(m_appsList.size()) + " ===\n").c_str());
    
    // Load user-added Qt/Electron apps from config
    auto userQtApps = config.getStringArray(CFG_SECTION_SPECIAL_APPS, CFG_QT_ELECTRON_APPS);
    for (const auto& utf8Name : userQtApps) {
        if (!isDuplicate(utf8Name)) {
            SpecialAppEntry entry;
            entry.exeName = utf8Name;
            entry.type = SpecialAppType::QtElectron;
            entry.isDefault = false;
            m_appsList.push_back(entry);
        }
    }
    
    // Load user-added Skip IME apps from config
    auto userImeApps = config.getStringArray(CFG_SECTION_SPECIAL_APPS, CFG_SKIP_IME_APPS);
    for (const auto& utf8Name : userImeApps) {
        if (!isDuplicate(utf8Name)) {
            SpecialAppEntry entry;
            entry.exeName = utf8Name;
            entry.type = SpecialAppType::SkipImeCheck;
            entry.isDefault = false;
            m_appsList.push_back(entry);
        }
    }
}

void SpecialAppsDialogSciter::saveData() {
    // Collect user-added apps (non-default) by type
    std::vector<std::string> userQtApps;
    std::vector<std::string> userImeApps;
    
    for (const auto& entry : m_appsList) {
        if (entry.isDefault) continue;  // Skip defaults
        
        if (entry.type == SpecialAppType::QtElectron) {
            userQtApps.push_back(entry.exeName);
        } else {
            userImeApps.push_back(entry.exeName);
        }
    }
    
    // Save to ConfigManager
    auto& config = ConfigManager::instance();
    config.setStringArray(CFG_SECTION_SPECIAL_APPS, CFG_QT_ELECTRON_APPS, userQtApps);
    config.setStringArray(CFG_SECTION_SPECIAL_APPS, CFG_SKIP_IME_APPS, userImeApps);
    config.save();
    
    // Update global lists immediately (no restart needed!)
    _qtElectronApps.clear();
    _skipImeCheckApps.clear();
    
    for (const auto& entry : m_appsList) {
        if (entry.type == SpecialAppType::QtElectron) {
            _qtElectronApps.push_back(entry.exeName);
        } else {
            _skipImeCheckApps.push_back(entry.exeName);
        }
    }
    
    // Debug: Log global lists after update
    DEBUG_LOG("=== saveData: Global lists updated ===\n");
    DEBUG_LOG(("_qtElectronApps count: " + std::to_string(_qtElectronApps.size()) + "\n").c_str());
    for (const auto& app : _qtElectronApps) {
        DEBUG_LOG(("  Qt: " + app + "\n").c_str());
    }
    DEBUG_LOG(("_skipImeCheckApps count: " + std::to_string(_skipImeCheckApps.size()) + "\n").c_str());
    for (const auto& app : _skipImeCheckApps) {
        DEBUG_LOG(("  IME: " + app + "\n").c_str());
    }
    
    // Notify main process
    HWND mainWnd = FindWindow(APP_CLASS, NULL);
    if (mainWnd) {
        PostMessage(mainWnd, WM_USER + 101, 0, 0);
    }
}

void SpecialAppsDialogSciter::fillAppsListUI() {
    call_function("clearAppList");
    
    DEBUG_LOG("=== fillAppsListUI ===\n");
    for (const auto& entry : m_appsList) {
        std::wstring wName = utf8ToWideString(entry.exeName);
        int typeInt = static_cast<int>(entry.type);
        
        DEBUG_LOG(("  UI: " + entry.exeName + ", type=" + std::to_string(typeInt) + ", isDefault=" + std::to_string(entry.isDefault) + "\n").c_str());
        
        // Pass isDefault directly - JS expects isDefault (not canDelete)
        call_function("addAppToList", wName.c_str(), typeInt, entry.isDefault);
    }
    
    call_function("forceRefresh");
}

// ===== Event Handling =====

bool SpecialAppsDialogSciter::handle_event(HELEMENT he, BEHAVIOR_EVENT_PARAMS& params) {
    if (params.cmd == DOCUMENT_READY) {
        loadAppsList();
        fillAppsListUI();
        
        // Apply theme - read opacity from ConfigManager (subprocess must read from config)
        int bgOpacity = ConfigManager::instance().getInt("system", "backgroundOpacity", 80);
        
        sciter::dom::element root = get_root();
        bool isDarkMode = OpenKeyHelper::isWindowsDarkMode();
        sciter::dom::element body = root.find_first("body");
        if (body) {
            if (isDarkMode) {
                body.set_attribute("class", L"dark");
            } else {
                body.remove_attribute("class");
            }
        }
        
        sciter::dom::element container = root.find_first(".container");
        if (container) {
            double opacity = bgOpacity / 100.0;
            wchar_t bgColor[64];
            if (isDarkMode) {
                swprintf_s(bgColor, L"rgba(18, 20, 28, %.2f)", opacity * 0.9);
            } else {
                swprintf_s(bgColor, L"rgba(255, 255, 255, %.2f)", opacity);
            }
            container.set_style_attribute("background-color", bgColor);
        }
        
        return true;
    }
    
    if (params.cmd == VALUE_CHANGED) {
        sciter::dom::element el(params.heTarget);
        std::wstring id = el.get_attribute("id");
        
        if (id == L"val-action") {
            sciter::dom::element root = this->root();
            sciter::value actionVal = el.get_value();
            std::wstring action = actionVal.is_string() ? actionVal.get<std::wstring>() : L"";
            
            if (action == L"add-app") {
                sciter::dom::element nameEl = root.find_first("#val-app-name");
                sciter::dom::element typeEl = root.find_first("#val-app-type");
                
                if (nameEl.is_valid() && typeEl.is_valid()) {
                    sciter::value nameVal = nameEl.get_value();
                    sciter::value typeVal = typeEl.get_value();
                    
                    std::wstring name = nameVal.is_string() ? nameVal.get<std::wstring>() : L"";
                    
                    // CRITICAL FIX: HTML select value is STRING ("0" or "1"), not int!
                    int typeInt = 0;
                    if (typeVal.is_int()) {
                        typeInt = typeVal.get<int>();
                    } else if (typeVal.is_string()) {
                        std::wstring typeStr = typeVal.get<std::wstring>();
                        typeInt = _wtoi(typeStr.c_str());
                    }
                    
                    DEBUG_LOG(("onAddApp: name=" + wideStringToUtf8(name) + ", type=" + std::to_string(typeInt) + "\n").c_str());
                    
                    if (!name.empty()) {
                        onAddApp(name, static_cast<SpecialAppType>(typeInt));
                    }
                }
                el.set_value(sciter::value(L""));
                return true;
            }
            
            if (action == L"delete-app") {
                sciter::dom::element nameEl = root.find_first("#val-app-name");
                if (nameEl.is_valid()) {
                    sciter::value nameVal = nameEl.get_value();
                    std::wstring name = nameVal.is_string() ? nameVal.get<std::wstring>() : L"";
                    if (!name.empty()) {
                        onDeleteApp(name);
                    }
                }
                el.set_value(sciter::value(L""));
                return true;
            }
            
            if (action == L"change-type") {
                sciter::dom::element nameEl = root.find_first("#val-app-name");
                sciter::dom::element typeEl = root.find_first("#val-app-type");
                
                if (nameEl.is_valid() && typeEl.is_valid()) {
                    sciter::value nameVal = nameEl.get_value();
                    sciter::value typeVal = typeEl.get_value();
                    
                    std::wstring name = nameVal.is_string() ? nameVal.get<std::wstring>() : L"";
                    // Handle both int and string from JS (select.value is string)
                    int typeInt = 0;
                    if (typeVal.is_int()) {
                        typeInt = typeVal.get<int>();
                    } else if (typeVal.is_string()) {
                        std::wstring typeStr = typeVal.get<std::wstring>();
                        typeInt = (typeStr == L"1") ? 1 : 0;
                    }
                    
                    if (!name.empty()) {
                        onChangeType(name, static_cast<SpecialAppType>(typeInt));
                    }
                }
                el.set_value(sciter::value(L""));
                return true;
            }
            
            if (action == L"pick-window") {
                startWindowPicking();
                el.set_value(sciter::value(L""));
                return true;
            }
            
            if (action == L"get-running-apps") {
                sendRunningAppsToJS();
                el.set_value(sciter::value(L""));
                return true;
            }
            
            if (action == L"close") {
                PostMessage(get_hwnd(), WM_CLOSE, 0, 0);
                return true;
            }
        }
    }
    
    if (params.cmd == BUTTON_CLICK) {
        sciter::dom::element el(params.heTarget);
        std::wstring id = el.get_attribute("id");
        
        if (id == L"btn-close") {
            PostMessage(get_hwnd(), WM_CLOSE, 0, 0);
            return true;
        }
    }
    
    return false;
}

// ===== App Management =====

void SpecialAppsDialogSciter::onAddApp(const std::wstring& appName, SpecialAppType type) {
    std::string utf8Name = wideStringToUtf8(appName);
    
    if (isDuplicate(utf8Name)) {
        // "Ứng dụng này đã có trong danh sách!"
        MessageBoxW(get_hwnd(), 
            L"\u1EE8ng d\u1EE5ng n\u00E0y \u0111\u00E3 c\u00F3 trong danh s\u00E1ch!", 
            L"OpenKey", 
            MB_OK | MB_ICONINFORMATION);
        return;
    }
    
    SpecialAppEntry entry;
    entry.exeName = utf8Name;
    entry.type = type;
    entry.isDefault = false;
    m_appsList.push_back(entry);
    
    saveData();
    
    // Update UI - pass false for isDefault (user-added apps are NOT defaults)
    call_function("addAppToList", appName.c_str(), static_cast<int>(type), false);
    call_function("clearInput");
    call_function("forceRefresh", sciter::value(true));
    
    forceForegroundWindow(get_hwnd());
}

void SpecialAppsDialogSciter::onDeleteApp(const std::wstring& appName) {
    DEBUG_LOG(("onDeleteApp: " + wideStringToUtf8(appName) + "\n").c_str());
    
    std::string utf8Name = wideStringToUtf8(appName);
    std::string lowerName = toLower(utf8Name);
    bool wasDefault = false;
    bool found = false;
    
    // Find and remove - both defaults and user-added can be deleted
    for (auto it = m_appsList.begin(); it != m_appsList.end(); ++it) {
        if (toLower(it->exeName) == lowerName) {
            wasDefault = it->isDefault;
            m_appsList.erase(it);
            found = true;
            DEBUG_LOG(("  Found and deleted, wasDefault=" + std::to_string(wasDefault) + "\n").c_str());
            break;
        }
    }
    
    if (!found) {
        DEBUG_LOG("  NOT FOUND in list!\n");
    }
    
    // If deleting a default, add it to deleted defaults list
    if (wasDefault) {
        auto& config = ConfigManager::instance();
        auto deletedList = config.getStringArray(CFG_SECTION_SPECIAL_APPS, CFG_DELETED_DEFAULTS);
        deletedList.push_back(utf8Name);
        config.setStringArray(CFG_SECTION_SPECIAL_APPS, CFG_DELETED_DEFAULTS, deletedList);
        config.save();
    }
    
    saveData();
    
    call_function("removeAppFromList", appName.c_str());
    call_function("forceRefresh");
}

void SpecialAppsDialogSciter::onChangeType(const std::wstring& appName, SpecialAppType newType) {
    std::string utf8Name = wideStringToUtf8(appName);
    std::string lowerName = toLower(utf8Name);
    
    for (auto& entry : m_appsList) {
        if (toLower(entry.exeName) == lowerName) {
            entry.type = newType;
            break;
        }
    }
    
    saveData();
}

// ===== Window Picker =====

void SpecialAppsDialogSciter::startWindowPicking() {
    m_isPickingWindow = true;
    SetCapture(get_hwnd());
    
    HCURSOR hOriginalArrow = LoadCursor(NULL, IDC_ARROW);
    m_hSavedArrowCursor = CopyCursor(hOriginalArrow);
    
    HCURSOR hCross = LoadCursor(NULL, IDC_CROSS);
    HCURSOR hCrossCopy = CopyCursor(hCross);
    if (!SetSystemCursor(hCrossCopy, OCR_NORMAL)) {
        DestroyCursor(hCrossCopy);  // Prevent handle leak on failure
    }
}

void SpecialAppsDialogSciter::stopWindowPicking() {
    m_isPickingWindow = false;
    ReleaseCapture();
    
    if (m_hSavedArrowCursor) {
        SetSystemCursor(m_hSavedArrowCursor, OCR_NORMAL);
        m_hSavedArrowCursor = NULL;
    }
    
    forceForegroundWindow(get_hwnd());
}

std::string SpecialAppsDialogSciter::getExeNameFromWindow(HWND hwnd) {
    if (!hwnd) return "";
    
    DWORD processId = 0;
    GetWindowThreadProcessId(hwnd, &processId);
    if (processId == 0) return "";
    
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (!hProcess) return "";
    
    WCHAR exePath[MAX_PATH] = {0};
    GetProcessImageFileNameW(hProcess, exePath, MAX_PATH);
    CloseHandle(hProcess);
    
    if (wcslen(exePath) == 0) return "";
    
    WCHAR* filename = wcsrchr(exePath, L'\\');
    if (filename) filename++;
    else filename = exePath;
    
    return wideStringToUtf8(filename);
}

void SpecialAppsDialogSciter::onAddPickedApp(const std::string& exeName) {
    if (exeName.find("OpenKey") != std::string::npos) {
        MessageBoxW(get_hwnd(), 
            L"Kh\u00F4ng th\u1EC3 th\u00EAm OpenKey v\u00E0o danh s\u00E1ch!", 
            L"OpenKey", 
            MB_OK | MB_ICONWARNING);
        return;
    }
    
    if (isDuplicate(exeName)) {
        MessageBoxW(get_hwnd(), 
            L"\u1EE8ng d\u1EE5ng n\u00E0y \u0111\u00E3 c\u00F3 trong danh s\u00E1ch!", 
            L"OpenKey", 
            MB_OK | MB_ICONINFORMATION);
        return;
    }
    
    // Default type is QtElectron (0)
    SpecialAppEntry entry;
    entry.exeName = exeName;
    entry.type = SpecialAppType::QtElectron;
    entry.isDefault = false;
    m_appsList.push_back(entry);
    
    saveData();
    
    std::wstring wName = utf8ToWideString(exeName);
    call_function("addAppToList", wName.c_str(), 0, false);  // User-picked = NOT default
    call_function("clearInput");
    call_function("forceRefresh");
    
    forceForegroundWindow(get_hwnd());
}

// ===== Running Apps Dropdown =====

void SpecialAppsDialogSciter::sendRunningAppsToJS() {
    std::set<std::string> runningApps;
    std::set<DWORD> processIdsWithWindows;
    
    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        auto* pids = reinterpret_cast<std::set<DWORD>*>(lParam);
        if (GetParent(hwnd) == NULL) {
            DWORD pid = 0;
            GetWindowThreadProcessId(hwnd, &pid);
            if (pid != 0) {
                pids->insert(pid);
            }
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&processIdsWithWindows));
    
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return;
    }
    
    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);
    
    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            if (processIdsWithWindows.find(pe32.th32ProcessID) == processIdsWithWindows.end()) {
                continue;
            }
            
            std::wstring exeName = pe32.szExeFile;
            
            if (_wcsicmp(exeName.c_str(), L"ApplicationFrameHost.exe") == 0 ||
                exeName.find(L"OpenKey") != std::wstring::npos ||
                _wcsicmp(exeName.c_str(), L"TextInputHost.exe") == 0) {
                continue;
            }
            
            std::string utf8Name = wideStringToUtf8(exeName);
            if (!utf8Name.empty()) {
                runningApps.insert(utf8Name);
            }
            
        } while (Process32NextW(hSnapshot, &pe32));
    }
    
    CloseHandle(hSnapshot);
    
    sciter::value appsArray;
    appsArray.set_item(0, sciter::value());
    appsArray.clear();
    
    int index = 0;
    for (const auto& app : runningApps) {
        std::wstring wApp = utf8ToWideString(app);
        appsArray.set_item(index++, sciter::value(wApp.c_str()));
    }
    
    call_function("setRunningApps", appsArray);
}

BOOL CALLBACK SpecialAppsDialogSciter::EnumWindowsCallback(HWND hwnd, LPARAM lParam) {
    // Not used - using lambda instead
    return TRUE;
}
