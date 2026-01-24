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
#include "stdafx.h"
#include "AppDelegate.h"
#include "AboutDialog.h"
#include "SettingsDialog.h"
#include "MacroDialogSciter.h"
#include "ExcludedAppsDialogSciter.h"
#include "ConvertToolDialogSciter.h"
#include "AppOverridesDialogSciter.h"
#include "SciterDllLoader.h"
#include <Shlobj.h>

// Force window to foreground (bypasses Windows focus stealing prevention)
static void ForceForegroundWindow(HWND hWnd) {
	// First restore if minimized
	if (IsIconic(hWnd)) {
		ShowWindow(hWnd, SW_RESTORE);
	}
	
	// Make sure window is visible
	ShowWindow(hWnd, SW_SHOW);
	
	// AttachThreadInput pattern for reliable focus
	DWORD dwCurrentThread = GetCurrentThreadId();
	DWORD dwForegroundThread = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
	
	if (dwCurrentThread != dwForegroundThread) {
		AttachThreadInput(dwCurrentThread, dwForegroundThread, TRUE);
	}
	
	// Simulate Alt key press to trick Windows
	keybd_event(VK_MENU, 0, 0, 0);  // Alt down
	SetForegroundWindow(hWnd);
	keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);  // Alt up
	
	if (dwCurrentThread != dwForegroundThread) {
		AttachThreadInput(dwCurrentThread, dwForegroundThread, FALSE);
	}
	
	// Additional methods for reliability
	BringWindowToTop(hWnd);
	SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
}

// Forward declaration - defined in SciterArchive.cpp
void BindSciterResources();

// Initialize Sciter: Load DLL (Release) and bind embedded resources
bool InitSciter() {
	LPCWSTR dllPath;
	if (!EnsureSciterDll(dllPath)) {
		MessageBoxW(NULL, L"EnsureSciterDll() failed - could not extract or find sciter.dll", L"NextKey Error", MB_OK | MB_ICONERROR);
		return false;
	}
	
	// Load the DLL from our path
	HMODULE hSciter = LoadLibraryW(dllPath);
	if (!hSciter) {
		WCHAR errMsg[512];
		swprintf_s(errMsg, L"LoadLibraryW failed for: %s\nError code: %lu", dllPath, GetLastError());
		MessageBoxW(NULL, errMsg, L"NextKey Error", MB_OK | MB_ICONERROR);
		return false;
	}
	
	// Bind embedded UI resources (Release only, no-op in Debug)
	BindSciterResources();
	
	return true;
}

// Helper: Run a sciter dialog with single-instance mutex protection
template<typename DialogType>
int runSingleInstanceDialog(const wchar_t* mutexName, const wchar_t* windowTitle) {
	// Initialize Sciter DLL and resources first
	if (!InitSciter()) return 1;
	
	// Single instance check using named mutex
	HANDLE hMutex = CreateMutexW(NULL, TRUE, mutexName);
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		// Another instance is running - find and activate it
		HWND existingWnd = FindWindowW(NULL, windowTitle);
		if (existingWnd) {
			ForceForegroundWindow(existingWnd);
		}
		CloseHandle(hMutex);
		return 0;
	}
	
	// Enable Inspector for debugging
	SciterSetOption(NULL, SCITER_SET_DEBUG_MODE, TRUE);
	SciterSetOption(NULL, SCITER_SET_SCRIPT_RUNTIME_FEATURES,
		ALLOW_FILE_IO |
		ALLOW_SOCKET_IO |
		ALLOW_EVAL |
		ALLOW_SYSINFO);
	
	DialogType dialog;
	dialog.show();
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}

// Helper: Run simple dialog without mutex (no show() method)
template<typename DialogType>
int runSimpleDialog() {
	// Initialize Sciter DLL and resources first
	if (!InitSciter()) return 1;
	
	DialogType dialog;
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
						_In_opt_ HINSTANCE hPrevInstance,
						_In_ LPWSTR    lpCmdLine,
						_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	
	// ===== ROUTER: Single Exe - Multi Personality =====
	
	// About dialog subprocess
	if (lpCmdLine && wcsstr(lpCmdLine, L"--about")) {
		return runSimpleDialog<AboutDialog>();
	}
	
	// Settings dialog subprocess (with single-instance protection)
	if (lpCmdLine && wcsstr(lpCmdLine, L"--settings")) {
		return runSingleInstanceDialog<SettingsDialog>(
			L"NextKeySettingsDialogMutex", 
			L"NextKey Settings"
		);
	}
	
	// Macro dialog subprocess (with single-instance protection)
	if (lpCmdLine && wcsstr(lpCmdLine, L"--macro")) {
		return runSingleInstanceDialog<MacroDialogSciter>(
			L"NextKeyMacroDialogMutex", 
			L"B\u1EA3ng g\u00F5 t\u1EAFt"
		);
	}
	
	// ExcludedApps dialog subprocess (with single-instance protection)
	if (lpCmdLine && wcsstr(lpCmdLine, L"--excludedapps")) {
		return runSingleInstanceDialog<ExcludedAppsDialogSciter>(
			L"NextKeyExcludedAppsDialogMutex", 
			L"Lo\u1EA1i tr\u1EEB \u1EE9ng d\u1EE5ng"
		);
	}
	
	// ConvertTool dialog subprocess (with single-instance protection)
	if (lpCmdLine && wcsstr(lpCmdLine, L"--convert-tool")) {
		return runSingleInstanceDialog<ConvertToolDialogSciter>(
			L"NextKeyConvertToolDialogMutex", 
			L"C\u00F4ng c\u1EE5 chuy\u1EC3n m\u00E3"
		);
	}
	
	// AppOverrides dialog subprocess (unified per-app config)
	// "Cấu hình ứng dụng" = "C\u1EA5u h\u00ECnh \u1EE9ng d\u1EE5ng"
	if (lpCmdLine && wcsstr(lpCmdLine, L"--appoverrides")) {
		return runSingleInstanceDialog<AppOverridesDialogSciter>(
			L"NextKeyAppOverridesDialogMutex", 
			L"C\u1EA5u h\u00ECnh \u1EE9ng d\u1EE5ng"
		);
	}
	
	// ===== ENGINE PROCESS MODE: Normal app =====
	UNREFERENCED_PARAMETER(lpCmdLine);
	
	// NOTE: Admin mode is now handled by Task Scheduler with /rl highest
	// (configured via registerRunOnStartup when "Run as Admin" is enabled)
	// This removes the UAC prompt on every startup
	
	AppDelegate app;
	return app.run(hInstance);
}