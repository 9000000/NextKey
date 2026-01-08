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
#include "SystemTrayHelper.h"
#include "AppDelegate.h"
#include "OpenKeyManager.h"
#include <Wtsapi32.h>
#include <CommCtrl.h>

#pragma comment(lib, "Wtsapi32.lib")
#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Gdiplus.lib")
#pragma comment(lib, "D2d1.lib")
#pragma comment(lib, "Dwrite.lib")
#pragma comment(lib, "windowscodecs.lib")

#include <gdiplus.h>
#include <d2d1.h>
#include <dwrite.h>
#include <initguid.h>
#include <wincodec.h>

using namespace Gdiplus;

// Extern declaration for macro engine function
extern void initMacroMap(const Byte* pData, const int& size);

#define TIMER_REINSTALL_HOOKS 1001

#define WM_TRAYMESSAGE (WM_USER + 1)
#define TRAY_ICONUID 100

#define POPUP_VIET_ON_OFF 900
#define POPUP_SPELLING 901
#define POPUP_SMART_SWITCH 902
#define POPUP_USE_MACRO 903

#define POPUP_TELEX 910
#define POPUP_VNI 911
#define POPUP_SIMPLE_TELEX_1 912
#define POPUP_SIMPLE_TELEX_2 913

#define POPUP_UNICODE 930
#define POPUP_TCVN3 931
#define POPUP_VNI_WINDOWS 932
#define POPUP_UNICODE_COMPOUND 933
#define POPUP_VN_LOCALE_1258 934

#define POPUP_CONVERT_TOOL 980
#define POPUP_QUICK_CONVERT 981

#define POPUP_MACRO_TABLE 990

#define POPUP_CONTROL_PANEL 1000
#define POPUP_ABOUT_OPENKEY 1010
#define POPUP_OPENKEY_EXIT 2000

#define MODIFY_MENU(MENU, COMMAND, DATA) ModifyMenu(MENU, COMMAND, \
											MF_BYCOMMAND | (DATA ? MF_CHECKED : MF_UNCHECKED), \
											COMMAND, \
											menuData[COMMAND]);

static HMENU popupMenu;
//static HMENU menuInputType;
static HMENU otherCode;

static NOTIFYICONDATA nid;
static ULONGLONG lastUnlockTime = 0;

#define SESSION_UNLOCK_DEBOUNCE_MS 2000

