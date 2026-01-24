/*----------------------------------------------------------
NextKey - The Modern Vietnamese Input Method Engine.
Based on OpenKey architecture.

Copyright (C) 2026 NextKey Project
Author: Mai Tan Phat
License: GPL (Inherited from OpenKey)
-----------------------------------------------------------*/
#include "stdafx.h"

// Undefine conflicting macros from engine/platforms/win32.h (included via stdafx.h -> Engine.h)
// These conflict with Sciter's KEY_EVENTS enum in sciter-x-behavior.h
#ifdef KEY_DOWN
#undef KEY_DOWN
#endif
#ifdef KEY_UP
#undef KEY_UP
#endif
#ifdef KEY_CHAR
#undef KEY_CHAR
#endif

#include "ConvertToolDialogSciter.h"
#include "OpenKeyHelper.h"
#include "OpenKeyManager.h"
#include "ConfigManager.h"
#include "ConfigIntent.h"
#include "sciter-x-dom.hpp"
#include "../../../engine/Engine.h"
#include "../../../engine/ConvertTool.h"
#include "../../../engine/Vietnamese.h"  // For initKeyCodeToChar()
#include "AppDelegate.h"  // For vQuickConvertAutoPaste extern
#include <dwmapi.h>
#include <windowsx.h>
#include <commctrl.h>
#include <fstream>
#include <sstream>
#include <iterator>
#include <commdlg.h>
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")

#include "ScaleHelper.h"

#define CONVERT_TOOL_WINDOW_TITLE L"C\u00F4ng c\u1EE5 chuy\u1EC3n m\u00E3"

// Hotkey modifier masks
#define HOTKEY_CTRL_MASK  0x100
#define HOTKEY_ALT_MASK   0x200
#define HOTKEY_WIN_MASK   0x400
#define HOTKEY_SHIFT_MASK 0x800

