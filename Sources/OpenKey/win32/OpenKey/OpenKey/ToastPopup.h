/*----------------------------------------------------------
NextKey - The Modern Vietnamese Input Method Engine.

Copyright (C) 2026 NextKey Project
Author: Mai Tan Phat
License: GPL (Inherited from OpenKey)
-----------------------------------------------------------*/
#pragma once

#include <Windows.h>
#include <string>

// Lightweight toast popup that appears instantly
// Replaces Shell_NotifyIcon balloon which has inherent delays
class ToastPopup {
public:
    // Initialize the popup system (call from main/UI thread)
    static void init(HINSTANCE hInstance);
    
    // Show toast message (thread-safe - can be called from any thread)
    // Message will appear near the foreground window
    static void show(LPCWSTR message);
    
    // Cleanup (call before exit)
    static void shutdown();

private:
    static ToastPopup& instance();
    
    ToastPopup() = default;
    ~ToastPopup();
    
    // Non-copyable
    ToastPopup(const ToastPopup&) = delete;
    ToastPopup& operator=(const ToastPopup&) = delete;
    
    // Internal methods
    void createWindow(HINSTANCE hInstance);
    void showInternal(LPCWSTR message);
    void hide();
    void updatePosition();
    void paint(HDC hdc);
    
    static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    // Window
    HWND m_hwnd = nullptr;
    HINSTANCE m_hInstance = nullptr;
    HICON m_hIcon = nullptr;
    bool m_initialized = false;
    
    // Current message
    std::wstring m_message;
    
    // Appearance
    static constexpr int TOAST_WIDTH = 220;
    static constexpr int TOAST_HEIGHT = 56;
    static constexpr int TOAST_MARGIN = 16;
    static constexpr int ICON_SIZE = 20;
    static constexpr int PADDING = 10;
    static constexpr UINT TIMER_DISMISS = 1;
    static constexpr UINT DISMISS_DELAY_MS = 800;
    
    // Custom message for thread-safe show
    static constexpr UINT WM_TOAST_SHOW = WM_USER + 100;
    
    // Colors (dark theme)
    static constexpr COLORREF BG_COLOR = RGB(45, 45, 48);
    static constexpr COLORREF TEXT_COLOR = RGB(240, 240, 240);
    static constexpr COLORREF TITLE_COLOR = RGB(100, 180, 255);  // Blue accent
    static constexpr BYTE ALPHA = 235;  // 0-255 transparency
};
