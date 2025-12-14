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
#pragma once
#include <windows.h>
#include <algorithm>
#include <cmath>

/**
 * ScaleHelper - Utility for scaling dialog sizes based on DPI and resolution.
 * 
 * Considers both:
 * 1. DPI scaling (125%, 150%, etc.) - scales down inversely
 * 2. Screen resolution (1366x768, etc.) - scales down proportionally
 * 
 * Uses the smaller factor to ensure dialog fits well in all scenarios.
 */
class ScaleHelper {
public:
    // Reference resolution (your development monitor at 100% DPI)
    static constexpr int REF_SCREEN_WIDTH = 1920;
    static constexpr int REF_SCREEN_HEIGHT = 1080;
    
    /**
     * Get DPI scale factor (1.0 = 100%, 1.25 = 125%, etc.)
     */
    static double getDpiScale() {
        HDC hdc = GetDC(NULL);
        int dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
        ReleaseDC(NULL, hdc);
        return (double)dpiX / 96.0;  // Windows default DPI is 96
    }
    
    /**
     * Get the scale factor to apply to dialogs.
     * 
     * - Low resolution (1366x768): scale DOWN so dialog fits on screen
     * - High DPI (125%, 150%): scale UP because content is rendered larger
     * 
     * At 1920x1080 @ 100%: scale = 1.0
     * At 1920x1080 @ 125%: scale = 1.1 (window slightly larger)
     * At 1920x1080 @ 150%: scale = 1.2 (window larger)
     * At 1366x768 @ 100%:  scale = 0.71
     */
    static double getScaleFactor() {
        // Factor 1: Resolution scaling (for small screens)
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        double resScaleX = (double)screenWidth / REF_SCREEN_WIDTH;
        double resScaleY = (double)screenHeight / REF_SCREEN_HEIGHT;
        double resScale = (std::min)(resScaleX, resScaleY);
        
        // Factor 2: DPI scaling (scale UP for high DPI)
        // At 125% DPI, content is 25% larger, so window needs to be ~10% larger
        // At 150% DPI, content is 50% larger, so window needs to be ~20% larger
        double dpiScale = getDpiScale();
        double dpiBoost = 1.0 + (dpiScale - 1.0) * 0.4;  // Partial boost
        
        // Apply both factors
        double scale = resScale * dpiBoost;
        
        // Clamp: min 0.7, max 1.3
        scale = (std::max)(0.7, (std::min)(1.3, scale));
        
        return scale;
    }
    
    /**
     * Scale a dimension (width or height).
     */
    static int scale(int baseDimension) {
        return (int)(baseDimension * getScaleFactor());
    }
    
    /**
     * Scale a RECT for Sciter window initialization.
     */
    static RECT scaleRect(int baseWidth, int baseHeight) {
        double factor = getScaleFactor();
        return RECT{ 
            0, 0, 
            (LONG)(baseWidth * factor), 
            (LONG)(baseHeight * factor) 
        };
    }
    
    /**
     * Get scaled width and height as separate values.
     */
    static void getScaledSize(int baseWidth, int baseHeight, int& outWidth, int& outHeight) {
        double factor = getScaleFactor();
        outWidth = (int)(baseWidth * factor);
        outHeight = (int)(baseHeight * factor);
    }
};