ConvertToolDialogSciter::ConvertToolDialogSciter()
    : sciter::window(SW_POPUP | SW_ALPHA | SW_ENABLE_DEBUG, RECT{0, 0, 400, 380}) {
    
    // Initialize engine data needed for convertUtil() - subprocess starts without engine init
    initKeyCodeToChar();
    
    // Initialize ConfigManager for subprocess (reads from config.toml)
    ConfigManager::instance().init();
    
    // Load settings from registry (subprocess starts fresh)
    // Load settings from ConfigManager (fix persistence)
    convertToolFromCode = ConfigManager::instance().getInt("convertTool", "fromCode", 0);
    convertToolToCode = ConfigManager::instance().getInt("convertTool", "toCode", 0);
    convertToolHotKey = ConfigManager::instance().getInt("convertTool", "hotkey", 0);
    convertToolToAllCaps = ConfigManager::instance().getBool("convertTool", "toAllCaps", false) ? 1 : 0;
    convertToolToAllNonCaps = ConfigManager::instance().getBool("convertTool", "toAllNonCaps", false) ? 1 : 0;
    convertToolRemoveMark = ConfigManager::instance().getBool("convertTool", "removeMark", false) ? 1 : 0;
    convertToolToCapsEachWord = ConfigManager::instance().getBool("convertTool", "toCapsEachWord", false) ? 1 : 0;
    convertToolToCapsFirstLetter = ConfigManager::instance().getBool("convertTool", "toCapsFirstLetter", false) ? 1 : 0;
    convertToolDontAlertWhenCompleted = ConfigManager::instance().getBool("convertTool", "dontAlertCompleted", false) ? 1 : 0;
    vQuickConvertAutoPaste = ConfigManager::instance().getBool("convertTool", "autoPasteReselect", false) ? 1 : 0;
    vQuickConvertSequential = ConfigManager::instance().getBool("convertTool", "sequentialMode", false) ? 1 : 0;
    
    // Load HTML
#ifdef NDEBUG
    // Release: load from embedded resources
    if (!load(WSTR("this://app/convert-tool/convert-tool.html"))) {
        MessageBoxW(NULL, L"Failed to load convert-tool.html from resources", L"Error", MB_OK | MB_ICONERROR);
        return;
    }
#else
    // Debug: load from file
    WCHAR exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    WCHAR* lastSlash = wcsrchr(exePath, L'\\');
    if (lastSlash) *lastSlash = L'\0';
    
    WCHAR htmlPath[MAX_PATH];
    swprintf_s(htmlPath, MAX_PATH, L"%s\\Resources\\Sciter\\convert-tool\\convert-tool.html", exePath);
    
    if (!load(htmlPath)) {
        MessageBoxW(NULL, htmlPath, L"Failed to load convert-tool.html", MB_OK | MB_ICONERROR);
        return;
    }
#endif

    // Show the window
    expand();
    
    // Subclass window
    SetWindowSubclass(get_hwnd(), ConvertToolDialogSciter::SubclassProc, 1, (DWORD_PTR)this);
    
    // Set window title
    SetWindowTextW(get_hwnd(), CONVERT_TOOL_WINDOW_TITLE);
    
    // Auto-fit window
    sciter::dom::element rootEl = this->root();
    sciter::dom::element container = rootEl.find_first(".container");
    if (container) {
        // DOM measurements are already in screen pixels (DPI-scaled by Sciter)
        RECT contentRect = container.get_location(CONTENT_BOX);
        double dpiScale = ScaleHelper::getDpiScale();
        
        // Scale minimum constraints, not DOM measurements
        int contentWidth = max(contentRect.right - contentRect.left, (int)(400 * dpiScale));
        int contentHeight = max(contentRect.bottom - contentRect.top, (int)(200 * dpiScale));
        SetWindowPos(get_hwnd(), NULL, 0, 0, contentWidth, contentHeight, SWP_NOMOVE | SWP_NOZORDER);
    }
    
    // Center on screen
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    RECT rc;
    GetWindowRect(get_hwnd(), &rc);
    int x = (screenWidth - (rc.right - rc.left)) / 2;
    int y = (screenHeight - (rc.bottom - rc.top)) / 2;
    SetWindowPos(get_hwnd(), HWND_NOTOPMOST, x, y, 0, 0, SWP_NOSIZE);
    
    // Enable blur
    enableAcrylicEffect();
    
    // Load initial settings to UI
    loadSettings();
}

// Acrylic blur effect
void ConvertToolDialogSciter::enableAcrylicEffect() {
    HWND hwnd = get_hwnd();
    SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);

    struct ACCENT_POLICY { int AccentState; int AccentFlags; int GradientColor; int AnimationId; };
    struct WINDOWCOMPOSITIONATTRIBDATA { int Attrib; void* pvData; size_t cbData; };
    
    HMODULE hUser = GetModuleHandle(L"user32.dll");
    if (hUser) {
        typedef BOOL(WINAPI* pSetWindowCompositionAttribute)(HWND, WINDOWCOMPOSITIONATTRIBDATA*);
        auto SetWindowCompositionAttribute = 
            (pSetWindowCompositionAttribute)GetProcAddress(hUser, "SetWindowCompositionAttribute");

        if (SetWindowCompositionAttribute) {
            ACCENT_POLICY policy = { 3, 0, 0x00000000, 0 };  // BLURBEHIND
            WINDOWCOMPOSITIONATTRIBDATA data = { 19, &policy, sizeof(policy) };
            SetWindowCompositionAttribute(hwnd, &data);
        }
    }

    // Round corners (Windows 11)
    int preference = 2;  // DWMWCP_ROUND
    DwmSetWindowAttribute(hwnd, 33, &preference, sizeof(preference));
}

