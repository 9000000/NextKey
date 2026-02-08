/*----------------------------------------------------------
NextKey - The Modern Vietnamese Input Method Engine.

Copyright (C) 2026 NextKey Project
Author: Mai Tan Phat
License: GPL (Inherited from OpenKey)
-----------------------------------------------------------*/
#include "stdafx.h"
#include "ToastPopup.h"
#include "resource.h"
#include "GdiPlusManager.h"

static const wchar_t* TOAST_CLASS_NAME = L"NextKeyToastPopup";

ToastPopup& ToastPopup::instance() {
    static ToastPopup inst;
    return inst;
}

ToastPopup::~ToastPopup() {
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

void ToastPopup::init(HINSTANCE hInstance) {
    instance().createWindow(hInstance);
}

void ToastPopup::shutdown() {
    auto& inst = instance();
    if (inst.m_hwnd) {
        DestroyWindow(inst.m_hwnd);
        inst.m_hwnd = nullptr;
    }
}

void ToastPopup::show(LPCWSTR message) {
    auto& inst = instance();
    if (!inst.m_hwnd) return;
    
    // Thread-safe: PostMessage to UI thread
    // We need to copy the message since it might be on stack
    size_t len = wcslen(message);
    wchar_t* msgCopy = new wchar_t[len + 1];
    wcscpy_s(msgCopy, len + 1, message);
    
    PostMessage(inst.m_hwnd, WM_TOAST_SHOW, 0, (LPARAM)msgCopy);
}

void ToastPopup::createWindow(HINSTANCE hInstance) {
    if (m_initialized) return;
    m_hInstance = hInstance;
    
    // Register window class
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = wndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;  // We handle painting
    wc.lpszClassName = TOAST_CLASS_NAME;
    RegisterClassExW(&wc);
    
    // Create layered popup window
    // WS_EX_TOOLWINDOW: doesn't appear in taskbar
    // WS_EX_TOPMOST: always on top
    // WS_EX_LAYERED: supports alpha transparency
    // WS_EX_NOACTIVATE: doesn't steal focus
    m_hwnd = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_NOACTIVATE,
        TOAST_CLASS_NAME,
        L"",
        WS_POPUP,
        0, 0, TOAST_WIDTH, TOAST_HEIGHT,
        nullptr, nullptr, hInstance, this
    );
    
    if (m_hwnd) {
        // Set alpha transparency
        SetLayeredWindowAttributes(m_hwnd, 0, ALPHA, LWA_ALPHA);
        
        // Load app icon for toast
        m_hIcon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_ICON1), 
                                   IMAGE_ICON, ICON_SIZE, ICON_SIZE, LR_DEFAULTCOLOR);
        m_initialized = true;
    }
}

void ToastPopup::showInternal(LPCWSTR message) {
    if (!m_hwnd) return;
    
    m_message = message;
    
    // Calculate position: bottom-right corner of foreground window
    updatePosition();
    
    // Show without activating
    ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
    InvalidateRect(m_hwnd, nullptr, TRUE);
    
    // Set auto-dismiss timer
    SetTimer(m_hwnd, TIMER_DISMISS, DISMISS_DELAY_MS, nullptr);
}

void ToastPopup::hide() {
    if (m_hwnd) {
        KillTimer(m_hwnd, TIMER_DISMISS);
        ShowWindow(m_hwnd, SW_HIDE);
    }
}

