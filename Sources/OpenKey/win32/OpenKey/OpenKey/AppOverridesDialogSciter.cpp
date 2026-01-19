/*----------------------------------------------------------
NextKey - Vietnamese Keyboard Input Method

AppOverridesDialogSciter - Unified per-app configuration dialog
Replaces separate SpecialApps + ClipboardApps dialogs

Copyright (C) 2024 Phat Mai
-----------------------------------------------------------*/
#include "AppOverridesDialogSciter.h"
#include "stdafx.h"
#include "OpenKeyHelper.h"
#include "ConfigManager.h"
#include <dwmapi.h>
#include <CommCtrl.h>
#include <windowsx.h>
#include <TlHelp32.h>
#include <sstream>
#include <cctype>
#include <set>

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

// Debug logging macro
#ifdef _DEBUG
#define DEBUG_LOG(msg) OutputDebugStringA(msg)
#else
#define DEBUG_LOG(msg) ((void)0)
#endif

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
struct ACCENT_POLICY_APPOVERRIDES {
    int AccentState;
    int AccentFlags;
    int GradientColor;
    int AnimationId;
};

struct WINDOWCOMPOSITIONATTRIBDATA_APPOVERRIDES {
    int Attrib;
    void* pvData;
    size_t cbData;
};

enum ACCENT_STATE_APPOVERRIDES {
    ACCENT_DISABLED_APPOVERRIDES = 0,
    ACCENT_ENABLE_BLURBEHIND_APPOVERRIDES = 3,
    ACCENT_ENABLE_ACRYLICBLURBEHIND_APPOVERRIDES = 4
};

// ===== Helper Functions =====

std::string AppOverridesDialogSciter::toLower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(), 
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

bool AppOverridesDialogSciter::isDuplicate(const std::string& exeName) {
    std::string lowerName = toLower(exeName);
    for (const auto& entry : m_appsList) {
        if (toLower(entry.exeName) == lowerName) {
            return true;
        }
    }
    return false;
}

// ===== Constructor =====

AppOverridesDialogSciter::AppOverridesDialogSciter() 
    : sciter::window(SW_POPUP | SW_ALPHA | SW_ENABLE_DEBUG, RECT{ 0, 0, 450, 520 }) {
    
    // Load HTML
#ifdef NDEBUG
    if (!load(WSTR("this://app/appoverrides/appoverrides.html"))) {
        MessageBoxW(NULL, L"Failed to load appoverrides.html from resources", L"Error", MB_OK | MB_ICONERROR);
        return;
    }
#else
    WCHAR exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    WCHAR* lastSlash = wcsrchr(exePath, L'\\');
    if (lastSlash) *lastSlash = L'\0';
    
    WCHAR htmlPath[MAX_PATH];
    swprintf_s(htmlPath, MAX_PATH, L"%s\\Resources\\Sciter\\appoverrides\\appoverrides.html", exePath);
    
    if (!load(htmlPath)) {
        MessageBoxW(NULL, htmlPath, L"Failed to load appoverrides.html", MB_OK | MB_ICONERROR);
        return;
    }
#endif
    
    expand();
    
    // "Cấu hình ứng dụng"
    SetWindowTextW(get_hwnd(), L"C\u1EA5u h\u00ECnh \u1EE9ng d\u1EE5ng");
    
    int scaledWidth, scaledHeight;
    ScaleHelper::getScaledSize(430, 480, scaledWidth, scaledHeight);
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
    SetWindowSubclass(get_hwnd(), AppOverridesDialogSciter::SubclassProc, 1, (DWORD_PTR)this);
}

void AppOverridesDialogSciter::show() {
    ShowWindow(get_hwnd(), SW_SHOW);
    SetForegroundWindow(get_hwnd());
}