LRESULT CALLBACK ConvertToolDialogSciter::SubclassProc(HWND hwnd, UINT msg, WPARAM wParam,
                                                        LPARAM lParam, UINT_PTR uIdSubclass,
                                                        DWORD_PTR dwRefData) {
    if (msg == WM_CLOSE) {
        ExitProcess(0);
        return 0;
    }
    
    // Drag zone
    if (msg == WM_NCHITTEST) {
        LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
        if (result == HTCLIENT) {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(hwnd, &pt);
            RECT winRect;
            GetClientRect(hwnd, &winRect);
            int closeButtonZone = winRect.right - 40;
            if (pt.y < 36 && pt.x < closeButtonZone) {
                return HTCAPTION;
            }
        }
        return result;
    }
    
    // IPC: Bring to foreground
    if (msg == WM_USER + 107) {
        return OpenKeyHelper::handleIPCForeground(hwnd);
    }
    
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void ConvertToolDialogSciter::show() {
    ShowWindow(get_hwnd(), SW_SHOW);
    SetForegroundWindow(get_hwnd());
}

void ConvertToolDialogSciter::loadSettings() {
    sciter::dom::element root = this->root();
    
    // Helper to set toggle state
    auto setToggle = [&root](const char* id, int value) {
        sciter::dom::element el = root.find_first(id);
        if (el) {
            if (value) el.set_attribute("class", L"toggle-switch-small checked");
            else el.set_attribute("class", L"toggle-switch-small");
        }
    };
    
    setToggle("#toggle-all-caps", convertToolToAllCaps);
    setToggle("#toggle-non-caps", convertToolToAllNonCaps);
    setToggle("#toggle-remove-mark", convertToolRemoveMark);
    setToggle("#toggle-caps-first", convertToolToCapsFirstLetter);
    setToggle("#toggle-caps-each", convertToolToCapsEachWord);
    setToggle("#toggle-alert", !convertToolDontAlertWhenCompleted);  // Inverted
    setToggle("#toggle-auto-paste", vQuickConvertAutoPaste);
    setToggle("#toggle-sequential", vQuickConvertSequential);
    
    // Hotkey toggles
    setToggle("#hotkey-ctrl", (convertToolHotKey & 0x100) ? 1 : 0);
    setToggle("#hotkey-alt", (convertToolHotKey & 0x200) ? 1 : 0);
    setToggle("#hotkey-win", (convertToolHotKey & 0x400) ? 1 : 0);
    setToggle("#hotkey-shift", (convertToolHotKey & 0x800) ? 1 : 0);
    
    // Hotkey character
    sciter::dom::element hotkeyChar = root.find_first("#hotkey-char");
    if (hotkeyChar) {
        int charCode = (convertToolHotKey >> 24) & 0xFF;
        if (charCode > 0) {
            std::wstring charStr;
            if (charCode == 32) {
                charStr = L"Space";
            } else {
                charStr = std::wstring(1, (wchar_t)charCode);
            }
            hotkeyChar.set_value(sciter::value(charStr));
        }
    }
    
    // Encoding dropdowns
    sciter::dom::element sourceEnc = root.find_first("#source-encoding");
    sciter::dom::element destEnc = root.find_first("#dest-encoding");
    if (sourceEnc) sourceEnc.set_value(sciter::value((int)convertToolFromCode));
    if (destEnc) destEnc.set_value(sciter::value((int)convertToolToCode));
    
    // Note: Theme and opacity are applied in DOCUMENT_READY handler where DOM is guaranteed to be ready
}

void ConvertToolDialogSciter::onConvert() {
    sciter::dom::element root = this->root();
    sciter::dom::element uiModeEl = root.find_first("#val-ui-mode");
    std::wstring uiMode = L"clipboard";
    if (uiModeEl) {
        sciter::value val = uiModeEl.get_value();
        if (val.is_string()) uiMode = val.get<std::wstring>();
    }

    bool result = false;
    if (uiMode == L"file") {
        sciter::dom::element srcEl = root.find_first("#val-source-file");
        sciter::dom::element dstEl = root.find_first("#val-dest-file");
        std::wstring srcPath = srcEl ? srcEl.get_value().get<std::wstring>() : L"";
        std::wstring dstPath = dstEl ? dstEl.get_value().get<std::wstring>() : L"";

        if (srcPath.empty() || dstPath.empty()) {
            MessageBoxW(get_hwnd(), L"Vui l\u00F2ng ch\u1ECDn file ngu\u1ED3n v\u00E0 file \u0111\u00EDch!", CONVERT_TOOL_WINDOW_TITLE, MB_OK | MB_ICONWARNING);
            return;
        }

        // Read source file
        std::ifstream srcFile(srcPath.c_str(), std::ios::binary);
        if (!srcFile.is_open()) {
            MessageBoxW(get_hwnd(), L"Kh\u00F4ng th\u1EC3 m\u1EDF file ngu\u1ED3n!", CONVERT_TOOL_WINDOW_TITLE, MB_OK | MB_ICONERROR);
            return;
        }

        std::stringstream ss;
        ss << srcFile.rdbuf();
        std::string content = ss.str();
        srcFile.close();

        // Convert
        std::string converted = convertUtil(content);

        // Write to destination
        std::ofstream dstFile(dstPath.c_str(), std::ios::binary);
        if (!dstFile.is_open()) {
            MessageBoxW(get_hwnd(), L"Kh\u00F4ng th\u1EC3 m\u1EDF file \u0111\u00EDch \u0111\u1EC3 ghi!", CONVERT_TOOL_WINDOW_TITLE, MB_OK | MB_ICONERROR);
            return;
        }

        dstFile.write(converted.c_str(), converted.size());
        dstFile.close();
        result = true;
    } else {
        // Clipboard mode
        result = OpenKeyHelper::quickConvert();
    }
    
    // Show success notification if enabled and conversion succeeded
    if (result && !convertToolDontAlertWhenCompleted) {
        MessageBoxW(get_hwnd(), L"Chuy\u1EC3n m\u00E3 ho\u00E0n t\u1EA5t!", CONVERT_TOOL_WINDOW_TITLE, MB_OK | MB_ICONINFORMATION);
    }
}

void ConvertToolDialogSciter::recalcWindowSize() {
    // Get current window position
    RECT rc;
    GetWindowRect(get_hwnd(), &rc);
    int x = rc.left;
    int y = rc.top;
    
    sciter::dom::element rootEl = root();
    rootEl.update(true); // Force layout calculation
    
    sciter::dom::element container = rootEl.find_first(".container");
    if (container) {
        // DOM measurements are already in screen pixels (DPI-scaled by Sciter)
        RECT r = container.get_location(CONTENT_BOX);
        double dpiScale = ScaleHelper::getDpiScale();
        
        // Scale minimum constraints, not DOM measurements
        int w = max(r.right - r.left, (int)(420 * dpiScale));
        int h = max(r.bottom - r.top, (int)(200 * dpiScale));
        
        SetWindowPos(get_hwnd(), NULL, x, y, w, h, SWP_NOZORDER);
        
        // Re-apply blur as it might be lost after resize
        enableAcrylicEffect();
    }
}

void ConvertToolDialogSciter::onSelectFile(bool isSource) {
    WCHAR szFile[MAX_PATH] = { 0 };
    OPENFILENAMEW ofn = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = get_hwnd();
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(WCHAR);
    ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0Rich Text Format (*.rtf)\0*.rtf\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    BOOL result = FALSE;
    if (isSource) {
        ofn.lpstrTitle = L"Ch\u1ECDn file ngu\u1ED3n";
        result = GetOpenFileNameW(&ofn);
    } else {
        ofn.lpstrTitle = L"Ch\u1ECDn file \u0111\u00EDch";
        // Remove OFN_FILEMUSTEXIST for destination - file may not exist yet
        // Remove OFN_OVERWRITEPROMPT - user just wants to pick path, not confirm overwrite
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
        result = GetSaveFileNameW(&ofn);
    }

    if (result) {
        std::wstring filePath = szFile;
        std::wstring ext = L"";
        size_t dotPos = filePath.find_last_of(L".");
        if (dotPos != std::wstring::npos) {
            ext = filePath.substr(dotPos);
            // Lowercase extension for check
            for (auto& c : ext) c = towlower(c);
        }

        if (ext != L".txt" && ext != L".rtf") {
            MessageBoxW(get_hwnd(), L"\u0110\u1ECBnh d\u1EA1ng file kh\u00F4ng h\u1ED7 tr\u1EE3! Ch\u1EC9 h\u1ED7 tr\u1EE3 .txt ho\u1EB7c .rtf", CONVERT_TOOL_WINDOW_TITLE, MB_OK | MB_ICONWARNING);
            return;
        }

        // Update UI
        sciter::dom::element rootEl = root();
        sciter::dom::element displayEl = rootEl.find_first(isSource ? "#source-file-path" : "#dest-file-path");
        sciter::dom::element hiddenEl = rootEl.find_first(isSource ? "#val-source-file" : "#val-dest-file");

        if (displayEl) {
            // Smart path truncation: show full if short, or ...\\partial\\path if long
            const size_t MAX_DISPLAY_CHARS = 35; // Approximate fit for input width
            std::wstring displayName = filePath;
            
            if (filePath.length() > MAX_DISPLAY_CHARS) {
                // Find a good truncation point - try to keep meaningful path segments
                size_t charsToKeep = MAX_DISPLAY_CHARS - 3; // Reserve space for "..."
                std::wstring endPart = filePath.substr(filePath.length() - charsToKeep);
                
                // Try to cut at a path separator for cleaner display
                size_t firstSlash = endPart.find_first_of(L"\\/");
                if (firstSlash != std::wstring::npos && firstSlash < endPart.length() - 5) {
                    endPart = endPart.substr(firstSlash);
                }
                displayName = L"..." + endPart;
            }
            
            displayEl.set_text(displayName.c_str());
            // Set title attribute for tooltip with full path
            displayEl.set_attribute("title", filePath.c_str());
            displayEl.update(true); // Force redraw
        }
        if (hiddenEl) {
            hiddenEl.set_value(sciter::value(filePath));
        }
    }
}

// Notify main process to reload settings
static void notifyMainProcess() {
    // Central Writer: Send convert tool settings via IPC
    HWND mainWnd = FindWindow(APP_CLASS, NULL);
    if (!mainWnd) return;
    
    ConvertToolPayload payload = {};
    payload.hotkey = convertToolHotKey;
    payload.fromCode = convertToolFromCode;
    payload.toCode = convertToolToCode;
    payload.toAllCaps = convertToolToAllCaps ? 1 : 0;
    payload.toAllNonCaps = convertToolToAllNonCaps ? 1 : 0;
    payload.removeMark = convertToolRemoveMark ? 1 : 0;
    payload.toCapsEachWord = convertToolToCapsEachWord ? 1 : 0;
    payload.toCapsFirstLetter = convertToolToCapsFirstLetter ? 1 : 0;
    payload.dontAlertCompleted = convertToolDontAlertWhenCompleted ? 1 : 0;
    payload.autoPasteReselect = vQuickConvertAutoPaste ? 1 : 0;
    payload.sequentialMode = vQuickConvertSequential ? 1 : 0;
    
    auto buffer = serializeConvertTool(payload);
    if (sendConfigIntent(mainWnd, ConfigIntentType::UPDATE_CONVERT_TOOL, buffer)) {
        LOG(L"[ConvertToolDialog] Sent settings via IPC\n");
    } else {
        LOG(L"[ConvertToolDialog] Failed to send settings via IPC\n");
    }
}


bool ConvertToolDialogSciter::handle_event(HELEMENT he, BEHAVIOR_EVENT_PARAMS& params) {
    if (sciter::window::handle_event(he, params))
        return true;
    
    // DOCUMENT_READY - apply theme and opacity here (DOM is ready)
    if (params.cmd == DOCUMENT_READY) {
        // Apply dark/light theme
        sciter::dom::element root = this->root();
        bool isDarkMode = OpenKeyHelper::isWindowsDarkMode();
        sciter::dom::element body = root.find_first("body");
        if (body) {
            if (isDarkMode) {
                body.set_attribute("class", L"dark");
            } else {
                body.remove_attribute("class");
            }
        }
        
        // Apply background opacity from ConfigManager
        int bgOpacity = ConfigManager::instance().getInt("system", "backgroundOpacity", 80);
        
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
    
    // Button clicks
    if (params.cmd == BUTTON_CLICK) {
        sciter::dom::element el(params.heTarget);
        std::wstring id = el.get_attribute("id");
        
        if (id == L"btn-close") {
            PostMessage(get_hwnd(), WM_CLOSE, 0, 0);
            return true;
        }
        
        if (id == L"btn-convert") {
            onConvert();
            return true;
        }
        
    }
    
    // Handle VALUE_CHANGED for toggles, dropdowns, etc.
    if (params.cmd == VALUE_CHANGED) {
        sciter::dom::element el(params.heTarget);
        std::wstring id = el.get_attribute("id");
        
        // Action from JS (close button, file selection)
        if (id == L"val-action") {
            sciter::value val = el.get_value();
            std::wstring action = val.is_string() ? val.get<std::wstring>() : L"";
            if (action == L"close") {
                PostMessage(get_hwnd(), WM_CLOSE, 0, 0);
                return true;
            }
            if (action == L"select-source-file") {
                onSelectFile(true);
                return true;
            }
            if (action == L"select-dest-file") {
                onSelectFile(false);
                return true;
            }
        }

        // UI Mode Change - trigger resize
        if (id == L"val-ui-mode") {
            recalcWindowSize();
            return true;
        }
        
        // Option toggles
        if (id == L"val-toggle-all-caps") {
            sciter::value val = el.get_value();
            std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
            convertToolToAllCaps = (strVal == L"1") ? 1 : 0;
            APP_SET_DATA(convertToolToAllCaps, convertToolToAllCaps);
            notifyMainProcess();
            return true;
        }
        
        if (id == L"val-toggle-non-caps") {
            sciter::value val = el.get_value();
            std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
            convertToolToAllNonCaps = (strVal == L"1") ? 1 : 0;
            APP_SET_DATA(convertToolToAllNonCaps, convertToolToAllNonCaps);
            notifyMainProcess();
            return true;
        }
        
        if (id == L"val-toggle-remove-mark") {
            sciter::value val = el.get_value();
            std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
            convertToolRemoveMark = (strVal == L"1") ? 1 : 0;
            APP_SET_DATA(convertToolRemoveMark, convertToolRemoveMark);
            notifyMainProcess();
            return true;
        }
        
        if (id == L"val-toggle-caps-first") {
            sciter::value val = el.get_value();
            std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
            convertToolToCapsFirstLetter = (strVal == L"1") ? 1 : 0;
            APP_SET_DATA(convertToolToCapsFirstLetter, convertToolToCapsFirstLetter);
            notifyMainProcess();
            return true;
        }
        
        if (id == L"val-toggle-caps-each") {
            sciter::value val = el.get_value();
            std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
            convertToolToCapsEachWord = (strVal == L"1") ? 1 : 0;
            APP_SET_DATA(convertToolToCapsEachWord, convertToolToCapsEachWord);
            notifyMainProcess();
            return true;
        }
        
        if (id == L"val-toggle-alert") {
            sciter::value val = el.get_value();
            std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
            // Inverted: toggle ON = alert ON = DontAlert = 0
            convertToolDontAlertWhenCompleted = (strVal == L"1") ? 0 : 1;
            APP_SET_DATA(convertToolDontAlertWhenCompleted, convertToolDontAlertWhenCompleted);
            notifyMainProcess();
            return true;
        }
        
        if (id == L"val-toggle-auto-paste") {
            sciter::value val = el.get_value();
            std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
            vQuickConvertAutoPaste = (strVal == L"1") ? 1 : 0;
            
            // If auto-paste turned OFF, also turn off sequential mode
            if (!vQuickConvertAutoPaste && vQuickConvertSequential) {
                vQuickConvertSequential = 0;
                // Update sequential toggle UI
                sciter::dom::element rootEl = this->root();
                sciter::dom::element seqToggle = rootEl.find_first("#toggle-sequential");
                if (seqToggle) seqToggle.set_attribute("class", L"toggle-switch-small");
            }
            
            notifyMainProcess();
            return true;
        }
        
        if (id == L"val-toggle-sequential") {
            // Only allow enabling if auto-paste is ON
            if (!vQuickConvertAutoPaste) {
                return true;  // Ignore - parent option is OFF
            }
            sciter::value val = el.get_value();
            std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
            vQuickConvertSequential = (strVal == L"1") ? 1 : 0;
            notifyMainProcess();
            return true;
        }
        
        // Hotkey modifier toggles
        if (id == L"val-hotkey-ctrl") {
            sciter::value val = el.get_value();
            std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
            bool checked = (strVal == L"1");
            convertToolHotKey &= (~HOTKEY_CTRL_MASK);
            if (checked) convertToolHotKey |= HOTKEY_CTRL_MASK;
            APP_SET_DATA(convertToolHotKey, convertToolHotKey);
            notifyMainProcess();
            return true;
        }
        
        if (id == L"val-hotkey-alt") {
            sciter::value val = el.get_value();
            std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
            bool checked = (strVal == L"1");
            convertToolHotKey &= (~HOTKEY_ALT_MASK);
            if (checked) convertToolHotKey |= HOTKEY_ALT_MASK;
            APP_SET_DATA(convertToolHotKey, convertToolHotKey);
            notifyMainProcess();
            return true;
        }
        
        if (id == L"val-hotkey-win") {
            sciter::value val = el.get_value();
            std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
            bool checked = (strVal == L"1");
            convertToolHotKey &= (~HOTKEY_WIN_MASK);
            if (checked) convertToolHotKey |= HOTKEY_WIN_MASK;
            APP_SET_DATA(convertToolHotKey, convertToolHotKey);
            notifyMainProcess();
            return true;
        }
        
        if (id == L"val-hotkey-shift") {
            sciter::value val = el.get_value();
            std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
            bool checked = (strVal == L"1");
            convertToolHotKey &= (~HOTKEY_SHIFT_MASK);
            if (checked) convertToolHotKey |= HOTKEY_SHIFT_MASK;
            APP_SET_DATA(convertToolHotKey, convertToolHotKey);
            notifyMainProcess();
            return true;
        }
        
        // Hotkey character input
        if (id == L"hotkey-char") {
            sciter::value val = el.get_value();
            std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"";
            int charCode = 0;
            if (strVal == L"Space") {
                charCode = 32;
            } else if (strVal.length() == 1) {
                charCode = (int)strVal[0];
            }
            // Update high byte
            convertToolHotKey &= 0x00FFFFFF;
            convertToolHotKey |= ((unsigned int)charCode << 24);
            // Also update low byte for compatibility
            convertToolHotKey &= 0xFFFFFF00;
            convertToolHotKey |= (charCode & 0xFF);
            APP_SET_DATA(convertToolHotKey, convertToolHotKey);
            notifyMainProcess();
            return true;
        }
        
        // Encoding dropdowns
        if (id == L"source-encoding") {
            sciter::value val = el.get_value();
            if (val.is_int()) {
                convertToolFromCode = val.get<int>();
            } else if (val.is_string()) {
                convertToolFromCode = std::stoi(val.get<std::wstring>());
            } else {
                convertToolFromCode = 0;
            }
            APP_SET_DATA(convertToolFromCode, convertToolFromCode);
            notifyMainProcess();
            return true;
        }
        
        if (id == L"dest-encoding") {
            sciter::value val = el.get_value();
            if (val.is_int()) {
                convertToolToCode = val.get<int>();
            } else if (val.is_string()) {
                convertToolToCode = std::stoi(val.get<std::wstring>());
            } else {
                convertToolToCode = 0;
            }
            APP_SET_DATA(convertToolToCode, convertToolToCode);
            notifyMainProcess();
            return true;
        }
    }
    
    return false;
}
