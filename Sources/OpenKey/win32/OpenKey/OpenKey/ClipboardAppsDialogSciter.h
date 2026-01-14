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
#include <algorithm>

// Injection methods for per-app configuration
enum class ClipboardMethod {
    ShiftInsert = 0,     // Clipboard + Shift+Insert (default)
    CtrlV = 1,           // Clipboard + Ctrl+V
    SendInputKey = 2     // SendInput key-by-key with Smart Batch delay (12ms)
};

struct ClipboardAppEntry {
    std::string exeName;
    ClipboardMethod method;
    int delayMs = 0;  // Delay after paste (0-500ms)
};

class ClipboardAppsDialogSciter : public sciter::window {
public:
    ClipboardAppsDialogSciter();
    virtual ~ClipboardAppsDialogSciter();
    
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
    void onAddApp(const std::wstring& appName, ClipboardMethod method, int delayMs);
    void onDeleteApp(const std::wstring& appName);
    void onChangeMethod(const std::wstring& appName, ClipboardMethod newMethod);
    void onChangeDelay(const std::wstring& appName, int delayMs);
    
    // Window Picker
    void startWindowPicking();
    void stopWindowPicking();
    std::string getExeNameFromWindow(HWND hwnd);
    void onAddPickedApp(const std::string& exeName);
    
    // Running Apps Dropdown
    void sendRunningAppsToJS();
    
    // Helpers
    static std::string toLower(const std::string& s);
    bool isDuplicate(const std::string& exeName);
    
    std::vector<ClipboardAppEntry> m_appsList;
    
    // Window Picker state
    bool m_isPickingWindow = false;
    HCURSOR m_hOldCursor = NULL;
    HCURSOR m_hSavedArrowCursor = NULL;
};
