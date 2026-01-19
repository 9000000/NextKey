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

	static string& getFrontMostAppExecuteName();
	static string& getLastAppExecuteName();

	static wstring getFullPath();

	static wstring getClipboardText(const int& type);
	static bool setClipboardText(LPCTSTR data, const int& len, const int& type);

	static bool quickConvert();

	static DWORD getVersionNumber();
	static wstring getVersionString();

	static wstring getContentOfUrl(LPCTSTR url);
	
	// Check if Windows is using dark mode (Windows 10 1809+)
	// Returns false (light mode) as fallback for older Windows
	static bool isWindowsDarkMode();
	
	// Handle WM_USER+107 IPC message to bring window to foreground
	// Call this from SubclassProc when receiving WM_USER+107
	// Returns LRESULT to return from SubclassProc (always 0)
	static LRESULT handleIPCForeground(HWND hwnd);
};
