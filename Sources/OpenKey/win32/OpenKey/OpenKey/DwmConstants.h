/*----------------------------------------------------------
NextKey - The Modern Vietnamese Input Method Engine.
Based on OpenKey architecture.

Copyright (C) 2026 NextKey Project
Author: Mai Tan Phat
License: GPL (Inherited from OpenKey)

DwmConstants.h - Desktop Window Manager constants and attributes
Fixes: CR-009 - Eliminates magic numbers for DWM attributes
Reference: https://learn.microsoft.com/en-us/windows/win32/api/dwmapi/
-----------------------------------------------------------*/

#pragma once

// ============================================================
// DWM Window Attributes (DWMWINDOWATTRIBUTE enum extension)
// Windows 10/11 undocumented attributes
// ============================================================

// Dark mode support (Windows 10 1809+)
// Use: BOOL dark = TRUE; DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
constexpr DWORD DWMWA_USE_IMMERSIVE_DARK_MODE = 20;

// Window corner preference (Windows 11)
// Use: DWM_WINDOW_CORNER_PREFERENCE corner = DWMWCP_ROUND; 
//      DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner, sizeof(corner));
constexpr DWORD DWMWA_WINDOW_CORNER_PREFERENCE = 33;

// System backdrop type (Windows 11 22H2+)
// Use: DWM_SYSTEMBACKDROP_TYPE backdrop = DWMSBT_TRANSIENTWINDOW; 
//      DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdrop, sizeof(backdrop));
constexpr DWORD DWMWA_SYSTEMBACKDROP_TYPE = 38;

// ============================================================
// DWM System Backdrop Types (DWM_SYSTEMBACKDROP_TYPE enum)
// For DWMWA_SYSTEMBACKDROP_TYPE attribute
// ============================================================

constexpr DWORD DWMSBT_AUTO = 0;              // Let system decide
constexpr DWORD DWMSBT_NONE = 1;              // No backdrop (opaque)
constexpr DWORD DWMSBT_MAINWINDOW = 2;        // Mica background
constexpr DWORD DWMSBT_TRANSIENTWINDOW = 3;   // Acrylic blur
constexpr DWORD DWMSBT_TABBEDWINDOW = 4;      // Mica Alt (tabbed)

// ============================================================
// DWM Window Corner Preferences (DWM_WINDOW_CORNER_PREFERENCE enum)
// For DWMWA_WINDOW_CORNER_PREFERENCE attribute
// ============================================================

constexpr DWORD DWMWCP_DEFAULT = 0;           // System default (usually rounded)
constexpr DWORD DWMWCP_DONOTROUND = 1;        // Square corners
constexpr DWORD DWMWCP_ROUND = 2;             // Large rounded corners (8px radius)
constexpr DWORD DWMWCP_ROUNDSMALL = 3;        // Small rounded corners (4px radius)

// ============================================================
// SetWindowCompositionAttribute (Windows 10 Acrylic/Blur-behind)
// For use with SetWindowCompositionAttribute() undocumented API
// ============================================================

// Window Composition Attributes
constexpr int WCA_ACCENT_POLICY = 19;

// Accent States for ACCENT_POLICY.AccentState
enum ACCENT_STATE {
    ACCENT_DISABLED = 0,
    ACCENT_ENABLE_GRADIENT = 1,              // Solid color gradient
    ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,   // Transparent gradient
    ACCENT_ENABLE_BLURBEHIND = 3,            // Blur behind (Aero-style)
    ACCENT_ENABLE_ACRYLICBLURBEHIND = 4,     // Acrylic blur (Windows 10 1803+)
    ACCENT_ENABLE_HOSTBACKDROP = 5           // Host backdrop (Mica, Windows 11)
};

// Structures for SetWindowCompositionAttribute
struct ACCENT_POLICY {
    int AccentState;
    int AccentFlags;
    int GradientColor;  // ABGR format: 0xAABBGGRR
    int AnimationId;
};

struct WINDOWCOMPOSITIONATTRIBDATA {
    int Attrib;
    void* pvData;
    size_t cbData;
};

// ============================================================
// Helper macros
// ============================================================

// Convert COLORREF (0x00BBGGRR) to ABGR with alpha
// Usage: ABGR_FROM_COLORREF(0xFF, RGB(255, 0, 0)) -> 0xFF0000FF (opaque red)
#define ABGR_FROM_COLORREF(alpha, colorRef) \
    (((alpha) << 24) | ((colorRef) & 0x00FFFFFF))

// Create ABGR color from components
// Usage: ABGR_COLOR(0xCC, 32, 32, 32) -> 0xCC202020 (80% opaque dark gray)
#define ABGR_COLOR(a, r, g, b) \
    (((a) << 24) | ((b) << 16) | ((g) << 8) | (r))
