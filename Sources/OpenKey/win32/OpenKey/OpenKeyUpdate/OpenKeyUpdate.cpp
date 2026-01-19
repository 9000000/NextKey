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
#include <shellapi.h>  // For ShellExecute
#include <TlHelp32.h>  // For CreateToolhelp32Snapshot, Process32First/Next
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
		// Set window always on top
		SetWindowPos(hDlg, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
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
	wsprintf(path, TEXT("%s\\_NextKey.tempf"), currentDir);
	
	// Fetch from GitHub Releases API (use correct fork)
	HRESULT res = URLDownloadToFile(NULL, L"https://api.github.com/repos/phatMT97/NextKey/releases/latest", path, 0, NULL);

	string data; //test
	if (res == S_OK) {
		std::ifstream t(path);
		std::stringstream buffer;
		buffer << t.rdbuf();
		t.close();
		DeleteFile(path);
		data = buffer.str();
	} else {
		MessageBox(hDlg, _T("Có lỗi trong quá trình cập nhật, vui lòng thử lại sau!"), _T("NextKey Update"), MB_OK);
		ExitProcess(0);
		return 0;
	}

	// Find download URL for correct architecture
#ifdef _WIN64
	string assetName = "NextKey-x64.zip";
#else
	string assetName = "NextKey-x86.zip";
#endif

	// Find browser_download_url for our architecture
	size_t assetPos = data.find(assetName);
	if (assetPos == string::npos) {
		MessageBox(hDlg, _T("Không tìm thấy file cập nhật cho kiến trúc này!"), _T("NextKey Update"), MB_OK);
		ExitProcess(0);
		return 0;
	}

	// Extract download URL (browser_download_url comes AFTER name in JSON)
	size_t urlKeyPos = data.find("\"browser_download_url\"", assetPos);
	if (urlKeyPos == string::npos) {
		MessageBox(hDlg, _T("Không tìm thấy đường dẫn tải file!"), _T("NextKey Update"), MB_OK);
		ExitProcess(0);
		return 0;
	}
	
	size_t urlColonPos = data.find(':', urlKeyPos);
	size_t urlQuoteStart = data.find('"', urlColonPos + 1);
	size_t urlQuoteEnd = data.find('"', urlQuoteStart + 1);
	
	if (urlQuoteStart == string::npos || urlQuoteEnd == string::npos) {
		MessageBox(hDlg, _T("Lỗi phân tích đường dẫn tải file!"), _T("NextKey Update"), MB_OK);
		ExitProcess(0);
		return 0;
	}
	
	string downloadUrlStr = data.substr(urlQuoteStart + 1, urlQuoteEnd - urlQuoteStart - 1);
	wstring downloadUrl(downloadUrlStr.begin(), downloadUrlStr.end());
	
	// Download zip file
	wsprintf(path, TEXT("%s\\_NextKeyUpdate.zip"), currentDir);
	res = URLDownloadToFile(NULL, downloadUrl.c_str(), path, 0, NULL);

	if (res == S_OK) {
		// Terminate main NextKey app first to release file lock
#ifdef _WIN64
		HWND mainWnd = FindWindowW(L"NextKeyVietnameseInputMethod", NULL);
		if (mainWnd) {
			DWORD processId = 0;
			GetWindowThreadProcessId(mainWnd, &processId);
			if (processId) {
				HANDLE hProcess = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, processId);
				if (hProcess) {
					TerminateProcess(hProcess, 0);
					WaitForSingleObject(hProcess, 3000);  // Wait up to 3 seconds
					CloseHandle(hProcess);
				}
			}
		}
		// Also try to terminate by process name (in case window not found)
		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hSnapshot != INVALID_HANDLE_VALUE) {
			PROCESSENTRY32W pe = { sizeof(pe) };
			if (Process32FirstW(hSnapshot, &pe)) {
				do {
					if (_wcsicmp(pe.szExeFile, L"NextKey64.exe") == 0) {
						HANDLE hProc = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, pe.th32ProcessID);
						if (hProc) {
							TerminateProcess(hProc, 0);
							WaitForSingleObject(hProc, 3000);
							CloseHandle(hProc);
						}
					}
				} while (Process32NextW(hSnapshot, &pe));
			}
			CloseHandle(hSnapshot);
		}
		Sleep(500);  // Extra wait for file handles to be released
		DeleteFile(L"NextKey64.exe");
