/*----------------------------------------------------------
NextKey - The Modern Vietnamese Input Method Engine.
Based on OpenKey architecture.

Copyright (C) 2026 NextKey Project
Author: Mai Tan Phat
License: GPL (Inherited from OpenKey)
-----------------------------------------------------------*/
#pragma once
#include <windows.h>
#include <string>

// Unique enum names to avoid macro collisions
enum class SciterBlurMode {
    BM_SOLID = 0,
    BM_BLUR = 1
};

// DWM structures for window effects
struct ACCENT_POLICY_SHARED {
    int AccentState;
    int AccentFlags;
    int GradientColor;
    int AnimationId;
};

struct WINDOWCOMPOSITIONATTRIBDATA_SHARED {
    int Attrib;
    void* pvData;
    size_t cbData;
};

enum ACCENT_STATE_SHARED {
    ACCENT_DISABLED = 0,
    ACCENT_ENABLE_BLURBEHIND = 3,
    ACCENT_ENABLE_ACRYLICBLURBEHIND = 4,
    ACCENT_ENABLE_HOSTBACKDROP = 5
};

namespace SciterHelper {
    // Enable DWM Blur/Acrylic/Mica effect on the window
    void enableWindowBlur(HWND hwnd, SciterBlurMode mode);

    // Standard handler for WM_NCHITTEST to allow dragging via title area
    // Returns HTCAPTION if in drag zone, or HTCLIENT/other otherwise.
    LRESULT handleWindowDrag(HWND hwnd, LPARAM lParam, int titleHeight = 40);
}