map<UINT, LPCTSTR> menuData = {
	{POPUP_VIET_ON_OFF, _T("Bật Tiếng Việt")},
	{POPUP_SPELLING, _T("Bật kiểm tra chính tả")},
	{POPUP_SMART_SWITCH, _T("Bật loại trừ ứng dụng thông minh")},
	{POPUP_USE_MACRO, _T("Bật gõ tắt")},
	{POPUP_TELEX, _T("Kiểu gõ Telex")},
	{POPUP_VNI, _T("Kiểu gõ VNI")},
	{POPUP_SIMPLE_TELEX_1, _T("Kiểu gõ Simple Telex 1")},
	{POPUP_SIMPLE_TELEX_2, _T("Kiểu gõ Simple Telex 2")},
	{POPUP_UNICODE, _T("Unicode dựng sẵn")},
	{POPUP_TCVN3, _T("TCVN3 (ABC)")},
	{POPUP_VNI_WINDOWS, _T("VNI Windows")},
	{POPUP_UNICODE_COMPOUND, _T("Unicode tổ hợp")},
	{POPUP_VN_LOCALE_1258, _T("Vietnamese locale CP 1258")},
	{POPUP_CONVERT_TOOL, _T("Công cụ chuyển mã...")},
	{POPUP_QUICK_CONVERT, _T("Chuyển mã nhanh")},
	{POPUP_MACRO_TABLE, _T("Cấu hình gõ tắt...")},
	{POPUP_CONTROL_PANEL, _T("Bảng điều khiển...")},
	{POPUP_ABOUT_OPENKEY, _T("Giới thiệu NextKey")},
	{POPUP_OPENKEY_EXIT, _T("Thoát")},
};

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	static UINT taskbarCreated;

	switch (message) {
	case WM_CREATE:
		taskbarCreated = RegisterWindowMessage(_T("TaskbarCreated"));
		
		// Register session notification for lock/unlock detection
		if (!WTSRegisterSessionNotification(hWnd, NOTIFY_FOR_THIS_SESSION)) {
			OutputDebugString(_T("OpenKey: Failed to register session notification\n"));
		} else {
			OutputDebugString(_T("OpenKey: Session notification registered successfully\n"));
		}
		break;
	case WM_USER+2019:
		AppDelegate::getInstance()->onControlPanel();
		break;
	
	// Handle deferred AboutDialog destruction (posted from WM_CLOSE handler)
	case WM_USER+100:
		AppDelegate::getInstance()->closeAboutDialog();
		break;
	
	// Handle settings reload notification from SettingsDialog subprocess
	case WM_USER+101:
		LOG(L"[Main] WM_USER+101 received - reloading settings\n");
		// Reload settings from registry
		APP_GET_DATA(vLanguage, 1);
		APP_GET_DATA(vInputType, 0);
		APP_GET_DATA(vCodeTable, 0);
		APP_GET_DATA(vSwitchKeyStatus, 0);
		APP_GET_DATA(vUseSmartSwitchKey, 0);
		APP_GET_DATA(vCheckSpelling, 1);
		APP_GET_DATA(vUseModernOrthography, 0);
		// Bộ gõ tab settings
		APP_GET_DATA(vFixRecommendBrowser, 1);
		APP_GET_DATA(vUpperCaseFirstChar, 0);
		APP_GET_DATA(vRememberCode, 0);
		APP_GET_DATA(vRestoreIfWrongSpelling, 1);
		APP_GET_DATA(vAllowConsonantZFWJ, 0);
		APP_GET_DATA(vTempOffSpelling, 0);
		APP_GET_DATA(vTempOffOpenKey, 0);
		// Gõ tắt tab settings
		APP_GET_DATA(vUseMacro, 1);
		APP_GET_DATA(vUseMacroInEnglishMode, 0);
		APP_GET_DATA(vAutoCapsMacro, 0);
		APP_GET_DATA(vQuickTelex, 0);
		APP_GET_DATA(vQuickStartConsonant, 0);
		APP_GET_DATA(vQuickEndConsonant, 0);
		APP_GET_DATA(vExcludeApps, 0);
		// Khác tab settings
		APP_GET_DATA(vSupportMetroApp, 0);
		APP_GET_DATA(vUseGrayIcon, 0);
		APP_GET_DATA(vFixChromiumBrowser, 0);
		APP_GET_DATA(vSendKeyStepByStep, 1);  // Clipboard send keys
		// Convert Tool settings
		APP_GET_DATA(convertToolHotKey, 0);
		APP_GET_DATA(convertToolFromCode, 0);
		APP_GET_DATA(convertToolToCode, 0);
		APP_GET_DATA(convertToolToAllCaps, 0);
		APP_GET_DATA(convertToolToAllNonCaps, 0);
		APP_GET_DATA(convertToolRemoveMark, 0);
		APP_GET_DATA(convertToolToCapsEachWord, 0);
		APP_GET_DATA(convertToolToCapsFirstLetter, 0);
		APP_GET_DATA(convertToolDontAlertWhenCompleted, 0);
		// Tray icon colors
		vTrayIconColorV = (COLORREF)OpenKeyHelper::getRegInt(_T("vTrayIconColorV"), 0);
		vTrayIconColorE = (COLORREF)OpenKeyHelper::getRegInt(_T("vTrayIconColorE"), 0);
		LOG(L"[Main] Loaded colors: V=0x%08X, E=0x%08X\n", vTrayIconColorV, vTrayIconColorE);
		
		// Reload macro data from registry
		// NOTE: getRegBinary returns a static pointer - DO NOT delete[] it
		{
			DWORD macroDataSize = 0;
			BYTE* macroData = OpenKeyHelper::getRegBinary(_T("macroData"), macroDataSize);
			if (macroData && macroDataSize > 0) {
				initMacroMap(macroData, (int)macroDataSize);
				// Do NOT delete[] macroData - it's a static pointer managed by OpenKeyHelper
			} else {
				// Empty/deleted macro data - clear the macro map
				initMacroMap(nullptr, 0);
			}
		}
		
		// Reload English-only apps data from registry
		{
			extern void initEnglishOnlyApps(const Byte* pData, const int& size);
			DWORD appsDataSize = 0;
			BYTE* appsData = OpenKeyHelper::getRegBinary(_T("englishOnlyApps"), appsDataSize);
			if (appsData && appsDataSize > 0) {
				initEnglishOnlyApps(appsData, (int)appsDataSize);
			} else {
				initEnglishOnlyApps(nullptr, 0);
			}
		}
		
		// Reload special apps lists (Qt/Electron and Skip IME apps)
		{
			extern void reloadSpecialAppsLists();
			reloadSpecialAppsLists();
		}
		
		// Refresh tray icon and menu to reflect new settings
		SystemTrayHelper::updateData();
		break;
	
	// Handle macro table open request from SettingsDialog subprocess
	case WM_USER+103:
		AppDelegate::getInstance()->onMacroTable();
		break;
	
	// Handle excluded apps dialog open request from SettingsDialog subprocess
	case WM_USER+104:
		AppDelegate::getInstance()->onSpawnExcludedAppsSciter();
		break;
	
	// Handle manual update check request from SettingsDialog subprocess
	case WM_USER+105:
		AppDelegate::getInstance()->onCheckUpdate();
		break;
	
	// Handle special apps dialog open request from SettingsDialog subprocess
	case WM_USER+106:
		AppDelegate::getInstance()->onSpawnSpecialApps();
		break;
		
	// Handle session change (lock/unlock)
	case WM_WTSSESSION_CHANGE:
		if (wParam == WTS_SESSION_LOCK) {
			OutputDebugString(_T("OpenKey: Session locked\n"));
		} else if (wParam == WTS_SESSION_UNLOCK) {
			// Debounce: Only process if at least 2 seconds apart
			ULONGLONG now = GetTickCount64();
			if (lastUnlockTime == 0 || now - lastUnlockTime > SESSION_UNLOCK_DEBOUNCE_MS) {
				lastUnlockTime = now;
				OutputDebugString(_T("OpenKey: Session unlocked. Scheduling hook reinstall...\n"));
				
				// Use timer to delay 500ms, then reinstall from main thread
				SetTimer(hWnd, TIMER_REINSTALL_HOOKS, 500, NULL);
			}
		}
		break;
		
	// Handle timer for hook reinstallation
	case WM_TIMER:
		if (wParam == TIMER_REINSTALL_HOOKS) {
			KillTimer(hWnd, TIMER_REINSTALL_HOOKS);
			
			// CRITICAL: Called from main thread (has message loop)
			OutputDebugString(_T("OpenKey: Reinstalling hooks from main thread...\n"));
			OpenKeyManager::reinstallHooks();
		}
		break;
	case WM_TRAYMESSAGE: {
		if (lParam == WM_LBUTTONDBLCLK) {
			AppDelegate::getInstance()->onControlPanel();
		}
		if (lParam == WM_LBUTTONUP) {
			AppDelegate::getInstance()->onToggleVietnamese();
			SystemTrayHelper::updateData();
			// Notify settings subprocess to update UI if it's open
			HWND settingsWnd = FindWindow(NULL, _T("NextKey Settings"));
			if (settingsWnd) {
				PostMessage(settingsWnd, WM_USER + 102, 0, 0);
			}
		} else if (lParam == WM_RBUTTONDOWN) {
			POINT curPoint;
			GetCursorPos(&curPoint);
			SetForegroundWindow(hWnd);
			UINT commandId = TrackPopupMenu(
				popupMenu,
				TPM_RETURNCMD | TPM_NONOTIFY,
				curPoint.x,
				curPoint.y,
				0,
				hWnd,
				NULL
			);
			switch (commandId) {
			case POPUP_VIET_ON_OFF:
				AppDelegate::getInstance()->onToggleVietnamese();
				break;
			case POPUP_SPELLING:
				AppDelegate::getInstance()->onToggleCheckSpelling();
				break;
			case POPUP_SMART_SWITCH:
				AppDelegate::getInstance()->onToggleUseSmartSwitchKey();
				break;
			case POPUP_USE_MACRO:
				AppDelegate::getInstance()->onToggleUseMacro();
				break;
			case POPUP_MACRO_TABLE:
				AppDelegate::getInstance()->onMacroTable();
				break;
			case POPUP_CONVERT_TOOL:
				AppDelegate::getInstance()->onConvertTool();
				break;
			case POPUP_QUICK_CONVERT:
				AppDelegate::getInstance()->onQuickConvert();
				break;
			case POPUP_TELEX:
				AppDelegate::getInstance()->onInputType(0);
				break;
			case POPUP_VNI:
				AppDelegate::getInstance()->onInputType(1);
				break;
			case POPUP_SIMPLE_TELEX_1:
				AppDelegate::getInstance()->onInputType(2);
				break;
			case POPUP_SIMPLE_TELEX_2:
				AppDelegate::getInstance()->onInputType(3);
				break;
			case POPUP_UNICODE:
				AppDelegate::getInstance()->onTableCode(0);
				break;
			case POPUP_TCVN3:
				AppDelegate::getInstance()->onTableCode(1);
				break;
			case POPUP_VNI_WINDOWS:
				AppDelegate::getInstance()->onTableCode(2);
				break;
			case POPUP_UNICODE_COMPOUND:
				AppDelegate::getInstance()->onTableCode(3);
				break;
			case POPUP_VN_LOCALE_1258:
				AppDelegate::getInstance()->onTableCode(4);
				break;
			case POPUP_CONTROL_PANEL:
				AppDelegate::getInstance()->onControlPanel();
				break;
			case POPUP_ABOUT_OPENKEY:
				AppDelegate::getInstance()->onOpenKeyAbout();
				break;
			case POPUP_OPENKEY_EXIT:
				AppDelegate::getInstance()->onOpenKeyExit();
				break;
			}
			SystemTrayHelper::updateData();
			
			// Notify settings subprocess to update UI if it's open
			HWND settingsWnd = FindWindow(NULL, _T("NextKey Settings"));
			if (settingsWnd) {
				PostMessage(settingsWnd, WM_USER + 102, 0, 0);
			}
		}
	}
	break;
	
	case WM_DESTROY:
		// Kill timer if still active
		KillTimer(hWnd, TIMER_REINSTALL_HOOKS);
		
		// Unregister session notification on destroy
		WTSUnRegisterSessionNotification(hWnd);
		OutputDebugString(_T("OpenKey: Session notification unregistered\n"));
		break;
		
	default:
		// if the taskbar is restarted, add the system tray icon again
		if (message == taskbarCreated) {
			Shell_NotifyIcon(NIM_ADD, &nid);
		}
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

HWND SystemTrayHelper::createFakeWindow(const HINSTANCE & hIns) {
	//create fake window
	WNDCLASSEXW wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = 0;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hIns;
	wcex.hIcon = LoadIcon(hIns, MAKEINTRESOURCE(IDI_APP_ICON));
	wcex.hCursor = NULL;
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = APP_CLASS;
	wcex.hIconSm = NULL;
	ATOM atom = RegisterClassExW(&wcex);
	HWND hWnd = CreateWindowW(APP_CLASS, _T(""), WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hIns, nullptr);
	if (!hWnd) {
		return NULL;
	}
	ShowWindow(hWnd, 0);
	UpdateWindow(hWnd);
	return hWnd;
}

void SystemTrayHelper::createPopupMenu() {
	popupMenu = CreatePopupMenu();
	AppendMenu(popupMenu, MF_CHECKED, POPUP_VIET_ON_OFF, menuData[POPUP_VIET_ON_OFF]);
	AppendMenu(popupMenu, MF_SEPARATOR, 0, 0);
	AppendMenu(popupMenu, MF_CHECKED, POPUP_SPELLING, menuData[POPUP_SPELLING]);
	AppendMenu(popupMenu, MF_CHECKED, POPUP_SMART_SWITCH, menuData[POPUP_SMART_SWITCH]);
	AppendMenu(popupMenu, MF_CHECKED, POPUP_USE_MACRO, menuData[POPUP_USE_MACRO]);
	AppendMenu(popupMenu, MF_SEPARATOR, 0, 0);
	AppendMenu(popupMenu, MF_UNCHECKED, POPUP_MACRO_TABLE, menuData[POPUP_MACRO_TABLE]);
	AppendMenu(popupMenu, MF_UNCHECKED, POPUP_CONVERT_TOOL, menuData[POPUP_CONVERT_TOOL]);
	AppendMenu(popupMenu, MF_UNCHECKED, POPUP_QUICK_CONVERT, menuData[POPUP_QUICK_CONVERT]);
	AppendMenu(popupMenu, MF_SEPARATOR, 0, 0);

	//menuInputType = CreatePopupMenu();
	AppendMenu(popupMenu, MF_CHECKED, POPUP_TELEX, menuData[POPUP_TELEX]);
	AppendMenu(popupMenu, MF_CHECKED, POPUP_VNI, menuData[POPUP_VNI]);
	AppendMenu(popupMenu, MF_CHECKED, POPUP_SIMPLE_TELEX_1, menuData[POPUP_SIMPLE_TELEX_1]);
	AppendMenu(popupMenu, MF_CHECKED, POPUP_SIMPLE_TELEX_2, menuData[POPUP_SIMPLE_TELEX_2]);

	//AppendMenu(popupMenu, MF_POPUP, (UINT_PTR)menuInputType, _T("Kiểu gõ"));
	AppendMenu(popupMenu, MF_SEPARATOR, 0, 0);

	AppendMenu(popupMenu, MF_UNCHECKED, POPUP_UNICODE, menuData[POPUP_UNICODE]);
	AppendMenu(popupMenu, MF_UNCHECKED, POPUP_TCVN3, menuData[POPUP_TCVN3]);
	AppendMenu(popupMenu, MF_UNCHECKED, POPUP_VNI_WINDOWS, menuData[POPUP_VNI_WINDOWS]);

	otherCode = CreatePopupMenu();
	AppendMenu(otherCode, MF_CHECKED, POPUP_UNICODE_COMPOUND, menuData[POPUP_UNICODE_COMPOUND]);
	AppendMenu(otherCode, MF_CHECKED, POPUP_VN_LOCALE_1258, menuData[POPUP_VN_LOCALE_1258]);
	AppendMenu(popupMenu, MF_POPUP, (UINT_PTR)otherCode, _T("Bảng mã khác"));

	AppendMenu(popupMenu, MF_SEPARATOR, 0, 0);

	AppendMenu(popupMenu, MF_STRING, POPUP_CONTROL_PANEL, menuData[POPUP_CONTROL_PANEL]);
	AppendMenu(popupMenu, MF_UNCHECKED, POPUP_ABOUT_OPENKEY, menuData[POPUP_ABOUT_OPENKEY]);
	AppendMenu(popupMenu, MF_SEPARATOR, 0, 0);
	AppendMenu(popupMenu, MF_UNCHECKED, POPUP_OPENKEY_EXIT, menuData[POPUP_OPENKEY_EXIT]);

	SetMenuDefaultItem(popupMenu, POPUP_CONTROL_PANEL, false);
}

// GDI+ initialization token
static ULONG_PTR gdiplusToken = 0;

// Initialize GDI+ (call once at startup)
static void initGdiPlus() {
	if (gdiplusToken == 0) {
		GdiplusStartupInput gdiplusStartupInput;
		GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	}
}

// Use shared default color constants from stdafx.h:
// TRAY_DEFAULT_COLOR_V and TRAY_DEFAULT_COLOR_E

// Create dynamic tray icon with custom font using DirectWrite
// DirectWrite provides better text quality than GDI+
// Parameters:
//   letter: "V" or "E"
//   color: COLORREF for text color
//   fontName: Font face name
// Returns: HICON (caller must DestroyIcon when done)
static HICON createDynamicTrayIconWithFont(const wchar_t* letter, COLORREF color, const wchar_t* fontName) {
	// Get system small icon size (DPI-aware)
	int iconSize = GetSystemMetrics(SM_CXSMICON);
	if (iconSize < 16) iconSize = 16;
	
	// Use supersampling for better quality
	const int scale = 4;
	int renderSize = iconSize * scale;
	
	// Initialize COM (needed for WIC)
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	
	// Create DirectWrite factory
	IDWriteFactory* pDWriteFactory = NULL;
	HRESULT hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, 
		__uuidof(IDWriteFactory), (IUnknown**)&pDWriteFactory);
	if (FAILED(hr)) return NULL;
	
	// Create text format
	IDWriteTextFormat* pTextFormat = NULL;
	hr = pDWriteFactory->CreateTextFormat(
		fontName,
		NULL,  // Font collection (NULL = system fonts)
		DWRITE_FONT_WEIGHT_BOLD,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		(FLOAT)(renderSize * 1.2f),  // Font size - 120% of render size
		L"",  // Locale
		&pTextFormat);
	
	if (FAILED(hr)) {
		// Fallback to Arial
		hr = pDWriteFactory->CreateTextFormat(L"Arial", NULL,
			DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL, (FLOAT)(renderSize * 1.2f), L"", &pTextFormat);
		if (FAILED(hr)) {
			pDWriteFactory->Release();
			return NULL;
		}
	}
	
	// Center text
	pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
	
	// Create WIC factory for bitmap
	IWICImagingFactory* pWICFactory = NULL;
	hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
		IID_IWICImagingFactory, (void**)&pWICFactory);
	if (FAILED(hr)) {
		pTextFormat->Release();
		pDWriteFactory->Release();
		return NULL;
	}
	
	// Create WIC bitmap
	IWICBitmap* pWICBitmap = NULL;
	hr = pWICFactory->CreateBitmap(renderSize, renderSize, 
		GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnDemand, &pWICBitmap);
	if (FAILED(hr)) {
		pWICFactory->Release();
		pTextFormat->Release();
		pDWriteFactory->Release();
		return NULL;
	}
	
	// Create D2D factory
	ID2D1Factory* pD2DFactory = NULL;
	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2DFactory);
	if (FAILED(hr)) {
		pWICBitmap->Release();
		pWICFactory->Release();
		pTextFormat->Release();
		pDWriteFactory->Release();
		return NULL;
	}
	
	// Create D2D render target on WIC bitmap
	D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties(
		D2D1_RENDER_TARGET_TYPE_DEFAULT,
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
	
	ID2D1RenderTarget* pRT = NULL;
	hr = pD2DFactory->CreateWicBitmapRenderTarget(pWICBitmap, rtProps, &pRT);
	if (FAILED(hr)) {
		LOG(L"[DWrite] CreateWicBitmapRenderTarget failed: 0x%08X\n", hr);
		pD2DFactory->Release();
		pWICBitmap->Release();
		pWICFactory->Release();
		pTextFormat->Release();
		pDWriteFactory->Release();
		return NULL;
	}
	
	// Set text antialiasing mode - GRAYSCALE works with transparent background (not ClearType)
	pRT->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
	
	// Create brush with user color
	ID2D1SolidColorBrush* pBrush = NULL;
	D2D1_COLOR_F d2dColor = D2D1::ColorF(
		GetRValue(color) / 255.0f,
		GetGValue(color) / 255.0f,
		GetBValue(color) / 255.0f,
		1.0f);
	hr = pRT->CreateSolidColorBrush(d2dColor, &pBrush);
	if (FAILED(hr)) {
		LOG(L"[DWrite] CreateSolidColorBrush failed: 0x%08X\n", hr);
	}
	
	// Draw text
	pRT->BeginDraw();
	pRT->Clear(D2D1::ColorF(0, 0, 0, 0));  // Transparent background
	
	D2D1_RECT_F layoutRect = D2D1::RectF(0, 0, (FLOAT)renderSize, (FLOAT)renderSize);
	pRT->DrawText(letter, (UINT32)wcslen(letter), pTextFormat, layoutRect, pBrush);
	
	hr = pRT->EndDraw();
	if (FAILED(hr)) {
		LOG(L"[DWrite] EndDraw failed: 0x%08X\n", hr);
	}
	
	// Get bitmap data
	WICRect lockRect = { 0, 0, renderSize, renderSize };
	IWICBitmapLock* pLock = NULL;
	pWICBitmap->Lock(&lockRect, WICBitmapLockRead, &pLock);
	
	UINT stride = 0;
	UINT bufferSize = 0;
	BYTE* pData = NULL;
	pLock->GetStride(&stride);
	pLock->GetDataPointer(&bufferSize, &pData);
	
	// Scale down using GDI+ (high quality bicubic)
	initGdiPlus();
	Bitmap largeBmp(renderSize, renderSize, stride, PixelFormat32bppPARGB, pData);
	
	Bitmap finalBmp(iconSize, iconSize, PixelFormat32bppARGB);
	Graphics finalGfx(&finalBmp);
	finalGfx.SetInterpolationMode(InterpolationModeHighQualityBicubic);
	finalGfx.SetSmoothingMode(SmoothingModeHighQuality);
	finalGfx.SetPixelOffsetMode(PixelOffsetModeHighQuality);
	finalGfx.DrawImage(&largeBmp, 0, 0, iconSize, iconSize);
	
	// Convert to HICON
	HICON hIcon = NULL;
	finalBmp.GetHICON(&hIcon);
	
	// Cleanup
	pLock->Release();
	pBrush->Release();
	pRT->Release();
	pD2DFactory->Release();
	pWICBitmap->Release();
	pWICFactory->Release();
	pTextFormat->Release();
	pDWriteFactory->Release();
	
	return hIcon;
}