#else
		HWND mainWnd = FindWindowW(L"NextKeyVietnameseInputMethod", NULL);
		if (mainWnd) {
			DWORD processId = 0;
			GetWindowThreadProcessId(mainWnd, &processId);
			if (processId) {
				HANDLE hProcess = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, processId);
				if (hProcess) {
					TerminateProcess(hProcess, 0);
					WaitForSingleObject(hProcess, 3000);
					CloseHandle(hProcess);
				}
			}
		}
		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hSnapshot != INVALID_HANDLE_VALUE) {
			PROCESSENTRY32W pe = { sizeof(pe) };
			if (Process32FirstW(hSnapshot, &pe)) {
				do {
					if (_wcsicmp(pe.szExeFile, L"NextKey32.exe") == 0) {
						HANDLE hProc = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, pe.th32ProcessID);
						if (hProc) {
							TerminateProcess(hProc, 0);
							WaitForSingleObject(hProc, 3000);
							CloseHandle(hProc);
						}
					}
				} while (Process32NextW(hSnapshot, &pe));
			}
			CloseHandle(hSnapshot);
		}
		Sleep(500);
		DeleteFile(L"NextKey32.exe");
#endif
		
		// Extract zip file using PowerShell with proper process waiting
		STARTUPINFOW si = { sizeof(si) };
		PROCESS_INFORMATION pi = { 0 };
		
		wstring psCmd = L"powershell.exe -NoProfile -NonInteractive -Command \"Expand-Archive -Path '.\\_NextKeyUpdate.zip' -DestinationPath '.\\_NextKeyUpdate' -Force\"";
		
		if (CreateProcessW(NULL, (LPWSTR)psCmd.c_str(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
			// Wait for PowerShell to finish (max 60 seconds)
			DWORD waitResult = WaitForSingleObject(pi.hProcess, 60000);
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			
			if (waitResult == WAIT_TIMEOUT) {
				MessageBox(hDlg, _T("Quá trình giải nén mất quá lâu. Vui lòng thử lại!"), _T("NextKey Update"), MB_OK | MB_ICONERROR);
				ExitProcess(0);
				return 0;
			}
		} else {
			MessageBox(hDlg, _T("Không thể chạy PowerShell để giải nén file!"), _T("NextKey Update"), MB_OK | MB_ICONERROR);
			ExitProcess(0);
			return 0;
		}
		
		// Move new executable with error checking
		BOOL moveSuccess = FALSE;
#ifdef _WIN64
		wstring srcExe = L"_NextKeyUpdate\\NextKey64.exe";
		wstring dstExe = L"NextKey64.exe";
#else
		wstring srcExe = L"_NextKeyUpdate\\NextKey32.exe";
		wstring dstExe = L"NextKey32.exe";
#endif
		
		// Check if source file exists
		if (GetFileAttributesW(srcExe.c_str()) == INVALID_FILE_ATTRIBUTES) {
			MessageBox(hDlg, _T("Không tìm thấy file sau khi giải nén! Có thể file zip không đúng định dạng."), _T("NextKey Update"), MB_OK | MB_ICONERROR);
			ExitProcess(0);
			return 0;
		}
		
		// Try to move file
		moveSuccess = MoveFileExW(srcExe.c_str(), dstExe.c_str(), MOVEFILE_REPLACE_EXISTING);
		
		if (!moveSuccess) {
			DWORD err = GetLastError();
			wchar_t errMsg[256];
			wsprintf(errMsg, L"Không thể thay thế file! Lỗi: %d\nFile có thể đang được sử dụng.", err);
			MessageBox(hDlg, errMsg, _T("NextKey Update"), MB_OK | MB_ICONERROR);
			ExitProcess(0);
			return 0;
		}
		
		// Also copy sciter.dll if exists
		wstring srcDll = L"_NextKeyUpdate\\sciter.dll";
		if (GetFileAttributesW(srcDll.c_str()) != INVALID_FILE_ATTRIBUTES) {
			MoveFileExW(srcDll.c_str(), L"sciter.dll", MOVEFILE_REPLACE_EXISTING);
		}
		
		// Cleanup
		DeleteFile(path);  // Delete zip file
		
		// Use cmd to recursively delete folder
		STARTUPINFOW siClean = { sizeof(siClean) };
		PROCESS_INFORMATION piClean = { 0 };
		wstring cleanCmd = L"cmd.exe /c rd /s /q \"_NextKeyUpdate\"";
		if (CreateProcessW(NULL, (LPWSTR)cleanCmd.c_str(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &siClean, &piClean)) {
			WaitForSingleObject(piClean.hProcess, 5000);
			CloseHandle(piClean.hProcess);
			CloseHandle(piClean.hThread);
		}
		
		MessageBox(hDlg, _T("Cập nhật thành công! NextKey sẽ tự động khởi động lại."), _T("NextKey Update"), MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
		
		// Restart NextKey app after successful update
#ifdef _WIN64
		ShellExecute(NULL, L"open", L"NextKey64.exe", NULL, NULL, SW_SHOWNORMAL);
#else
		ShellExecute(NULL, L"open", L"NextKey32.exe", NULL, NULL, SW_SHOWNORMAL);
#endif
		
		ExitProcess(0);
	} else {
		MessageBox(hDlg, _T("Có lỗi trong quá trình tải file cập nhật!"), _T("NextKey Update"), MB_OK | MB_ICONERROR | MB_TOPMOST);
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