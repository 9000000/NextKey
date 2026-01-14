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

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

// Windows Header Files
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <string>
#include <vector>

#include <shellapi.h>
#include <Shlobj.h>
#include <Commctrl.h>
#include <psapi.h>

#include "resource.h"

#include "../../../engine/Engine.h"

#include "OpenKeyManager.h"
#include "OpenKeyHelper.h"
#include "SystemTrayHelper.h"

using namespace std;

// Global debug flag - set by PerformanceLogger::setEnabled()
// Used by LOG macro to conditionally output debug messages
extern bool _debugLogEnabled;

extern wchar_t _logBuffer[1024];
// LOG macro only outputs when Debug mode is enabled
// This silences development logs in production while keeping them for debugging
#define LOG(...)  if(_debugLogEnabled) { \
                      wsprintfW(_logBuffer, __VA_ARGS__); \
                      OutputDebugString(_logBuffer); \
                  }

// APP_SET_DATA: Set variable only. ConfigManager handles persistence via TOML.
// Note: After changing settings, caller should sync to ConfigManager and save.
#define APP_SET_DATA(KEY, VAL) KEY = VAL
// APP_GET_DATA: For legacy compatibility. Settings should be loaded from ConfigManager at startup.
#define APP_GET_DATA(KEY, DEFAULT_VAL) KEY = DEFAULT_VAL

#define APP_CLASS _T("NextKeyVietnameseInputMethod")

extern void saveSmartSwitchKeyData();

// Cross-process message for realtime V/E toggle sync
// Uses RegisterWindowMessage for unique system-wide message ID
inline UINT GetNextKeyUpdateMsg() {
	static UINT msg = RegisterWindowMessage(L"NextKeyLanguageStateChange");
	return msg;
}

// Notify all NextKey UI windows (Settings, etc.) of language change
// Uses HWND_BROADCAST so no need to track window handles
inline void NotifyUILanguageChange(bool isVietnamese) {
	PostMessage(HWND_BROADCAST, GetNextKeyUpdateMsg(), (WPARAM)isVietnamese, 0);
}

extern int vLanguage;
extern int vInputType;
extern int vFreeMark;
extern int vCodeTable;
extern int vCheckSpelling;
extern int vUseModernOrthography;
extern int vQuickTelex;
extern int vSwitchKeyStatus;
extern int vRestoreIfWrongSpelling;
extern int vFixRecommendBrowser;
extern int vUseMacro;
extern int vUseMacroInEnglishMode;
extern int vAutoCapsMacro;
extern int vSendKeyStepByStep;
extern int vUseSmartSwitchKey;
extern int vUpperCaseFirstChar;
extern int vUseGrayIcon;
extern int vShowOnStartUp;
extern int vRunWithWindows;
extern int vSupportMetroApp;
extern int vCreateDesktopShortcut;
extern int vRunAsAdmin;
extern int vCheckNewVersion;
extern int vRememberCode;
extern int vOtherLanguage;
extern int vTempOffOpenKey;
extern int vFixChromiumBrowser;
extern COLORREF vTrayIconColorV;
extern COLORREF vTrayIconColorE;
extern wchar_t vTrayIconFontName[LF_FACESIZE];

// Default tray icon colors
// COLORREF format: 0x00BBGGRR (BGR order, not RGB!)
// Use these constants instead of hardcoded values throughout the codebase
#define TRAY_DEFAULT_COLOR_V RGB(243, 98, 103)   // CSS: #F36267 - Pink/Red for Vietnamese
#define TRAY_DEFAULT_COLOR_E RGB(47, 175, 218)   // CSS: #2FAFDA - Blue for English