// Create custom colored tray icon by colorizing existing icon
// Uses direct Windows API (no GDI+ Bitmap conversion to preserve quality)
// Parameters:
//   baseIconId: Resource ID of base icon (V or E)
//   newColor: COLORREF (RGB value) for new color
// Returns: HICON with new color (caller must DestroyIcon when done)
static HICON createColorizedTrayIcon(int baseIconId, COLORREF newColor) {
	// Load the base icon using LoadIconMetric for DPI-aware size
	HICON hBaseIcon = NULL;
	HRESULT hr = LoadIconMetric(GetModuleHandle(0), MAKEINTRESOURCE(baseIconId), LIM_SMALL, &hBaseIcon);
	if (!SUCCEEDED(hr) || !hBaseIcon) {
		hBaseIcon = LoadIcon(GetModuleHandle(0), MAKEINTRESOURCE(baseIconId));
		if (!hBaseIcon) return NULL;
	}
	
	// Get icon info
	ICONINFO iconInfo;
	if (!GetIconInfo(hBaseIcon, &iconInfo)) {
		DestroyIcon(hBaseIcon);
		return NULL;
	}
	
	// Get color bitmap info
	BITMAP bm;
	if (!GetObject(iconInfo.hbmColor, sizeof(bm), &bm)) {
		DeleteObject(iconInfo.hbmMask);
		DeleteObject(iconInfo.hbmColor);
		DestroyIcon(hBaseIcon);
		return NULL;
	}
	
	// Create DIB section for direct pixel access
	BITMAPINFO bmi = {};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = bm.bmWidth;
	bmi.bmiHeader.biHeight = -bm.bmHeight;  // Top-down
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	
	// Get the color bitmap bits
	HDC hdc = GetDC(NULL);
	BYTE* pixels = new BYTE[bm.bmWidth * bm.bmHeight * 4];
	
	if (GetDIBits(hdc, iconInfo.hbmColor, 0, bm.bmHeight, pixels, &bmi, DIB_RGB_COLORS)) {
		// Get new color components
		BYTE newR = GetRValue(newColor);
		BYTE newG = GetGValue(newColor);
		BYTE newB = GetBValue(newColor);
		
		// Process each pixel - DIB is BGRA format
		for (int i = 0; i < bm.bmWidth * bm.bmHeight; i++) {
			BYTE* pixel = pixels + i * 4;
			BYTE alpha = pixel[3];
			
			// Only change pixels that have some opacity
			if (alpha > 0) {
				pixel[0] = newB;  // Blue
				pixel[1] = newG;  // Green
				pixel[2] = newR;  // Red
				// Keep alpha unchanged
			}
		}
		
		// Set the modified bits back
		SetDIBits(hdc, iconInfo.hbmColor, 0, bm.bmHeight, pixels, &bmi, DIB_RGB_COLORS);
	}
	
	delete[] pixels;
	ReleaseDC(NULL, hdc);
	
	// Create new icon from modified bitmaps
	HICON hNewIcon = CreateIconIndirect(&iconInfo);
	
	// Cleanup
	DeleteObject(iconInfo.hbmMask);
	DeleteObject(iconInfo.hbmColor);
	DestroyIcon(hBaseIcon);
	
	return hNewIcon;
}


