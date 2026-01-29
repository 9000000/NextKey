/*----------------------------------------------------------
NextKey - The Modern Vietnamese Input Method Engine.
Based on OpenKey architecture.

Copyright (C) 2026 NextKey Project
Author: Mai Tan Phat
License: GPL (Inherited from OpenKey)
-----------------------------------------------------------*/
#include "stdafx.h"

// Undefine Windows/Engine macros that conflict with Sciter enums
#ifdef KEY_DOWN
#undef KEY_DOWN
#endif
#ifdef KEY_UP
#undef KEY_UP
#endif

#include "SciterHelper.h"
#include "sciter-x.h"
#include <dwmapi.h>
#include <windowsx.h>

#pragma comment(lib, "dwmapi.lib")

namespace SciterHelper {

void enableWindowBlur(HWND hwnd, SciterBlurMode mode) {
    if (!hwnd) return;

    bool isBlur = (mode == SciterBlurMode::BM_BLUR);

    // 1. Set WS_EX_LAYERED to allow alpha/blur composition
    SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);

    // 2. Apply SetWindowCompositionAttribute (Acrylic logic)
    HMODULE hUser = GetModuleHandle(L"user32.dll");
    if (hUser) {
        typedef BOOL(WINAPI* pSetWindowCompositionAttribute)(HWND, WINDOWCOMPOSITIONATTRIBDATA_SHARED*);
        auto SetWindowCompositionAttribute = (pSetWindowCompositionAttribute)GetProcAddress(hUser, "SetWindowCompositionAttribute");

        if (SetWindowCompositionAttribute) {
            ACCENT_POLICY_SHARED policy = { 0 };
            // Using ACCENT_ENABLE_BLURBEHIND (3) as it's most stable across Win10/11 version
            policy.AccentState = isBlur ? ACCENT_ENABLE_BLURBEHIND : ACCENT_DISABLED;
            policy.AccentFlags = 0;
            policy.GradientColor = 0;
            policy.AnimationId = 0;

            WINDOWCOMPOSITIONATTRIBDATA_SHARED data = { 0 };
            data.Attrib = 19; // WCA_ACCENT_POLICY
            data.pvData = &policy;
            data.cbData = sizeof(policy);

            SetWindowCompositionAttribute(hwnd, &data);
        }
    }

    // 3. Fix rounded corners on Windows 11
    // DWMWA_WINDOW_CORNER_PREFERENCE = 33
    int preference = 2; // DWMWCP_ROUND
    DwmSetWindowAttribute(hwnd, 33, &preference, sizeof(preference));
}

LRESULT handleWindowDrag(HWND hwnd, LPARAM lParam, int titleHeight) {
    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
    ScreenToClient(hwnd, &pt);

    RECT clientRect;
    GetClientRect(hwnd, &clientRect);

    // Exclude buttons area (right-most 80px for close + pin buttons)
    int buttonsZone = clientRect.right - 80;

    if (pt.y < titleHeight && pt.x < buttonsZone) {
        return HTCAPTION;
    }

    return HTCLIENT;
}

} // namespace SciterHelper
