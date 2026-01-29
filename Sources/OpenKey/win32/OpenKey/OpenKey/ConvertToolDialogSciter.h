/*----------------------------------------------------------
NextKey - The Modern Vietnamese Input Method Engine.
Based on OpenKey architecture.

Copyright (C) 2026 NextKey Project
Author: Mai Tan Phat
License: GPL (Inherited from OpenKey)
-----------------------------------------------------------*/
#pragma once
#include "stdafx.h"

// Undefine Windows/Engine macros that conflict with Sciter enums
#ifdef KEY_DOWN
#undef KEY_DOWN
#endif
#ifdef KEY_UP
#undef KEY_UP
#endif

#include "sciter-x.h"
#include "sciter-x-window.hpp"
#include "SciterHelper.h"
#include <string>

class ConvertToolDialogSciter : public sciter::window {
public:
    ConvertToolDialogSciter();
    virtual ~ConvertToolDialogSciter() {}
    
    virtual bool handle_event(HELEMENT he, BEHAVIOR_EVENT_PARAMS& params) override;
    void show();
    
    static LRESULT CALLBACK SubclassProc(HWND hwnd, UINT msg, WPARAM wParam,
                                         LPARAM lParam, UINT_PTR uIdSubclass,
                                         DWORD_PTR dwRefData);
private:
    // Removed enableAcrylicEffect() - using SciterHelper::enableWindowBlur instead
    void loadSettings();
    void onConvert();
    void recalcWindowSize();
    void onSelectFile(bool isSource);
};
