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
#include "OpenKeyHelper.h"
#include "ConfigManager.h"
#include <stdarg.h>
#include <Urlmon.h>
#include <fstream>
#include <sstream>
#include <unordered_map>

#pragma comment(lib, "version.lib")
#pragma comment(lib, "Urlmon.lib")

// Only keep registry for startup Run key - all other settings use ConfigManager
static HKEY hKey;
static LPCTSTR _runOnStartupKeyPath = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run");
static TCHAR _executePath[MAX_PATH];
static bool _hasGetPath = false;

static DWORD _tempProcessId = 0;
static HWND _tempWnd;
static TCHAR _exePath[1024] = { 0 };
static LPCTSTR _exeName = _exePath;
static HANDLE _proc;
static string _exeNameUtf8 = "TheOpenKeyProject";
static string _unknownProgram = "UnknownProgram";

// Multi-app process name cache with TTL + HWND validation
struct ProcessCacheEntry {
    std::string name;
    DWORD lastCheckTime;  // GetTickCount() when cached
    HWND hwnd;            // Window handle to detect PID reuse
};
static std::unordered_map<DWORD, ProcessCacheEntry> _processNameCache;
static const size_t MAX_PROCESS_CACHE_SIZE = 50;
static const DWORD CACHE_TTL_MS = 5000;  // 5 second TTL

int CF_RTF = RegisterClipboardFormat(_T("Rich Text Format"));
int CF_HTML = RegisterClipboardFormat(_T("HTML Format"));
int CF_OPENKEY = RegisterClipboardFormat(_T("OpenKey Format"));

// Windows Clipboard History exclusion format
// When this format is present, Windows will NOT save the clipboard content to history (Win+V)
// See: https://docs.microsoft.com/en-us/windows/win32/dataxchg/clipboard-formats
static UINT CF_EXCLUDE_CLIPBOARD_HISTORY = RegisterClipboardFormat(_T("ExcludeClipboardContentFromMonitorProcessing"));

// Note: Registry functions (setRegInt, getRegInt, etc.) removed
// All settings now use ConfigManager with TOML

void OpenKeyHelper::registerRunOnStartup(const int& val) {
	// Helper lambda to delete scheduled task with proper elevation
	auto deleteScheduledTask = []() {
		// Try non-elevated first (might work if task was created by current user)
		// If fail, use UAC elevation
		SHELLEXECUTEINFOW sei = { sizeof(sei) };
		sei.lpVerb = L"runas";  // Request elevation
		sei.lpFile = L"schtasks";
		sei.lpParameters = L"/delete /tn NextKey /f";
		sei.nShow = SW_HIDE;
		sei.fMask = SEE_MASK_NOCLOSEPROCESS;
		
		if (ShellExecuteExW(&sei)) {
			if (sei.hProcess) {
				WaitForSingleObject(sei.hProcess, 3000);
				CloseHandle(sei.hProcess);
			}
		}
	};

	if (val) {
		if (vRunAsAdmin) {
			// Use ShellExecuteEx with "runas" verb to request UAC for schtasks
			// This triggers UAC only ONCE when user enables "Run as Admin"
			wstring path = getFullPath();
			wstring taskCmd = L"/create /sc onlogon /tn NextKey /rl highest /tr \"\\\"" + path + L"\\\"\" /f";
			
			SHELLEXECUTEINFOW sei = { sizeof(sei) };
			sei.lpVerb = L"runas";
			sei.lpFile = L"schtasks";
			sei.lpParameters = taskCmd.c_str();
			sei.nShow = SW_HIDE;
			sei.fMask = SEE_MASK_NOCLOSEPROCESS;
			
			if (ShellExecuteExW(&sei)) {
				if (sei.hProcess) {
					WaitForSingleObject(sei.hProcess, 5000);  // Wait for schtasks to complete
					CloseHandle(sei.hProcess);
				}
				// Remove registry entry since we're using task scheduler
				RegOpenKeyEx(HKEY_CURRENT_USER, _runOnStartupKeyPath, NULL, KEY_ALL_ACCESS, &hKey);
				RegDeleteValue(hKey, _T("NextKey"));
				RegCloseKey(hKey);
			}
			// If user declined UAC, do nothing - startup won't be registered
		} else {
			// Non-admin: Use registry method
			// First, delete any existing admin task (requires UAC)
			deleteScheduledTask();
			
			// Then add registry entry
			RegOpenKeyEx(HKEY_CURRENT_USER, _runOnStartupKeyPath, NULL, KEY_ALL_ACCESS, &hKey);
			wstring path = getFullPath();
			RegSetValueEx(hKey, _T("NextKey"), 0, REG_SZ, (byte*)path.c_str(), ((DWORD)path.size() + 1) * sizeof(TCHAR));
			RegCloseKey(hKey);
		}
	} else {
		// Disable startup: remove both methods
		RegOpenKeyEx(HKEY_CURRENT_USER, _runOnStartupKeyPath, NULL, KEY_ALL_ACCESS, &hKey);
		RegDeleteValue(hKey, _T("NextKey"));
		RegCloseKey(hKey);
		// Delete scheduled task with elevation
		deleteScheduledTask();
	}
}