void ToastPopup::updatePosition() {
    if (!m_hwnd) return;
    
    HWND foreground = GetForegroundWindow();
    RECT workArea;
    
    // Get work area of the monitor where foreground window is
    HMONITOR monitor = MonitorFromWindow(foreground, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfo(monitor, &mi);
    workArea = mi.rcWork;
    
    // DPI scaling
    UINT dpi = 96;
    // Try to get DPI for the window (Win10 1607+)
    HMODULE user32 = GetModuleHandle(L"user32.dll");
    if (user32) {
        typedef UINT (WINAPI *GetDpiForWindowFunc)(HWND);
        auto pGetDpiForWindow = (GetDpiForWindowFunc)GetProcAddress(user32, "GetDpiForWindow");
        if (pGetDpiForWindow && foreground) {
            dpi = pGetDpiForWindow(foreground);
        }
    }
    
    int scale = MulDiv(100, dpi, 96);
    int width = MulDiv(TOAST_WIDTH, scale, 100);
    int height = MulDiv(TOAST_HEIGHT, scale, 100);
    int margin = MulDiv(TOAST_MARGIN, scale, 100);
    
    // Position: bottom-right of work area with margin
    int x = workArea.right - width - margin;
    int y = workArea.bottom - height - margin;
    
    SetWindowPos(m_hwnd, HWND_TOPMOST, x, y, width, height, SWP_NOACTIVATE);
}

void ToastPopup::paint(HDC hdc) {
    RECT rc;
    GetClientRect(m_hwnd, &rc);
    
    // Get DPI for scaling
    UINT dpi = 96;
    HMODULE user32 = GetModuleHandle(L"user32.dll");
    if (user32) {
        typedef UINT (WINAPI *GetDpiForWindowFunc)(HWND);
        auto pGetDpiForWindow = (GetDpiForWindowFunc)GetProcAddress(user32, "GetDpiForWindow");
        if (pGetDpiForWindow && m_hwnd) {
            dpi = pGetDpiForWindow(m_hwnd);
        }
    }
    
    int padding = MulDiv(PADDING, dpi, 96);
    int iconSize = MulDiv(ICON_SIZE, dpi, 96);
    
    // Simple solid background (transparency handled by SetLayeredWindowAttributes)
    HBRUSH bgBrush = CreateSolidBrush(BG_COLOR);
    FillRect(hdc, &rc, bgBrush);
    DeleteObject(bgBrush);
    
    // Subtle border
    HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(70, 70, 75));
    HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(borderPen);
    
    // Draw blue 'i' icon with background box
    int iconX = padding;
    int iconY = (rc.bottom - iconSize) / 2;
    
    // Blue background box (rounded)
    HBRUSH iconBgBrush = CreateSolidBrush(RGB(0, 120, 215));  // Windows blue
    HPEN iconBgPen = CreatePen(PS_SOLID, 1, RGB(0, 120, 215));
    HPEN oldIconPen = (HPEN)SelectObject(hdc, iconBgPen);
    HBRUSH oldIconBrush = (HBRUSH)SelectObject(hdc, iconBgBrush);
    RoundRect(hdc, iconX, iconY, iconX + iconSize, iconY + iconSize, 6, 6);
    SelectObject(hdc, oldIconPen);
    SelectObject(hdc, oldIconBrush);
    DeleteObject(iconBgBrush);
    DeleteObject(iconBgPen);
    
    // White 'i' text inside
    int iFontSize = MulDiv(14, dpi, 96);
    HFONT iFont = CreateFontW(
        -iFontSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
    );
    HFONT oldIFont = (HFONT)SelectObject(hdc, iFont);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));  // White
    RECT iRect = { iconX, iconY, iconX + iconSize, iconY + iconSize };
    DrawTextW(hdc, L"i", 1, &iRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, oldIFont);
    DeleteObject(iFont);
    
    // Text area starts after icon
    int textLeft = iconX + iconSize + padding / 2;
    
    SetBkMode(hdc, TRANSPARENT);
    
    // Title font (bold, smaller)
    int titleFontSize = MulDiv(11, dpi, 96);
    HFONT titleFont = CreateFontW(
        -titleFontSize, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
    );
    
    // Message font (normal)
    int msgFontSize = MulDiv(13, dpi, 96);
    HFONT msgFont = CreateFontW(
        -msgFontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
    );
    
    // Draw title "NextKey" in red (matching V icon)
    HFONT oldFont = (HFONT)SelectObject(hdc, titleFont);
    SetTextColor(hdc, RGB(220, 60, 60));  // Red like V icon
    RECT titleRc = { textLeft, padding - 2, rc.right - padding, rc.bottom / 2 };
    DrawTextW(hdc, L"NextKey", -1, &titleRc, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);
    
    // Draw message
    SelectObject(hdc, msgFont);
    SetTextColor(hdc, TEXT_COLOR);
    RECT msgRc = { textLeft, rc.bottom / 2 - 2, rc.right - padding, rc.bottom - padding + 4 };
    DrawTextW(hdc, m_message.c_str(), -1, &msgRc, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);
    
    SelectObject(hdc, oldFont);
    DeleteObject(titleFont);
    DeleteObject(msgFont);
}

LRESULT CALLBACK ToastPopup::wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    ToastPopup* self = nullptr;
    
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        self = reinterpret_cast<ToastPopup*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    } else {
        self = reinterpret_cast<ToastPopup*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    
    switch (msg) {
        case WM_TOAST_SHOW: {
            // Message received from PostMessage - show the toast
            wchar_t* msgCopy = reinterpret_cast<wchar_t*>(lParam);
            if (msgCopy && self) {
                self->showInternal(msgCopy);
                delete[] msgCopy;  // Free the copy made in show()
            }
            return 0;
        }
        
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            if (self) {
                self->paint(hdc);
            }
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_TIMER: {
            if (wParam == TIMER_DISMISS && self) {
                self->hide();
            }
            return 0;
        }
        
        case WM_DESTROY: {
            return 0;
        }
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
