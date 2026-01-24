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

// Tray icon ID for Shell_NotifyIcon
#define TRAY_ICON_ID 100

class SystemTrayHelper {
private:
	static void CALLBACK WaitToCreateFakeWindow(HWND hwnd, UINT uMsg, UINT timerId, DWORD dwTime);
	static HWND createFakeWindow(const HINSTANCE& hIns);
	static void createPopupMenu();

	static void _createSystemTrayIcon(const HINSTANCE& hIns);
public:
	static void createSystemTrayIcon(const HINSTANCE& hIns);
	static void updateData();
	static void removeSystemTray();
	
	// Get HWND of tray helper window (for balloon notifications)
	static HWND getHwnd();
};