void AppOverridesDialogSciter::enableAcrylicEffect() {
    HWND hwnd = get_hwnd();
    SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);

    HMODULE hUser = GetModuleHandle(L"user32.dll");
    if (hUser) {
        typedef BOOL(WINAPI* pSetWindowCompositionAttribute)(HWND, WINDOWCOMPOSITIONATTRIBDATA_APPOVERRIDES*);
        auto SetWindowCompositionAttribute = 
            (pSetWindowCompositionAttribute)GetProcAddress(hUser, "SetWindowCompositionAttribute");

        if (SetWindowCompositionAttribute) {
            ACCENT_POLICY_APPOVERRIDES policy = { 0 };
            policy.AccentState = ACCENT_ENABLE_BLURBEHIND_APPOVERRIDES;
            policy.AccentFlags = 0;
            policy.GradientColor = 0x00000000;
            policy.AnimationId = 0;

            WINDOWCOMPOSITIONATTRIBDATA_APPOVERRIDES data = { 0 };
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

LRESULT CALLBACK AppOverridesDialogSciter::SubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    AppOverridesDialogSciter* dialog = reinterpret_cast<AppOverridesDialogSciter*>(dwRefData);
    
    if (msg == WM_CLOSE) {
        // NOTE: Must use ExitProcess(0) for Sciter subprocesses!
        ExitProcess(0);
        return 0;
    }
    
    // IPC: Bring window to foreground
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

void AppOverridesDialogSciter::loadAppsList() {
    m_appsList.clear();
    
    auto& config = ConfigManager::instance();
    config.init();
    
    // Load from new appOverrides section
    auto overrides = config.getAppOverrides();
    for (const auto& cfg : overrides) {
        AppEntry entry;
        entry.exeName = cfg.exeName;
        entry.behaviorType = cfg.behaviorType;
        entry.clipboardMethod = cfg.clipboardMethod;
        m_appsList.push_back(entry);
    }
    
    DEBUG_LOG(("Loaded " + std::to_string(m_appsList.size()) + " app overrides\n").c_str());
}

void AppOverridesDialogSciter::saveData() {
    // Convert to ConfigManager format
    std::vector<ConfigManager::AppOverrideConfig> configApps;
    for (const auto& entry : m_appsList) {
        ConfigManager::AppOverrideConfig cfg;
        cfg.exeName = entry.exeName;
        cfg.behaviorType = entry.behaviorType;
        cfg.clipboardMethod = entry.clipboardMethod;
        configApps.push_back(cfg);
    }
    
    // Save to ConfigManager
    auto& config = ConfigManager::instance();
    config.setAppOverrides(configApps);
    config.save();
    
    // Notify main process to reload
    HWND mainWnd = FindWindow(APP_CLASS, NULL);
    if (mainWnd) {
        PostMessage(mainWnd, WM_USER + 101, 0, 0);
    }
    
    DEBUG_LOG(("Saved " + std::to_string(m_appsList.size()) + " app overrides\n").c_str());
}

void AppOverridesDialogSciter::fillAppsListUI() {
    call_function("clearAppList");
    
    for (const auto& entry : m_appsList) {
        std::wstring wName = utf8ToWideString(entry.exeName);
        call_function("addAppToList", wName.c_str(), (int)entry.behaviorType, (int)entry.clipboardMethod);
    }
    
    call_function("forceRefresh");
}

// ===== Event Handling =====

bool AppOverridesDialogSciter::handle_event(HELEMENT he, BEHAVIOR_EVENT_PARAMS& params) {
    if (params.cmd == DOCUMENT_READY) {
        loadAppsList();
        fillAppsListUI();
        
        // Apply theme
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
                sciter::dom::element behaviorEl = root.find_first("#val-behavior-type");
                sciter::dom::element clipboardEl = root.find_first("#val-clipboard-method");
                
                if (nameEl.is_valid()) {
                    std::wstring name = nameEl.get_value().get<std::wstring>();
                    
                    int8_t behaviorType = 0;
                    if (behaviorEl.is_valid()) {
                        sciter::value bVal = behaviorEl.get_value();
                        if (bVal.is_int()) {
                            behaviorType = static_cast<int8_t>(bVal.get<int>());
                        } else if (bVal.is_string()) {
                            behaviorType = static_cast<int8_t>(_wtoi(bVal.get<std::wstring>().c_str()));
                        }
                    }
                    
                    int8_t clipboardMethod = -1;
                    if (clipboardEl.is_valid()) {
                        sciter::value cVal = clipboardEl.get_value();
                        if (cVal.is_int()) {
                            clipboardMethod = static_cast<int8_t>(cVal.get<int>());
                        } else if (cVal.is_string()) {
                            clipboardMethod = static_cast<int8_t>(_wtoi(cVal.get<std::wstring>().c_str()));
                        }
                    }
                    
                    if (!name.empty()) {
                        onAddApp(name, behaviorType, clipboardMethod);
                    }
                }
                el.set_value(sciter::value(L""));
                return true;
            }
            
            if (action == L"delete-app") {
                sciter::dom::element nameEl = root.find_first("#val-app-name");
                if (nameEl.is_valid()) {
                    std::wstring name = nameEl.get_value().get<std::wstring>();
                    if (!name.empty()) {
                        onDeleteApp(name);
                    }
                }
                el.set_value(sciter::value(L""));
                return true;
            }
            
            if (action == L"change-behavior") {
                sciter::dom::element nameEl = root.find_first("#val-app-name");
                sciter::dom::element behaviorEl = root.find_first("#val-behavior-type");
                
                if (nameEl.is_valid() && behaviorEl.is_valid()) {
                    std::wstring name = nameEl.get_value().get<std::wstring>();
                    int8_t behaviorType = 0;
                    sciter::value bVal = behaviorEl.get_value();
                    if (bVal.is_int()) {
                        behaviorType = static_cast<int8_t>(bVal.get<int>());
                    } else if (bVal.is_string()) {
                        behaviorType = static_cast<int8_t>(_wtoi(bVal.get<std::wstring>().c_str()));
                    }
                    
                    if (!name.empty()) {
                        onChangeBehavior(name, behaviorType);
                    }
                }
                el.set_value(sciter::value(L""));
                return true;
            }
            
            if (action == L"change-clipboard") {
                sciter::dom::element nameEl = root.find_first("#val-app-name");
                sciter::dom::element clipboardEl = root.find_first("#val-clipboard-method");
                
                if (nameEl.is_valid() && clipboardEl.is_valid()) {
                    std::wstring name = nameEl.get_value().get<std::wstring>();
                    int8_t clipboardMethod = -1;
                    sciter::value cVal = clipboardEl.get_value();
                    if (cVal.is_int()) {
                        clipboardMethod = static_cast<int8_t>(cVal.get<int>());
                    } else if (cVal.is_string()) {
                        clipboardMethod = static_cast<int8_t>(_wtoi(cVal.get<std::wstring>().c_str()));
                    }
                    
                    if (!name.empty()) {
                        onChangeClipboard(name, clipboardMethod);
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

void AppOverridesDialogSciter::onAddApp(const std::wstring& appName, int8_t behaviorType, int8_t clipboardMethod) {
    std::string utf8Name = wideStringToUtf8(appName);
    
    if (isDuplicate(utf8Name)) {
        MessageBoxW(get_hwnd(), 
            L"\u1EE8ng d\u1EE5ng n\u00E0y \u0111\u00E3 c\u00F3 trong danh s\u00E1ch!", 
            L"NextKey", 
            MB_OK | MB_ICONINFORMATION);
        return;
    }
    
    AppEntry entry;
    entry.exeName = utf8Name;
    entry.behaviorType = behaviorType;
    entry.clipboardMethod = clipboardMethod;
    m_appsList.push_back(entry);
    
    saveData();
    
    std::wstring wName = utf8ToWideString(utf8Name);
    call_function("addAppToList", wName.c_str(), (int)behaviorType, (int)clipboardMethod);
    call_function("clearInput");
    call_function("forceRefresh", sciter::value(true));
    
    forceForegroundWindow(get_hwnd());
}

void AppOverridesDialogSciter::onDeleteApp(const std::wstring& appName) {
    std::string utf8Name = wideStringToUtf8(appName);
    std::string lowerName = toLower(utf8Name);
    
    for (auto it = m_appsList.begin(); it != m_appsList.end(); ++it) {
        if (toLower(it->exeName) == lowerName) {
            m_appsList.erase(it);
            break;
        }
    }
    
    saveData();
    
    call_function("removeAppFromList", appName.c_str());
    call_function("forceRefresh");
}

void AppOverridesDialogSciter::onChangeBehavior(const std::wstring& appName, int8_t behaviorType) {
    std::string utf8Name = wideStringToUtf8(appName);
    std::string lowerName = toLower(utf8Name);
    
    for (auto& entry : m_appsList) {
        if (toLower(entry.exeName) == lowerName) {
            entry.behaviorType = behaviorType;
            break;
        }
    }
    
    saveData();
}

void AppOverridesDialogSciter::onChangeClipboard(const std::wstring& appName, int8_t clipboardMethod) {
    std::string utf8Name = wideStringToUtf8(appName);
    std::string lowerName = toLower(utf8Name);
    
    for (auto& entry : m_appsList) {
        if (toLower(entry.exeName) == lowerName) {
            entry.clipboardMethod = clipboardMethod;
            break;
        }
    }
    
    saveData();
}

// ===== Window Picker =====

void AppOverridesDialogSciter::startWindowPicking() {
    m_isPickingWindow = true;
    SetCapture(get_hwnd());
    
    HCURSOR hOriginalArrow = LoadCursor(NULL, IDC_ARROW);
    m_hSavedArrowCursor = CopyCursor(hOriginalArrow);
    
    HCURSOR hCross = LoadCursor(NULL, IDC_CROSS);
    HCURSOR hCrossCopy = CopyCursor(hCross);
    if (!SetSystemCursor(hCrossCopy, OCR_NORMAL)) {
        DestroyCursor(hCrossCopy);
    }
}

void AppOverridesDialogSciter::stopWindowPicking() {
    m_isPickingWindow = false;
    ReleaseCapture();
    
    if (m_hSavedArrowCursor) {
        SetSystemCursor(m_hSavedArrowCursor, OCR_NORMAL);
        m_hSavedArrowCursor = NULL;
    }
    
    forceForegroundWindow(get_hwnd());
}

std::string AppOverridesDialogSciter::getExeNameFromWindow(HWND hwnd) {
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

void AppOverridesDialogSciter::onAddPickedApp(const std::string& exeName) {
    if (exeName.find("NextKey") != std::string::npos || exeName.find("OpenKey") != std::string::npos) {
        MessageBoxW(get_hwnd(), 
            L"Kh\u00F4ng th\u1EC3 th\u00EAm NextKey v\u00E0o danh s\u00E1ch!", 
            L"NextKey", 
            MB_OK | MB_ICONWARNING);
        return;
    }
    
    if (isDuplicate(exeName)) {
        MessageBoxW(get_hwnd(), 
            L"\u1EE8ng d\u1EE5ng n\u00E0y \u0111\u00E3 c\u00F3 trong danh s\u00E1ch!", 
            L"NextKey", 
            MB_OK | MB_ICONINFORMATION);
        return;
    }
    
    // Default: None behavior, no clipboard override
    AppEntry entry;
    entry.exeName = exeName;
    entry.behaviorType = 0;
    entry.clipboardMethod = -1;
    m_appsList.push_back(entry);
    
    saveData();
    
    std::wstring wName = utf8ToWideString(exeName);
    call_function("addAppToList", wName.c_str(), 0, -1);
    call_function("clearInput");
    call_function("forceRefresh");
    
    forceForegroundWindow(get_hwnd());
}

// ===== Running Apps Dropdown =====

void AppOverridesDialogSciter::sendRunningAppsToJS() {
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
                exeName.find(L"NextKey") != std::wstring::npos ||
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
    
    // Convert to array for JS
    std::vector<sciter::value> appsArray;
    for (const auto& app : runningApps) {
        appsArray.push_back(sciter::value(utf8ToWideString(app).c_str()));
    }
    
    sciter::value arr = sciter::value::make_array(appsArray.size());
    for (size_t i = 0; i < appsArray.size(); i++) {
        arr.set_item(i, appsArray[i]);
    }
    
    call_function("setRunningApps", arr);
}