void OpenKeyHelper::resetAllSettings() {
	// Remove startup entries first
	RegOpenKeyEx(HKEY_CURRENT_USER, _runOnStartupKeyPath, NULL, KEY_ALL_ACCESS, &hKey);
	RegDeleteValue(hKey, _T("NextKey"));
	RegCloseKey(hKey);
	
	// Try to delete scheduled task (may fail if not elevated, that's ok)
	// Use non-elevated call - if task exists with admin rights, user should manually delete
	_wsystem(L"schtasks /delete /tn NextKey /f 2>nul");
	
	// Delete config.toml to reset settings
	std::wstring configPath = ConfigManager::instance().getConfigPath();
	DeleteFileW(configPath.c_str());
}

LPTSTR OpenKeyHelper::getExecutePath() {
	if (!_hasGetPath) {
		HMODULE hModule = GetModuleHandleW(NULL);
		GetModuleFileNameW(hModule, _executePath, MAX_PATH);
		_hasGetPath = true;
	}
	return _executePath;
}

string& OpenKeyHelper::getFrontMostAppExecuteName() {
	_tempWnd = GetForegroundWindow();
	GetWindowThreadProcessId(_tempWnd, &_tempProcessId);
	
	// Check multi-app cache first (validate HWND + TTL)
	auto cacheIt = _processNameCache.find(_tempProcessId);
	if (cacheIt != _processNameCache.end()) {
		// HWND must match (same window) AND TTL must not be expired
		if (cacheIt->second.hwnd == _tempWnd && 
			(GetTickCount() - cacheIt->second.lastCheckTime) < CACHE_TTL_MS) {
			_exeNameUtf8 = cacheIt->second.name;
			return _exeNameUtf8;
		}
		// Cache invalid (HWND changed or TTL expired) - will re-query below
	}
	
	// Cache miss - query OS with safer flags
	_proc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, _tempProcessId);
	if (_proc == NULL) {
		// Failed to open process (protected/system process)
		return _unknownProgram;
	}
	
	GetProcessImageFileName((HMODULE)_proc, _exePath, 1024);
	CloseHandle(_proc);
	
	if (wcscmp(_exePath, _T("")) == 0) {
		return _unknownProgram;
	}
	_exeName = _tcsrchr(_exePath, '\\') + 1;
	if (wcscmp(_exeName, _T("NextKey64.exe")) == 0 ||
		wcscmp(_exeName, _T("NextKey32.exe")) == 0 || 
		wcscmp(_exeName, _T("explorer.exe")) == 0) {
		return _exeNameUtf8;
	}
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, _exeName, (int)lstrlen(_exeName), NULL, 0, NULL, NULL);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, _exeName, (int)lstrlen(_exeName), &strTo[0], size_needed, NULL, NULL);
	_exeNameUtf8 = strTo;
	
	// Add to cache (with size limit to prevent memory growth)
	if (_processNameCache.size() >= MAX_PROCESS_CACHE_SIZE) {
		_processNameCache.clear();  // Simple eviction - clear all when full
	}
	_processNameCache[_tempProcessId] = {_exeNameUtf8, GetTickCount(), _tempWnd};
	
	return _exeNameUtf8;
}

string & OpenKeyHelper::getLastAppExecuteName() {
	if (!vUseSmartSwitchKey)
		return getFrontMostAppExecuteName();
	return _exeNameUtf8;
}

wstring OpenKeyHelper::getFullPath() {
	HMODULE hModule = GetModuleHandle(NULL);
	TCHAR path[MAX_PATH];
	GetModuleFileName(hModule, path, MAX_PATH);
	wstring rs(path);
	return rs;
}