static void loadTrayIcon() {
	// Update tooltip based on language
	if (vLanguage) {
		LoadString(GetModuleHandle(0), IDS_TRAY_TITLE_2, nid.szTip, 128);
	} else {
		LoadString(GetModuleHandle(0), IDS_TRAY_TITLE, nid.szTip, 128);
	}
	
	// Auto-save default colors when Custom mode is selected but colors are not set
	// This handles edge case: user upgraded app or registry was reset while vUseGrayIcon=3
	if (vUseGrayIcon == 3 && vTrayIconColorV == 0 && vTrayIconColorE == 0) {
		vTrayIconColorV = TRAY_DEFAULT_COLOR_V;
		vTrayIconColorE = TRAY_DEFAULT_COLOR_E;
		APP_SET_DATA(vTrayIconColorV, vTrayIconColorV);
		APP_SET_DATA(vTrayIconColorE, vTrayIconColorE);
		LOG(L"[loadTrayIcon] Auto-saved default colors for Custom mode: V=0x%08X, E=0x%08X\n", 
			vTrayIconColorV, vTrayIconColorE);
	}
	
	// Check if custom color mode is selected (vUseGrayIcon == 3 means Custom)
	// Now colors are guaranteed to be set if mode is Custom
	bool useCustomColor = (vUseGrayIcon == 3);
	LOG(L"[loadTrayIcon] vUseGrayIcon=%d, useCustomColor=%d, ColorV=0x%08X, ColorE=0x%08X\n", 
		vUseGrayIcon, useCustomColor ? 1 : 0, vTrayIconColorV, vTrayIconColorE);
	
	HICON hIcon = NULL;
	
	if (useCustomColor) {
		// Use colorized version of base icon (best quality - preserves original icon)
		int baseIcon = vLanguage ? IDI_ICON_STATUS_VIET : IDI_ICON_STATUS_ENG;
		COLORREF customColor = vLanguage 
			? (vTrayIconColorV != 0 ? vTrayIconColorV : TRAY_DEFAULT_COLOR_V)
			: (vTrayIconColorE != 0 ? vTrayIconColorE : TRAY_DEFAULT_COLOR_E);
		
		LOG(L"[loadTrayIcon] Using colorized icon, baseIcon=%d, color=0x%08X\n", baseIcon, customColor);
		hIcon = createColorizedTrayIcon(baseIcon, customColor);
	} else {
		// Fall back to static icons based on vUseGrayIcon setting
		int icon = 0;
		if (vLanguage) {
			switch (vUseGrayIcon) {
				case 1: icon = IDI_ICON_STATUS_VIET_10; break;     // White
				case 2: icon = IDI_ICON_STATUS_VIET_BLACK; break;  // Black
				default: icon = IDI_ICON_STATUS_VIET; break;       // Color
			}
		} else {
			switch (vUseGrayIcon) {
				case 1: icon = IDI_ICON_STATUS_ENG_10; break;     // White
				case 2: icon = IDI_ICON_STATUS_ENG_BLACK; break;  // Black
				default: icon = IDI_ICON_STATUS_ENG; break;       // Color
			}
		}
		
		// Use LoadIconMetric for better High DPI scaling
		HRESULT hr = LoadIconMetric(GetModuleHandle(0), MAKEINTRESOURCE(icon), LIM_SMALL, &hIcon);
		if (!SUCCEEDED(hr)) {
			// Fallback to LoadIcon if LoadIconMetric fails
			hIcon = LoadIcon(GetModuleHandle(0), MAKEINTRESOURCE(icon));
		}
	}
	
	// Update the icon
	if (hIcon) {
		// Destroy old icon if any
		if (nid.hIcon) {
			DestroyIcon(nid.hIcon);
		}
		nid.hIcon = hIcon;
	}
}

