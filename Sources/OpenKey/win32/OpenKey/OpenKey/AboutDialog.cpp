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
#include "AboutDialog.h"
#include "SettingsDialog.h"
#include "AppDelegate.h"
#include "OpenKeyHelper.h"
#include "OpenKeyManager.h"
#include "../../../engine/Engine.h"
#include <shellapi.h>
#include <dwmapi.h>
#include <windowsx.h>  // For GET_X_LPARAM, GET_Y_LPARAM
#include <commctrl.h>  // For SetWindowSubclass
#include "sciter-x-dom.hpp"  // For SciterFindElement
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "comctl32.lib")

#include "ScaleHelper.h"

// Implement missing Sciter application function
namespace sciter {
	namespace application {
		HINSTANCE hinstance() {
			return GetModuleHandle(NULL);
		}
	}
}

AboutDialog::AboutDialog()
	: sciter::window(SW_POPUP, RECT{0, 0, 360, 320}) {
	
	// CRITICAL: Set transparent window option BEFORE load()
	// This allows Sciter to handle alpha channel properly
	SciterSetOption(get_hwnd(), SCITER_TRANSPARENT_WINDOW, 1);
	
#ifdef NDEBUG
	// Release: load from embedded resources (packed by packfolder.exe)
	if (!load(WSTR("this://app/about/about.html"))) {
		MessageBoxW(NULL, L"Failed to load about.html from resources", L"Error", MB_OK | MB_ICONERROR);
		return;
	}
#else
	// Debug: load from file (allows hot-reload during development)
	WCHAR exePath[MAX_PATH];
	GetModuleFileNameW(NULL, exePath, MAX_PATH);
	WCHAR* lastSlash = wcsrchr(exePath, L'\\');
	if (lastSlash) *lastSlash = L'\0';
	
	WCHAR htmlPath[MAX_PATH];
	swprintf_s(htmlPath, MAX_PATH, L"%s\\Resources\\Sciter\\about\\about.html", exePath);
	
	if (!load(htmlPath)) {
		MessageBoxW(NULL, htmlPath, L"Failed to load about.html", MB_OK | MB_ICONERROR);
		return;
	}
#endif

	// Show the window
	expand();
	
	// Set window title for anti-spam detection
	SetWindowTextW(get_hwnd(), L"V\u1EC1 NextKey");
	
	// Apply DPI scaling to window size
	int scaledWidth, scaledHeight;
	ScaleHelper::getScaledSize(360, 320, scaledWidth, scaledHeight);
	SetWindowPos(get_hwnd(), NULL, 0, 0, scaledWidth, scaledHeight, SWP_NOMOVE | SWP_NOZORDER);
	
	// Center window on screen
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	RECT rc;
	GetWindowRect(get_hwnd(), &rc);
	int winWidth = rc.right - rc.left;
	int winHeight = rc.bottom - rc.top;
	int x = (screenWidth - winWidth) / 2;
	int y = (screenHeight - winHeight) / 2;
	SetWindowPos(get_hwnd(), HWND_NOTOPMOST, x, y, 0, 0, SWP_NOSIZE);
	
	// Enable blur effect using shared SciterHelper (AFTER expand and SetWindowPos)
	SciterHelper::enableWindowBlur(get_hwnd(), SciterBlurMode::BM_BLUR);
	
	// Subclass window for dragging
	SetWindowSubclass(get_hwnd(), AboutDialog::SubclassProc, 1, 0);
}

AboutDialog::~AboutDialog() {
	if (get_hwnd()) {
		RemoveWindowSubclass(get_hwnd(), AboutDialog::SubclassProc, 1);
	}
}

// Subclass procedure for window dragging and close handling
LRESULT CALLBACK AboutDialog::SubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
	// WM_CLOSE: Must use ExitProcess for Sciter subprocesses!
	// PostQuitMessage causes Sciter reference counting assertion failure
	if (msg == WM_CLOSE) {
		ExitProcess(0);
		return 0;
	}
	
	// IPC: Bring window to foreground (sent from main process when window already exists)
	if (msg == WM_USER + 107) {
		return OpenKeyHelper::handleIPCForeground(hwnd);
	}
	
	if (msg == WM_NCHITTEST) {
		LRESULT result = SciterHelper::handleWindowDrag(hwnd, lParam, 50);
		if (result == HTCAPTION) return result;
		return DefSubclassProc(hwnd, msg, wParam, lParam);
	}
	
	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void AboutDialog::show() {
	// Show the window using Win32 API directly
	ShowWindow(get_hwnd(), SW_SHOW);
	SetForegroundWindow(get_hwnd());
}

