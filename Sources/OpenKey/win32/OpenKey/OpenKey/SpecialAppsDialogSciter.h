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
#include "sciter-x.h"
#include "sciter-x-window.hpp"
#include <string>
#include <vector>
#include <set>
#include <algorithm>

// App types for special handling
enum class SpecialAppType {
    QtElectron = 0,    // Skip empty char fix (prevents lag)
    SkipImeCheck = 1   // Apps that falsely report IME as ON
};

struct SpecialAppEntry {
    std::string exeName;
    SpecialAppType type;
    bool isDefault;  // True = hardcoded default, cannot delete
};

class SpecialAppsDialogSciter : public sciter::window {
public:
    SpecialAppsDialogSciter();
    virtual ~SpecialAppsDialogSciter();
    
    virtual bool handle_event(HELEMENT he, BEHAVIOR_EVENT_PARAMS& params) override;
    void show();
    
    static LRESULT CALLBACK SubclassProc(HWND hwnd, UINT msg, WPARAM wParam, 
                                         LPARAM lParam, UINT_PTR uIdSubclass, 
                                         DWORD_PTR dwRefData);
private:
    void enableAcrylicEffect();
    void loadAppsList();
    void fillAppsListUI();
    void saveData();
    void onAddApp(const std::wstring& appName, SpecialAppType type);
    void onDeleteApp(const std::wstring& appName);
    void onChangeType(const std::wstring& appName, SpecialAppType newType);
    
    // Window Picker
    void startWindowPicking();
    void stopWindowPicking();
    std::string getExeNameFromWindow(HWND hwnd);
    void onAddPickedApp(const std::string& exeName);
    
    // Running Apps Dropdown
    void sendRunningAppsToJS();
    static BOOL CALLBACK EnumWindowsCallback(HWND hwnd, LPARAM lParam);
    
    // Helpers
    static std::string toLower(const std::string& s);
    bool isDuplicate(const std::string& exeName);
    
    std::vector<SpecialAppEntry> m_appsList;
    
    // Window Picker state
    bool m_isPickingWindow = false;
    HCURSOR m_hOldCursor = NULL;
    HCURSOR m_hSavedArrowCursor = NULL;
};
