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
#include "OpenKeyManager.h"
#include <shlobj.h>

static vector<LPCTSTR> _inputType = {
	_T("Telex"),
	_T("VNI"),
	_T("Simple Telex 1"),
	_T("Simple Telex 2"),
};

static vector<LPCTSTR> _tableCode = {
	_T("Unicode"),
	_T("TCVN3 (ABC)"),
	_T("VNI Windows"),
	_T("Unicode Tổ hợp"),
	_T("Vietnamese Locale CP 1258")
};

/*-----------------------------------------------------------------------*/

extern void OpenKeyInit();
extern void OpenKeyFree();
extern void ReinstallHooks();

unsigned short  OpenKeyManager::_lastKeyCode = 0;

vector<LPCTSTR>& OpenKeyManager::getInputType() {
	return _inputType;
}

vector<LPCTSTR>& OpenKeyManager::getTableCode() {
	return _tableCode;
}

void OpenKeyManager::initEngine() {
	OpenKeyInit();
}

void OpenKeyManager::freeEngine() {
	OpenKeyFree();
}

void OpenKeyManager::reinstallHooks() {
	ReinstallHooks();
}

// Store download URL for later use
static std::wstring _updateDownloadUrl;

std::wstring OpenKeyManager::getUpdateDownloadUrl() {
	return _updateDownloadUrl;
}

bool OpenKeyManager::checkUpdate(string& newVersion) {
	_updateDownloadUrl.clear();
	
	// Fetch from GitHub Releases API (your fork)
	wstring dataW = OpenKeyHelper::getContentOfUrl(L"https://api.github.com/repos/phatMT97/NextKey/releases/latest");
	string data = wideStringToUtf8(dataW);
	
	if (data.empty()) {
		return false;
	}
	
	// Parse tag_name from JSON response
	// Format: "tag_name": "v1.0.3-rc"
	size_t tagPos = data.find("\"tag_name\"");
	if (tagPos == string::npos) {
		return false;
	}
	
	// Find the version string after tag_name
	size_t colonPos = data.find(':', tagPos);
	size_t quoteStart = data.find('"', colonPos + 1);
	size_t quoteEnd = data.find('"', quoteStart + 1);
	
	if (quoteStart == string::npos || quoteEnd == string::npos) {
		return false;
	}
	
	string tagName = data.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
	
	// Extract version from tag (e.g., "v1.0.3-rc" -> "1.0.3")
	// Remove 'v' prefix if present
	string versionStr = tagName;
	if (!versionStr.empty() && (versionStr[0] == 'v' || versionStr[0] == 'V')) {
		versionStr = versionStr.substr(1);
	}
	// Remove suffix like "-rc", "-beta" for comparison
	size_t dashPos = versionStr.find('-');
	if (dashPos != string::npos) {
		versionStr = versionStr.substr(0, dashPos);
	}
	
	newVersion = tagName;  // Return full tag name for display (e.g., "v1.0.3-rc")
	
	// Parse version numbers (e.g., "1.0.3" -> 1, 0, 3)
	int major = 0, minor = 0, patch = 0;
	if (sscanf_s(versionStr.c_str(), "%d.%d.%d", &major, &minor, &patch) < 2) {
		return false;
	}
	DWORD remoteVersion = (major << 16) | (minor << 8) | patch;
	
	// Get current version
	DWORD currentVersion = OpenKeyHelper::getVersionNumber();
	
	// Find download URL for correct architecture
#ifdef _WIN64
	const char* assetName = "NextKey-x64.zip";
#else
	const char* assetName = "NextKey-x86.zip";
#endif
	
	// Find browser_download_url for our architecture
	size_t assetPos = data.find(assetName);
	if (assetPos != string::npos) {
		// Look for browser_download_url before this asset name
		size_t urlKeyPos = data.rfind("\"browser_download_url\"", assetPos);
		if (urlKeyPos != string::npos) {
			size_t urlColonPos = data.find(':', urlKeyPos);
			size_t urlQuoteStart = data.find('"', urlColonPos + 1);
			size_t urlQuoteEnd = data.find('"', urlQuoteStart + 1);
			
			if (urlQuoteStart != string::npos && urlQuoteEnd != string::npos) {
				string downloadUrl = data.substr(urlQuoteStart + 1, urlQuoteEnd - urlQuoteStart - 1);
				_updateDownloadUrl = utf8ToWideString(downloadUrl);
			}
		}
	}
	
	return remoteVersion > currentVersion;
}

void OpenKeyManager::createDesktopShortcut() {
	CoInitialize(NULL);
	IShellLink* pShellLink = NULL;
	HRESULT hres;
	hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_ALL,
							IID_IShellLink, (void**)&pShellLink);
	if (SUCCEEDED(hres)) {
		wstring path = OpenKeyHelper::getFullPath();
		pShellLink->SetPath(path.c_str());
		pShellLink->SetDescription(_T("NextKey - Bộ gõ Tiếng Việt"));
		pShellLink->SetIconLocation(path.c_str(), 0);

		IPersistFile* pPersistFile;
		hres = pShellLink->QueryInterface(IID_IPersistFile, (void**)&pPersistFile);

		if (SUCCEEDED(hres)) {
			wchar_t desktopPath[MAX_PATH + 1];
			wchar_t savePath[MAX_PATH + 10];
			SHGetFolderPath(NULL, CSIDL_DESKTOP, NULL, 0, desktopPath);
			wsprintf(savePath, _T("%s\\NextKey.lnk"), desktopPath);
			hres = pPersistFile->Save(savePath, TRUE);
			pPersistFile->Release();
			pShellLink->Release();
			
			// Notify Shell to refresh icon cache - fixes icon not showing on Windows 10
			SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
		}
	}
}

void OpenKeyManager::deleteDesktopShortcut() {
	wchar_t desktopPath[MAX_PATH + 1];
	wchar_t shortcutPath[MAX_PATH + 20];
	SHGetFolderPath(NULL, CSIDL_DESKTOP, NULL, 0, desktopPath);
	wsprintf(shortcutPath, _T("%s\\NextKey.lnk"), desktopPath);
	DeleteFile(shortcutPath);
}
