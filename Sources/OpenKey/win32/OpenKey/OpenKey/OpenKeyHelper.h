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
private:
	static void openKey();
public:
	static void setRegInt(LPCTSTR key, const int& val);
	static int getRegInt(LPCTSTR key, const int& defaultValue);

	static void setRegBinary(LPCTSTR key, const BYTE* pData, const int& size);
	static BYTE* getRegBinary(LPCTSTR key, DWORD& outSize);

	static void setRegString(LPCTSTR key, LPCTSTR val);
	static bool getRegString(LPCTSTR key, LPTSTR outBuffer, DWORD bufferSize);
	static std::wstring getRegString(LPCTSTR key, LPCTSTR defaultValue);  // Convenient overload

	static void registerRunOnStartup(const int& val);
	static void resetAllSettings();  // Delete entire registry key to reset to defaults
	static void migrateFromOldRegistry();  // Migrate settings from TuyenMai\OpenKey to NextKey (one-time)

	static LPTSTR getExecutePath();

	static string& getFrontMostAppExecuteName();
	static string& getLastAppExecuteName();

	static wstring getFullPath();

	static wstring getClipboardText(const int& type);
	static void setClipboardText(LPCTSTR data, const int& len, const int& type);

	static bool quickConvert();

	static DWORD getVersionNumber();
	static wstring getVersionString();

	static wstring getContentOfUrl(LPCTSTR url);
	
	// Check if Windows is using dark mode (Windows 10 1809+)
	// Returns false (light mode) as fallback for older Windows
	static bool isWindowsDarkMode();
};
