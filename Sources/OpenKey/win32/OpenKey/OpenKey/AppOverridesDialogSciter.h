/*----------------------------------------------------------
NextKey - The Modern Vietnamese Input Method Engine.
Based on OpenKey architecture.

AppOverridesDialogSciter - Unified per-app configuration dialog
Replaces separate SpecialApps + ClipboardApps dialogs

Copyright (C) 2026 NextKey Project
Author: Mai Tan Phat
License: GPL (Inherited from OpenKey)
-----------------------------------------------------------*/
#pragma once
#include "sciter-x.h"
#include "sciter-x-window.hpp"
#include <string>
#include <vector>

class AppOverridesDialogSciter : public sciter::window {
public:
    AppOverridesDialogSciter();
    virtual ~AppOverridesDialogSciter() {}
    
    virtual bool handle_event(HELEMENT he, BEHAVIOR_EVENT_PARAMS& params) override;
    void show();
    
    static LRESULT CALLBACK SubclassProc(HWND hwnd, UINT msg, WPARAM wParam, 
                                         LPARAM lParam, UINT_PTR uIdSubclass, 
                                         DWORD_PTR dwRefData);
private:
    void enableAcrylicEffect();
    
    // Data management
    struct AppEntry {
        std::string exeName;
        int8_t behaviorType = 0;    // 0=None, 1=Skip IME, 2=Qt-Electron
        int8_t clipboardMethod = -1; // -1=None, 0=CtrlV, 1=ShiftInsert
    };
    std::vector<AppEntry> m_appsList;
    
    void loadAppsList();
    void saveData();
    void fillAppsListUI();
    
    // App management
    void onAddApp(const std::wstring& appName, int8_t behaviorType, int8_t clipboardMethod);
    void onDeleteApp(const std::wstring& appName);
    void onChangeBehavior(const std::wstring& appName, int8_t behaviorType);
    void onChangeClipboard(const std::wstring& appName, int8_t clipboardMethod);
    
    // Window picker
    bool m_isPickingWindow = false;
    HCURSOR m_hSavedArrowCursor = NULL;
    void startWindowPicking();
    void stopWindowPicking();
    std::string getExeNameFromWindow(HWND hwnd);
    void onAddPickedApp(const std::string& exeName);
    
    // Running apps dropdown
    void sendRunningAppsToJS();
    
    // Helpers
    std::string toLower(const std::string& s);
    bool isDuplicate(const std::string& exeName);
};
