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
#include "ClipboardAppsDialogSciter.h"
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

// ConfigManager keys
static const char* CFG_SECTION_CLIPBOARD = "clipboardApps";
static const char* CFG_APPS_LIST = "apps";

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
struct ACCENT_POLICY_CLIPBOARD {
    int AccentState;
    int AccentFlags;
    int GradientColor;
    int AnimationId;
};

struct WINDOWCOMPOSITIONATTRIBDATA_CLIPBOARD {
    int Attrib;
    void* pvData;
    size_t cbData;
};

enum ACCENT_STATE_CLIPBOARD {
    ACCENT_DISABLED_CLIPBOARD = 0,
    ACCENT_ENABLE_BLURBEHIND_CLIPBOARD = 3,
    ACCENT_ENABLE_ACRYLICBLURBEHIND_CLIPBOARD = 4
};

// ===== Helper Functions =====

std::string ClipboardAppsDialogSciter::toLower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(), 
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

bool ClipboardAppsDialogSciter::isDuplicate(const std::string& exeName) {
    std::string lowerName = toLower(exeName);
    for (const auto& entry : m_appsList) {
        if (toLower(entry.exeName) == lowerName) {
            return true;
        }
    }
    return false;
}

// ===== Constructor =====

ClipboardAppsDialogSciter::ClipboardAppsDialogSciter() 
    : sciter::window(SW_POPUP | SW_ALPHA | SW_ENABLE_DEBUG, RECT{ 0, 0, 420, 520 }) {
    
    // Load HTML
#ifdef NDEBUG
    if (!load(WSTR("this://app/clipboardapps/clipboardapps.html"))) {
        MessageBoxW(NULL, L"Failed to load clipboardapps.html from resources", L"Error", MB_OK | MB_ICONERROR);
        return;
    }
#else
    WCHAR exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    WCHAR* lastSlash = wcsrchr(exePath, L'\\');
    if (lastSlash) *lastSlash = L'\0';
    
    WCHAR htmlPath[MAX_PATH];
    swprintf_s(htmlPath, MAX_PATH, L"%s\\Resources\\Sciter\\clipboardapps\\clipboardapps.html", exePath);
    
    if (!load(htmlPath)) {
        MessageBoxW(NULL, htmlPath, L"Failed to load clipboardapps.html", MB_OK | MB_ICONERROR);
        return;
    }
#endif
    
    expand();
    
    SetWindowTextW(get_hwnd(), L"C\u1EA5u h\u00ECnh Clipboard");
    
    int scaledWidth, scaledHeight;
    ScaleHelper::getScaledSize(420, 480, scaledWidth, scaledHeight);
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
    SetWindowSubclass(get_hwnd(), ClipboardAppsDialogSciter::SubclassProc, 1, (DWORD_PTR)this);
}

ClipboardAppsDialogSciter::~ClipboardAppsDialogSciter() {
}

void ClipboardAppsDialogSciter::show() {
    ShowWindow(get_hwnd(), SW_SHOW);
    SetForegroundWindow(get_hwnd());
}

