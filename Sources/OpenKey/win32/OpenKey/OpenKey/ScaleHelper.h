/*----------------------------------------------------------
NextKey - The Modern Vietnamese Input Method Engine.
Based on OpenKey architecture.

Copyright (C) 2026 NextKey Project
Author: Mai Tan Phat
License: GPL (Inherited from OpenKey)
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
     * - High DPI (125%, 150%): scale UP to match Sciter's native DPI rendering
     * 
     * At 1920x1080 @ 100%: scale = 1.0
     * At 1920x1080 @ 125%: scale = 1.25
     * At 1920x1080 @ 150%: scale = 1.5
     * At 1366x768 @ 100%:  scale = 0.71
     */
    static double getScaleFactor() {
        // Factor 1: Resolution scaling (for small screens)
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        double resScaleX = (double)screenWidth / REF_SCREEN_WIDTH;
        double resScaleY = (double)screenHeight / REF_SCREEN_HEIGHT;
        double resScale = (std::min)(resScaleX, resScaleY);
        
        // Factor 2: DPI scaling - Sciter renders at native DPI, so window must match
        // At 125% DPI: content is 125% larger, window must be 125% of base
        // At 150% DPI: content is 150% larger, window must be 150% of base
        double dpiScale = getDpiScale();
        
        // Apply both factors
        double scale = resScale * dpiScale;
        
        // Clamp: min 0.7, max 1.6 (supports up to 160% DPI)
        scale = (std::max)(0.7, (std::min)(1.6, scale));
        
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

