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

#include "framework.h"
#include "OpenKeyUpdate.h"
#include <Urlmon.h>
#include <fstream>
#include <sstream>
#include <string>
#pragma comment(lib, "Urlmon.lib")

using namespace std;

INT_PTR CALLBACK MainDialogProcess(HWND, UINT, WPARAM, LPARAM);
void StartUpdate();
HWND hDlg;
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

	hDlg = CreateDialogParam(hInstance, MAKEINTRESOURCE(IDD_DIALOG_UPDATER), 0, MainDialogProcess, 0);
	ShowWindow(hDlg, SW_SHOWNORMAL);
 
	MSG msg;
	// Main message loop:
	while (GetMessage(&msg, nullptr, 0, 0)) {
		if (!IsDialogMessage(hDlg, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK MainDialogProcess(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
	switch (message) {
	case WM_INITDIALOG:{
		HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_OPENKEYUPDATE));
		if (hIcon) {
			SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
		}
		StartUpdate();
		return (INT_PTR)TRUE;
	}
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

DWORD WINAPI UpdateThreadFunction(LPVOID lpParam) {
	WCHAR path[MAX_PATH];
	WCHAR currentDir[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, currentDir);
	wsprintf(path, TEXT("%s\\_OpenKey.tempf"), currentDir);
	
	// Fetch from GitHub Releases API (use correct fork)
	HRESULT res = URLDownloadToFile(NULL, L"https://api.github.com/repos/phatMT97/OpenKey/releases/latest", path, 0, NULL);

	string data; //test
	if (res == S_OK) {
		std::ifstream t(path);
		std::stringstream buffer;
		buffer << t.rdbuf();
		t.close();
		DeleteFile(path);
		data = buffer.str();
	} else {
		MessageBox(hDlg, _T("Có lỗi trong quá trình cập nhật, vui lòng thử lại sau!"), _T("OpenKey Update"), MB_OK);
		ExitProcess(0);
		return 0;
	}

	// Find download URL for correct architecture
#ifdef _WIN64
	string assetName = "OpenKey-x64.zip";
#else
	string assetName = "OpenKey-x86.zip";
#endif

	// Find browser_download_url for our architecture
	size_t assetPos = data.find(assetName);
	if (assetPos == string::npos) {
		MessageBox(hDlg, _T("Không tìm thấy file cập nhật cho kiến trúc này!"), _T("OpenKey Update"), MB_OK);
		ExitProcess(0);
		return 0;
	}

	// Extract download URL (browser_download_url comes AFTER name in JSON)
	size_t urlKeyPos = data.find("\"browser_download_url\"", assetPos);
	if (urlKeyPos == string::npos) {
		MessageBox(hDlg, _T("Không tìm thấy đường dẫn tải file!"), _T("OpenKey Update"), MB_OK);
		ExitProcess(0);
		return 0;
	}
	
	size_t urlColonPos = data.find(':', urlKeyPos);
	size_t urlQuoteStart = data.find('"', urlColonPos + 1);
	size_t urlQuoteEnd = data.find('"', urlQuoteStart + 1);
	
	if (urlQuoteStart == string::npos || urlQuoteEnd == string::npos) {
		MessageBox(hDlg, _T("Lỗi phân tích đường dẫn tải file!"), _T("OpenKey Update"), MB_OK);
		ExitProcess(0);
		return 0;
	}
	
	string downloadUrlStr = data.substr(urlQuoteStart + 1, urlQuoteEnd - urlQuoteStart - 1);
	wstring downloadUrl(downloadUrlStr.begin(), downloadUrlStr.end());
	
	// Download zip file
	wsprintf(path, TEXT("%s\\_OpenKeyUpdate.zip"), currentDir);
	res = URLDownloadToFile(NULL, downloadUrl.c_str(), path, 0, NULL);

	if (res == S_OK) {
		// Remove old files
#ifdef _WIN64
		DeleteFile(L"OpenKey64.exe");
#else
		DeleteFile(L"OpenKey32.exe");
#endif
		// Extract zip file using PowerShell
		WinExec("powershell.exe -NoP -NonI -Command \"Expand-Archive '.\\_OpenKeyUpdate.zip' '.\\_OpenKeyUpdate' -Force\" ", SW_HIDE);
		Sleep(5000);
		
		// Move new executable
#ifdef _WIN64
		MoveFile(L"_OpenKeyUpdate\\OpenKey64.exe", L"OpenKey64.exe");
#else
		MoveFile(L"_OpenKeyUpdate\\OpenKey32.exe", L"OpenKey32.exe");
#endif
		
		// Cleanup
		DeleteFile(path);  // Delete zip file
		// Use rd /s /q to recursively delete folder (RemoveDirectory only works on empty folders)
		WinExec("cmd.exe /c rd /s /q \"_OpenKeyUpdate\"", SW_HIDE);
		
		MessageBox(hDlg, _T("Bạn đã cập nhật OpenKey bản mới nhất thành công!"), _T("OpenKey Update"), MB_OK);
		ExitProcess(0);
	} else {
		MessageBox(hDlg, _T("Có lỗi trong quá trình cập nhật, vui lòng thử lại sau!"), _T("OpenKey Update"), MB_OK);
		ExitProcess(0);
	}
	return 0;
}

void StartUpdate() {
	DWORD hThread;
	HANDLE t = CreateThread(
							NULL,                   // default security attributes
							0,                      // use default stack size  
							UpdateThreadFunction,       // thread function name
							0,          // argument to thread function 
							0,                      // use default creation flags 
							&hThread);   // returns the thread identifier 
}