wstring OpenKeyHelper::getClipboardText(const int& type) {
	// Try opening the clipboard
	if (!OpenClipboard(nullptr)) {
		return _T("");
	}

	// Get handle of clipboard object for ANSI text
	HANDLE hData = GetClipboardData(type);
	if (hData == nullptr) {
		return _T("");
	}

	// Lock the handle to get the actual text pointer
	wchar_t * pszText = static_cast<wchar_t*>(GlobalLock(hData));
	if (pszText == nullptr) {
		return _T("");
	}

	// Save text in a string class instance
	wstring text(pszText);
	
	// Release the lock
	GlobalUnlock(hData);

	// Release the clipboard
	CloseClipboard();
	
	return text;
}

bool OpenKeyHelper::setClipboardText(LPCTSTR data, const int & len, const int& type) {
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len * sizeof(WCHAR));
	if (!hMem) return false;
	
	memcpy(GlobalLock(hMem), data, len * sizeof(WCHAR));
	GlobalUnlock(hMem);
	
	if (!OpenClipboard(0)) {
		GlobalFree(hMem);
		return false;
	}
	
	EmptyClipboard();
	HANDLE result = SetClipboardData(type, hMem);
	// Exclude from Windows Clipboard History (Win+V)
	// This prevents OpenKey's typing from polluting user's clipboard history
	SetClipboardData(CF_EXCLUDE_CLIPBOARD_HISTORY, NULL);
	CloseClipboard();
	
	return (result != NULL);
}

bool OpenKeyHelper::quickConvert() {
	//read data from clipboard
	//support Unicode raw string, Rich Text Format and HTML

	if (!OpenClipboard(nullptr)) {
		return false;
	}

	string dataHTML, dataRTF;
	wstring dataUnicode;

	char* pHTML = 0, pRTF = 0;
	wchar_t* pUnicode = 0;

	//HTML
	HANDLE hData = GetClipboardData(CF_HTML);
	if (hData) {
		pHTML = static_cast<char*>(GlobalLock(hData));
		GlobalUnlock(hData);
	}
	if (pHTML) {
		dataHTML = pHTML;
		dataHTML = convertUtil(dataHTML);
	}

	//UNICODE
	hData = GetClipboardData(CF_UNICODETEXT);
	if (hData) {
		pUnicode = static_cast<wchar_t*>(GlobalLock(hData));
		GlobalUnlock(hData);
	}
	if (pUnicode) {
		dataUnicode = pUnicode;
		dataUnicode = utf8ToWideString(convertUtil(wideStringToUtf8(dataUnicode)));
	}

	OpenClipboard(0);
	EmptyClipboard();

	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (int)(dataHTML.size() + 1) * sizeof(char));
	memcpy(GlobalLock(hMem), dataHTML.c_str(), (int)(dataHTML.size() + 1) * sizeof(char));
	GlobalUnlock(hMem);
	SetClipboardData(CF_HTML, hMem);

	hMem = GlobalAlloc(GMEM_MOVEABLE, (int)(dataUnicode.size() + 1) * sizeof(wchar_t));
	memcpy(GlobalLock(hMem), dataUnicode.c_str(), (int)(dataUnicode.size() + 1) * sizeof(wchar_t));
	GlobalUnlock(hMem);
	SetClipboardData(CF_UNICODETEXT, hMem);

	CloseClipboard();
	return true;
}

DWORD OpenKeyHelper::getVersionNumber() {
	// get the filename of the executable containing the version resource
	TCHAR szFilename[MAX_PATH + 1] = { 0 };
	if (GetModuleFileName(NULL, szFilename, MAX_PATH) == 0) {
		return 0;
	}

	// allocate a block of memory for the version info
	DWORD dummy;
	UINT dwSize = GetFileVersionInfoSize(szFilename, &dummy);
	if (dwSize == 0) {
		return 0;
	}
	std::vector<BYTE> data(dwSize);

	// load the version info
	if (!GetFileVersionInfo(szFilename, NULL, dwSize, &data[0])) {
		return 0;
	}

	LPBYTE lpBuffer = NULL;

	if (VerQueryValue(&data[0], _T("\\"), (VOID FAR * FAR*) & lpBuffer, &dwSize)) {
		if (dwSize) {
			VS_FIXEDFILEINFO* verInfo = (VS_FIXEDFILEINFO*)lpBuffer;
			if (verInfo->dwSignature == 0xfeef04bd) {
				// Format: major in bits 16-31, minor in bits 8-15, patch in bits 0-7
				// FILEVERSION is stored as: MS = (major << 16) | minor, LS = (patch << 16) | build
				WORD major = (verInfo->dwFileVersionMS >> 16) & 0xffff;
				WORD minor = (verInfo->dwFileVersionMS >> 0) & 0xffff;
				WORD patch = (verInfo->dwFileVersionLS >> 16) & 0xffff;
				return (major << 16) | (minor << 8) | patch;
			}
		}
	}

	return 0;
}