void ClipboardAppsDialogSciter::enableAcrylicEffect() {
    HWND hwnd = get_hwnd();
    SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);

    HMODULE hUser = GetModuleHandle(L"user32.dll");
    if (hUser) {
        typedef BOOL(WINAPI* pSetWindowCompositionAttribute)(HWND, WINDOWCOMPOSITIONATTRIBDATA_CLIPBOARD*);
        auto SetWindowCompositionAttribute = 
            (pSetWindowCompositionAttribute)GetProcAddress(hUser, "SetWindowCompositionAttribute");

        if (SetWindowCompositionAttribute) {
            ACCENT_POLICY_CLIPBOARD policy = { 0 };
            policy.AccentState = ACCENT_ENABLE_BLURBEHIND_CLIPBOARD;
            policy.AccentFlags = 0;
            policy.GradientColor = 0x00000000;
            policy.AnimationId = 0;

            WINDOWCOMPOSITIONATTRIBDATA_CLIPBOARD data = { 0 };
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

LRESULT CALLBACK ClipboardAppsDialogSciter::SubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    ClipboardAppsDialogSciter* dialog = reinterpret_cast<ClipboardAppsDialogSciter*>(dwRefData);
    
    if (msg == WM_CLOSE) {
        // NOTE: Must use ExitProcess(0) for Sciter subprocesses!
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

void ClipboardAppsDialogSciter::loadAppsList() {
    m_appsList.clear();
    
    auto& config = ConfigManager::instance();
    config.init();
    
    // Load clipboard apps from config
    auto apps = config.getClipboardApps();
    for (const auto& app : apps) {
        ClipboardAppEntry entry;
        entry.exeName = app.exeName;
        entry.method = static_cast<ClipboardMethod>(app.method);
        entry.delayMs = app.delayMs;
        m_appsList.push_back(entry);
    }
    
    DEBUG_LOG(("Loaded " + std::to_string(m_appsList.size()) + " clipboard apps\n").c_str());
}

void ClipboardAppsDialogSciter::saveData() {
    // Convert to ConfigManager format
    std::vector<ConfigManager::ClipboardAppConfig> configApps;
    for (const auto& entry : m_appsList) {
        ConfigManager::ClipboardAppConfig cfg;
        cfg.exeName = entry.exeName;
        cfg.method = static_cast<int>(entry.method);
        cfg.delayMs = entry.delayMs;
        configApps.push_back(cfg);
    }
    
    // Save to ConfigManager
    auto& config = ConfigManager::instance();
    config.setClipboardApps(configApps);
    config.save();
    
    // Notify main process to reload
    HWND mainWnd = FindWindow(_T("OpenKeyVietnameseInputMethod"), NULL);
    if (mainWnd) {
        PostMessage(mainWnd, WM_USER + 101, 0, 0);
    }
    
    DEBUG_LOG(("Saved " + std::to_string(m_appsList.size()) + " clipboard apps\n").c_str());
}

void ClipboardAppsDialogSciter::fillAppsListUI() {
    call_function("clearAppList");
    
    for (const auto& entry : m_appsList) {
        std::wstring wName = utf8ToWideString(entry.exeName);
        int methodInt = static_cast<int>(entry.method);
        call_function("addAppToList", wName.c_str(), methodInt, entry.delayMs);
    }
    
    call_function("forceRefresh");
}

// ===== Event Handling =====

bool ClipboardAppsDialogSciter::handle_event(HELEMENT he, BEHAVIOR_EVENT_PARAMS& params) {
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
                sciter::dom::element methodEl = root.find_first("#val-paste-method");
                sciter::dom::element delayEl = root.find_first("#val-delay-ms");
                
                if (nameEl.is_valid() && methodEl.is_valid()) {
                    std::wstring name = nameEl.get_value().get<std::wstring>();
                    
                    int methodInt = 0;
                    sciter::value methodVal = methodEl.get_value();
                    if (methodVal.is_int()) {
                        methodInt = methodVal.get<int>();
                    } else if (methodVal.is_string()) {
                        methodInt = _wtoi(methodVal.get<std::wstring>().c_str());
                    }
                    
                    int delayMs = 0;
                    if (delayEl.is_valid()) {
                        sciter::value delayVal = delayEl.get_value();
                        if (delayVal.is_int()) {
                            delayMs = delayVal.get<int>();
                        } else if (delayVal.is_string()) {
                            delayMs = _wtoi(delayVal.get<std::wstring>().c_str());
                        }
                    }
                    
                    if (!name.empty()) {
                        onAddApp(name, static_cast<ClipboardMethod>(methodInt), delayMs);
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
            
            if (action == L"change-method") {
                sciter::dom::element nameEl = root.find_first("#val-app-name");
                sciter::dom::element methodEl = root.find_first("#val-paste-method");
                
                if (nameEl.is_valid() && methodEl.is_valid()) {
                    std::wstring name = nameEl.get_value().get<std::wstring>();
                    int methodInt = 0;
                    sciter::value methodVal = methodEl.get_value();
                    if (methodVal.is_int()) {
                        methodInt = methodVal.get<int>();
                    } else if (methodVal.is_string()) {
                        methodInt = _wtoi(methodVal.get<std::wstring>().c_str());
                    }
                    
                    if (!name.empty()) {
                        onChangeMethod(name, static_cast<ClipboardMethod>(methodInt));
                    }
                }
                el.set_value(sciter::value(L""));
                return true;
            }
            
            if (action == L"change-delay") {
                sciter::dom::element nameEl = root.find_first("#val-app-name");
                sciter::dom::element delayEl = root.find_first("#val-delay-ms");
                
                if (nameEl.is_valid() && delayEl.is_valid()) {
                    std::wstring name = nameEl.get_value().get<std::wstring>();
                    int delayMs = 0;
                    sciter::value delayVal = delayEl.get_value();
                    if (delayVal.is_int()) {
                        delayMs = delayVal.get<int>();
                    } else if (delayVal.is_string()) {
                        delayMs = _wtoi(delayVal.get<std::wstring>().c_str());
                    }
                    
                    if (!name.empty()) {
                        onChangeDelay(name, delayMs);
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

void ClipboardAppsDialogSciter::onAddApp(const std::wstring& appName, ClipboardMethod method, int delayMs) {
    std::string utf8Name = wideStringToUtf8(appName);
    
    if (isDuplicate(utf8Name)) {
        MessageBoxW(get_hwnd(), 
            L"\u1EE8ng d\u1EE5ng n\u00E0y \u0111\u00E3 c\u00F3 trong danh s\u00E1ch!", 
            L"OpenKey", 
            MB_OK | MB_ICONINFORMATION);
        return;
    }
    
    // Clamp delay
    if (delayMs < 0) delayMs = 0;
    if (delayMs > 500) delayMs = 500;
    
    ClipboardAppEntry entry;
    entry.exeName = utf8Name;
    entry.method = method;
    entry.delayMs = delayMs;
    m_appsList.push_back(entry);
    
    saveData();
    
    call_function("addAppToList", appName.c_str(), static_cast<int>(method), delayMs);
    call_function("clearInput");
    call_function("forceRefresh", sciter::value(true));
    
    forceForegroundWindow(get_hwnd());
}

void ClipboardAppsDialogSciter::onDeleteApp(const std::wstring& appName) {
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

void ClipboardAppsDialogSciter::onChangeMethod(const std::wstring& appName, ClipboardMethod newMethod) {
    std::string utf8Name = wideStringToUtf8(appName);
    std::string lowerName = toLower(utf8Name);
    
    for (auto& entry : m_appsList) {
        if (toLower(entry.exeName) == lowerName) {
            entry.method = newMethod;
            break;
        }
    }
    
    saveData();
}

void ClipboardAppsDialogSciter::onChangeDelay(const std::wstring& appName, int delayMs) {
    std::string utf8Name = wideStringToUtf8(appName);
    std::string lowerName = toLower(utf8Name);
    
    // Clamp delay
    if (delayMs < 0) delayMs = 0;
    if (delayMs > 500) delayMs = 500;
    
    for (auto& entry : m_appsList) {
        if (toLower(entry.exeName) == lowerName) {
            entry.delayMs = delayMs;
            break;
        }
    }
    
    saveData();
}

// ===== Window Picker =====

void ClipboardAppsDialogSciter::startWindowPicking() {
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

void ClipboardAppsDialogSciter::stopWindowPicking() {
    m_isPickingWindow = false;
    ReleaseCapture();
    
    if (m_hSavedArrowCursor) {
        SetSystemCursor(m_hSavedArrowCursor, OCR_NORMAL);
        m_hSavedArrowCursor = NULL;
    }
    
    forceForegroundWindow(get_hwnd());
}

std::string ClipboardAppsDialogSciter::getExeNameFromWindow(HWND hwnd) {
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

void ClipboardAppsDialogSciter::onAddPickedApp(const std::string& exeName) {
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
    
    // Default: ShiftInsert with no delay
    ClipboardAppEntry entry;
    entry.exeName = exeName;
    entry.method = ClipboardMethod::ShiftInsert;
    entry.delayMs = 0;
    m_appsList.push_back(entry);
    
    saveData();
    
    std::wstring wName = utf8ToWideString(exeName);
    call_function("addAppToList", wName.c_str(), 0, 0);
    call_function("clearInput");
    call_function("forceRefresh");
    
    forceForegroundWindow(get_hwnd());
}

// ===== Running Apps Dropdown =====

void ClipboardAppsDialogSciter::sendRunningAppsToJS() {
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