void SystemTrayHelper::updateData() {
	loadTrayIcon();
	Shell_NotifyIcon(NIM_MODIFY, &nid);

	MODIFY_MENU(popupMenu, POPUP_VIET_ON_OFF, vLanguage);
	MODIFY_MENU(popupMenu, POPUP_SPELLING, vCheckSpelling);
	MODIFY_MENU(popupMenu, POPUP_SMART_SWITCH, vUseSmartSwitchKey);
	MODIFY_MENU(popupMenu, POPUP_USE_MACRO, vUseMacro);
	MODIFY_MENU(popupMenu, POPUP_TELEX, vInputType == 0);
	MODIFY_MENU(popupMenu, POPUP_VNI, vInputType == 1);
	MODIFY_MENU(popupMenu, POPUP_SIMPLE_TELEX_1, vInputType == 2);
	MODIFY_MENU(popupMenu, POPUP_SIMPLE_TELEX_2, vInputType == 3);
	MODIFY_MENU(popupMenu, POPUP_UNICODE, vCodeTable == 0);
	MODIFY_MENU(popupMenu, POPUP_TCVN3, vCodeTable == 1);
	MODIFY_MENU(popupMenu, POPUP_VNI_WINDOWS, vCodeTable == 2);
	MODIFY_MENU(otherCode, POPUP_UNICODE_COMPOUND, vCodeTable == 3);
	MODIFY_MENU(otherCode, POPUP_VN_LOCALE_1258, vCodeTable == 4);

	wstring hotkey = L"";
	bool hasAdd = false;
	if (convertToolHotKey & 0x100) {
		hotkey += L"Ctrl";
		hasAdd = true;
	}
	if (convertToolHotKey & 0x200) {
		if (hasAdd)
			hotkey += L" + ";
		hotkey += L"Alt";
		hasAdd = true;
	}
	if (convertToolHotKey & 0x400) {
		if (hasAdd)
			hotkey += L" + ";
		hotkey += L"Win";
		hasAdd = true;
	}
	if (convertToolHotKey & 0x800) {
		if (hasAdd)
			hotkey += L" + ";
		hotkey += L"Shift";
		hasAdd = true;
	}

	unsigned short k = ((convertToolHotKey >> 24) & 0xFF);
	if (k != 0xFE) {
		if (hasAdd)
			hotkey += L" + ";
		if (k == VK_SPACE)
			hotkey += L"Space";
		else
			hotkey += (wchar_t)k;
	}

	wstring hotKeyString = menuData[POPUP_QUICK_CONVERT];
	if (hasAdd) {
		hotKeyString += L" - [";
		hotKeyString += hotkey;
		hotKeyString += L"]";
	}
	ModifyMenu(popupMenu, POPUP_QUICK_CONVERT, MF_BYCOMMAND | MF_UNCHECKED, POPUP_QUICK_CONVERT, hotKeyString.c_str());
}

