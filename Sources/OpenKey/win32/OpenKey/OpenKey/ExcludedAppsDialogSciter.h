/*----------------------------------------------------------
NextKey - The Modern Vietnamese Input Method Engine.
Based on OpenKey architecture.

Copyright (C) 2026 NextKey Project
Author: Mai Tan Phat
License: GPL (Inherited from OpenKey)
-----------------------------------------------------------*/
#pragma once

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
#include <vector>
#include <set>

class ExcludedAppsDialogSciter : public sciter::window {
public:
    ExcludedAppsDialogSciter();
    virtual ~ExcludedAppsDialogSciter();
    
    virtual bool handle_event(HELEMENT he, BEHAVIOR_EVENT_PARAMS& params) override;
    void show();
    
    static LRESULT CALLBACK SubclassProc(HWND hwnd, UINT msg, WPARAM wParam, 
                                         LPARAM lParam, UINT_PTR uIdSubclass, 
                                         DWORD_PTR dwRefData);
private:
    // Removed enableAcrylicEffect() - using SciterHelper::enableWindowBlur instead
    void fillAppsList();
    void saveAndReload();
    void onAddManual(const std::wstring& appName);
    void onAddCurrentApp();
    void onDeleteApp(const std::wstring& appName);
    
    // Window Picker
    void startWindowPicking();
    void stopWindowPicking();
    std::string getExeNameFromWindow(HWND hwnd);
    void onAddPickedApp(const std::string& exeName);
    
    // Running Apps Dropdown
    void sendRunningAppsToJS();
    static BOOL CALLBACK EnumWindowsCallback(HWND hwnd, LPARAM lParam);
    
    std::vector<std::string> m_appsList;
    
    // Window Picker state
    bool m_isPickingWindow = false;
    HCURSOR m_hOldCursor = NULL;
    HCURSOR m_hSavedArrowCursor = NULL;  // Saved arrow cursor before replacing
};
