/*----------------------------------------------------------
NextKey - The Cross platform Open source Vietnamese Keyboard application.

Copyright (C) 2019 Mai Vu Tuyen
This file is belong to the NextKey project, Win32 version
which is released under GPL license.
-----------------------------------------------------------*/
#include "SciterDllLoader.h"
#include "resource.h"
#include <shlobj.h>
#include <shlwapi.h>
#include <stdio.h>

#pragma comment(lib, "Shlwapi.lib")

static WCHAR g_sciterDllPath[MAX_PATH] = {0};

bool EnsureSciterDll(LPCWSTR& outPath) {
#ifdef NDEBUG
    // 1. First try: Look for sciter.dll next to EXE (portable mode)
    WCHAR exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    PathRemoveFileSpecW(exePath);
    
    swprintf_s(g_sciterDllPath, L"%s\\sciter.dll", exePath);
    
    // Check if DLL exists in exe folder
    if (GetFileAttributesW(g_sciterDllPath) != INVALID_FILE_ATTRIBUTES) {
        outPath = g_sciterDllPath;
        return true;
    }
    
    // 2. Fallback: Look in %LocalAppData%\NextKey\sciter.dll
    WCHAR appData[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, appData))) {
        swprintf_s(g_sciterDllPath, L"%s\\NextKey\\sciter.dll", appData);
        
        if (GetFileAttributesW(g_sciterDllPath) != INVALID_FILE_ATTRIBUTES) {
            outPath = g_sciterDllPath;
            return true;
        }
    }
    
    // 3. DLL not found - show error
    MessageBoxW(NULL, 
        L"sciter.dll not found!\n\n"
        L"Please ensure sciter.dll is in the same folder as NextKey64.exe\n"
        L"or in %LocalAppData%\\NextKey\\",
        L"NextKey Error", MB_OK | MB_ICONERROR);
    return false;
#else
    // Debug: use DLL from output folder
    outPath = L"sciter.dll";
    return true;
#endif
}