// Handle Sciter DOM events - catches BUTTON_CLICK
bool AboutDialog::handle_event(HELEMENT he, BEHAVIOR_EVENT_PARAMS& params) {
	// First let base class handle it
	if (sciter::window::handle_event(he, params))
		return true;
	
	// Handle DOCUMENT_READY to apply dark mode
	if (params.cmd == DOCUMENT_READY) {
		// Apply dark/light theme based on Windows setting
		sciter::dom::element root = this->root();
		bool isDarkMode = OpenKeyHelper::isWindowsDarkMode();
		sciter::dom::element body = root.find_first("body");
		if (body) {
			if (isDarkMode) {
				body.set_attribute("class", L"dark");
			} else {
				body.remove_attribute("class");
			}
		}
		
		// Apply default background opacity (80%) for About dialog
		// Standardized: About dialog doesn't need to load config.toml just for opacity
		sciter::dom::element container = root.find_first(".container");
		if (container) {
			wchar_t bgColor[64];
			double opacity = 0.8; // Default 80%
			if (isDarkMode) {
				swprintf_s(bgColor, L"rgba(18, 20, 28, %.2f)", opacity * 0.9);
			} else {
				swprintf_s(bgColor, L"rgba(255, 255, 255, %.2f)", opacity);
			}
			container.set_style_attribute("background-color", bgColor);
		}
		return true;
	}
	
	if (params.cmd == BUTTON_CLICK) {
		sciter::dom::element el(params.heTarget);
		auto id = el.get_attribute("id");
		
		// Check for button clicks
		if (id == L"check-update") {
			checkUpdate();
			return true;
		}
		else if (id == L"btn-close") {
			PostMessage(get_hwnd(), WM_CLOSE, 0, 0);
			return true;
		}
		
		// Check for link buttons with data-url attribute
		auto dataUrl = el.get_attribute("data-url");
		if (dataUrl.length() > 0) {
			std::wstring wurl(dataUrl.c_str());
			std::string url(wurl.begin(), wurl.end());
			openUrl(url);
			return true;
		}
	}
	
	return false;
}

void AboutDialog::setVersionInfo() {
	// Get version and build date
	std::wstring version = OpenKeyHelper::getVersionString();
	std::wstring buildDate = _T(__DATE__);
	
	// Convert to UTF-8 for JavaScript
	std::string versionUtf8 = wideStringToUtf8(version);
	std::string buildDateUtf8 = wideStringToUtf8(buildDate);
	
	// Call JavaScript function to set version info
	call_function("setVersionInfo", versionUtf8.c_str(), buildDateUtf8.c_str());
}

void AboutDialog::openUrl(std::string url) {
	// Open URL in system default browser
	ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

void AboutDialog::checkUpdate() {
	std::string newVersion;
	bool hasUpdate = OpenKeyManager::checkUpdate(newVersion);
	
	// Call JavaScript function to show result
	// TODO: Implement proper JavaScript calling
	// call_function("showUpdateResult", hasUpdate, newVersion.c_str());
	
	// Temporary: show message box instead
	if (hasUpdate) {
		MessageBoxA(NULL, ("New version available: " + newVersion).c_str(), "Update", MB_OK);
	} else {
		MessageBoxA(NULL, "You are using the latest version!", "Update", MB_OK);
	}
}

void AboutDialog::closeWindow() {
	// Must use ExitProcess for Sciter subprocesses!
	ExitProcess(0);
}

void AboutDialog::showUpdateDialog(std::string message, std::string newVersion) {
	// Convert message to wide string
	std::wstring wideMessage = utf8ToWideString(message);
	
	int msgboxID = MessageBoxW(
		get_hwnd(),
		wideMessage.c_str(),
		L"OpenKey Update",
		MB_ICONEXCLAMATION | MB_YESNO
	);
	
	if (msgboxID == IDYES) {
		// Call OpenKeyUpdate
		WCHAR path[MAX_PATH];
		GetCurrentDirectoryW(MAX_PATH, path);
		wcscat_s(path, L"\\OpenKeyUpdate.exe");
		ShellExecuteW(0, L"", path, 0, 0, SW_SHOWNORMAL);
		
		AppDelegate::getInstance()->onOpenKeyExit();
	}
}

void AboutDialog::showInfoMessage(std::string message) {
	std::wstring wmessage = utf8ToWideString(message);
	MessageBoxW(get_hwnd(), wmessage.c_str(), L"OpenKey", MB_OK | MB_ICONINFORMATION);
}
// Note: Blur effect now handled by SciterHelper::enableWindowBlur()