wstring OpenKeyHelper::getVersionString() {
	// Get the filename of the executable containing the version resource
	TCHAR szFilename[MAX_PATH + 1] = { 0 };
	if (GetModuleFileName(NULL, szFilename, MAX_PATH) == 0) {
		return _T("");
	}

	// Allocate a block of memory for the version info
	DWORD dummy;
	UINT dwSize = GetFileVersionInfoSize(szFilename, &dummy);
	if (dwSize == 0) {
		return _T("");
	}
	std::vector<BYTE> data(dwSize);

	// Load the version info
	if (!GetFileVersionInfo(szFilename, NULL, dwSize, &data[0])) {
		return _T("");
	}

	// Query ProductVersion string (supports "1.0.3 RC", "1.0.3 Beta", etc.)
	LPWSTR lpBuffer = NULL;
	UINT bufLen = 0;
	if (VerQueryValue(&data[0], _T("\\StringFileInfo\\040904b0\\ProductVersion"), 
	                  (VOID FAR* FAR*)&lpBuffer, &bufLen)) {
		if (lpBuffer && bufLen > 0) {
			return wstring(lpBuffer);
		}
	}

	// Fallback to numeric version if string not found
	DWORD ver = getVersionNumber();
	TCHAR versionBuffer[MAX_PATH];
	wsprintfW(versionBuffer, _T("%d.%d.%d"), ver & 0xFF, (ver>>8) & 0xFF, (ver >> 16) & 0xFF);
	return wstring(versionBuffer);
}

// Helper function to get temp path (Standard Windows API)
static DWORD GetTempPathCompat(DWORD nBufferLength, LPWSTR lpBuffer) {
	// Just use GetTempPathW for maximum compatibility with older Windows 10/8/7
	return GetTempPathW(nBufferLength, lpBuffer);
}

wstring OpenKeyHelper::getContentOfUrl(LPCTSTR url){
	WCHAR path[MAX_PATH];
	GetTempPathCompat(MAX_PATH, path);
	wsprintf(path, TEXT("%s\\_NextKey.tempf"), path);
	HRESULT res = URLDownloadToFile(NULL, url, path, 0, NULL);
	
	if (res == S_OK) {
		std::wifstream t(path);
		std::wstringstream buffer;
		buffer << t.rdbuf();
		t.close();
		DeleteFile(path);
		return buffer.str();
	} else if (res == E_OUTOFMEMORY) {
		
	} else if (res == INET_E_DOWNLOAD_FAILURE) {
		
	} else {
		
	}
	return L"";
}

bool OpenKeyHelper::isWindowsDarkMode() {
	HKEY hKey;
	// Default to 1 (Light mode) - safe fallback for older Windows (7/8.1)
	// where this registry key doesn't exist
	DWORD value = 1;
	DWORD size = sizeof(value);
	
	// Check user personalization setting (Windows 10 1809+)
	if (RegOpenKeyEx(HKEY_CURRENT_USER, 
		L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
		0, KEY_READ, &hKey) == ERROR_SUCCESS) {
		
		RegQueryValueEx(hKey, L"AppsUseLightTheme", NULL, NULL, 
		                (LPBYTE)&value, &size);
		RegCloseKey(hKey);
	}
	
	// AppsUseLightTheme: 0 = Dark, 1 = Light
	return value == 0;
}

LRESULT OpenKeyHelper::handleIPCForeground(HWND hwnd) {
	ShowWindow(hwnd, SW_SHOW);
	if (IsIconic(hwnd)) ShowWindow(hwnd, SW_RESTORE);
	
	// Alt key trick for reliable focus
	keybd_event(VK_MENU, 0, 0, 0);
	SetForegroundWindow(hwnd);
	keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
	
	// TOPMOST -> NOTOPMOST trick
	SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
	SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
	BringWindowToTop(hwnd);
	return 0;
}