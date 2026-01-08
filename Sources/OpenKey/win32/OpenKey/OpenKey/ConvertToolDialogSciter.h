/*----------------------------------------------------------
OpenKey - The Cross platform Open source Vietnamese Keyboard application.

Copyright (C) 2019 Mai Vu Tuyen
Contact: maivutuyen.91@gmail.com
Github: https://github.com/tuyenvm/OpenKey

This file is belong to the OpenKey project, Win32 version
which is released under GPL license.
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
