/*----------------------------------------------------------
NextKey - The Modern Vietnamese Input Method Engine.
Based on OpenKey architecture.

Copyright (C) 2026 NextKey Project
Author: Mai Tan Phat
License: GPL (Inherited from OpenKey)
-----------------------------------------------------------*/
#pragma once
#include "stdafx.h"
#include "sciter-x.h"
#include "sciter-x-window.hpp"
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
    void enableAcrylicEffect();
    void loadSettings();
    void onConvert();
    void recalcWindowSize();
    void onSelectFile(bool isSource);
};
