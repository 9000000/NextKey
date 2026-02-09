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

Portions Copyright (C) 2026 NextKey Project
Maintainer: Mai Tan Phat
-----------------------------------------------------------*/
#pragma once
#include "stdafx.h"

extern int CF_RTF;
extern int CF_HTML;
extern int CF_OPENKEY;

class OpenKeyHelper {
public:
	// All settings now use ConfigManager with TOML
	// Only registry used is for Windows Run key (startup) 
	
	static void registerRunOnStartup(const int& val);
	static void resetAllSettings();  // Delete config.toml to reset to defaults

	static LPTSTR getExecutePath();

	static std::string& getFrontMostAppExecuteName();
	static std::string& getLastAppExecuteName();

	static std::wstring getFullPath();

	static std::wstring getClipboardText(const int& type);
	static bool setClipboardText(LPCTSTR data, const int& len, const int& type);

	// Legacy quickConvert - use QuickConvert::convert() for new code
	static bool quickConvert();

	static DWORD getVersionNumber();
	static std::wstring getVersionString();

	static std::wstring getContentOfUrl(LPCTSTR url);
	
	// Check if Windows is using dark mode (Windows 10 1809+)
	// Returns false (light mode) as fallback for older Windows
	static bool isWindowsDarkMode();
	
	// Check if running on Windows 11 or later (build 22000+)
	// Used to determine if modern DWM features are available
	static bool isWindows11OrGreater();
	
	// Handle WM_USER+107 IPC message to bring window to foreground
	// Call this from SubclassProc when receiving WM_USER+107
	// Returns LRESULT to return from SubclassProc (always 0)
	static LRESULT handleIPCForeground(HWND hwnd);
};