static HINSTANCE ins;
static int recreateCount = 0;

void SystemTrayHelper::_createSystemTrayIcon(const HINSTANCE& hIns) {
	HWND hWnd = createFakeWindow(ins);
	
	if (hWnd == NULL) { //Use timer to create
		if (recreateCount >= 5) {
			PostQuitMessage(0);
			return;
		}
		ins = hIns;
		SetTimer(NULL, 0, 1000 * 3, (TIMERPROC)&WaitToCreateFakeWindow);
		++recreateCount;
		return;
	}
	createPopupMenu();

	//create system tray
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hWnd;
	nid.uID = TRAY_ICONUID;
	nid.uVersion = NOTIFYICON_VERSION;
	nid.uCallbackMessage = WM_TRAYMESSAGE;
	loadTrayIcon();
	LoadString(ins, IDS_APP_TITLE, nid.szTip, 128);
	nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;

	// Shell_NotifyIcon may fail if the system tray icon is not fully initialized
	const int maxRetries = 5;
	for (int attempt = 0; attempt < maxRetries; ++attempt) {
		if (Shell_NotifyIcon(NIM_ADD, &nid)) {
			break;
		}
		Sleep(1000);
	}
}


void CALLBACK SystemTrayHelper::WaitToCreateFakeWindow(HWND hwnd, UINT uMsg, UINT timerId, DWORD dwTime) {
	_createSystemTrayIcon(ins);
	KillTimer(0, timerId);
}

void SystemTrayHelper::createSystemTrayIcon(const HINSTANCE& hIns) {
	_createSystemTrayIcon(hIns);
}

void SystemTrayHelper::removeSystemTray() {
	// Clean up icon resource
	if (nid.hIcon) {
		DestroyIcon(nid.hIcon);
		nid.hIcon = NULL;
	}
	Shell_NotifyIcon(NIM_DELETE, &nid);
}