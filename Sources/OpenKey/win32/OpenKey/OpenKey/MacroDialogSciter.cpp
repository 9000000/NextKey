/*----------------------------------------------------------
NextKey - The Modern Vietnamese Input Method Engine.
Based on OpenKey architecture.

Copyright (C) 2026 NextKey Project
Author: Mai Tan Phat
License: GPL (Inherited from OpenKey)
-----------------------------------------------------------*/
#include "MacroDialogSciter.h"
#include "stdafx.h"
#include "OpenKeyHelper.h"
#include "ConfigManager.h"
#include "ConfigIntent.h"
#include <commdlg.h>
#include <dwmapi.h>
#include <CommCtrl.h>
#include <windowsx.h>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "comctl32.lib")

#include "ScaleHelper.h"

extern int vBackgroundOpacity;  // Defined in AppDelegate.cpp

// External macro functions from engine
extern void getAllMacro(std::vector<std::vector<Uint32>>& keys, std::vector<std::string>& macroText, std::vector<std::string>& macroContent);
extern bool addMacro(const std::string& macroName, const std::string& macroContent);
extern bool deleteMacro(const std::string& macroName);
extern bool hasMacro(const std::string& macroName);
extern void getMacroSaveData(std::vector<Byte>& data);
extern void readFromFile(const std::string& path, const bool& append);
extern void saveToFile(const std::string& path);
extern void initMacroMap(const Byte* pData, const int& size);
extern void initMacrosFromList(const std::vector<std::pair<std::string, std::string>>& macros);
extern std::vector<std::pair<std::string, std::string>> getAllMacrosAsList();

// Helper function to convert UTF-8 to wide string
extern std::wstring utf8ToWideString(const std::string& utf8str);
// Helper function to convert wide string to UTF-8
extern std::string wideStringToUtf8(const std::wstring& wstr);

// Redundant blur structures removed (using SciterHelper.h)

MacroDialogSciter::MacroDialogSciter() 
	: sciter::window(SW_POPUP, RECT{ 0, 0, 400, 600 }) {
	
	// 1. Initialize ConfigManager for subprocess
	ConfigManager::instance().init();

	// 2. CRITICAL: Set transparent window option BEFORE load()
	SciterSetOption(get_hwnd(), SCITER_TRANSPARENT_WINDOW, 1);
	
	auto macros = ConfigManager::instance().getMacros();
	initMacrosFromList(macros);  // Always use ConfigManager - no registry fallback
	
	// Load HTML
#ifdef NDEBUG
	// Release: load from embedded resources (packed by packfolder.exe)
	if (!load(WSTR("this://app/macro/macro.html"))) {
		MessageBoxW(NULL, L"Failed to load macro.html from resources", L"Error", MB_OK | MB_ICONERROR);
		return;
	}
#else
	// Debug: load from file (allows hot-reload during development)
	WCHAR exePath[MAX_PATH];
	GetModuleFileNameW(NULL, exePath, MAX_PATH);
	WCHAR* lastSlash = wcsrchr(exePath, L'\\');
	if (lastSlash) *lastSlash = L'\0';
	
	WCHAR htmlPath[MAX_PATH];
	swprintf_s(htmlPath, MAX_PATH, L"%s\\Resources\\Sciter\\macro\\macro.html", exePath);
	
	if (!load(htmlPath)) {
		MessageBoxW(NULL, htmlPath, L"Failed to load macro.html", MB_OK | MB_ICONERROR);
		return;
	}
#endif
	
	// Show the window
	expand();
	
	// Set window title for anti-spam detection
	// "Bảng gõ tắt" = "B\u1EA3ng g\u00F5 t\u1EAFt"
	SetWindowTextW(get_hwnd(), L"B\u1EA3ng g\u00F5 t\u1EAFt");  // Must match MACRO_WINDOW_TITLE in AppDelegate.cpp
	
	// Use scaled window size (base: 380x450 at 1920x1080)
	int scaledWidth, scaledHeight;
	ScaleHelper::getScaledSize(380, 450, scaledWidth, scaledHeight);
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
	
	// Enable blur effect using shared SciterHelper
	SciterHelper::enableWindowBlur(get_hwnd(), SciterBlurMode::BM_BLUR);
	
	// Subclass for WM_NCHITTEST (drag) and WM_CLOSE
	SetWindowSubclass(get_hwnd(), MacroDialogSciter::SubclassProc, 1, (DWORD_PTR)this);
}

MacroDialogSciter::~MacroDialogSciter() {
}

void MacroDialogSciter::show() {
	ShowWindow(get_hwnd(), SW_SHOW);
	SetForegroundWindow(get_hwnd());
}

// Redundant enableAcrylicEffect removed (using SciterHelper)

LRESULT CALLBACK MacroDialogSciter::SubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
	if (msg == WM_CLOSE) {
		// NOTE: Must use ExitProcess(0) for Sciter subprocesses!
		// PostQuitMessage(0) causes Sciter reference counting assertion failure
		ExitProcess(0);
		return 0;
	}
	
	// IPC: Bring window to foreground (sent from main process when window already exists)
	if (msg == WM_USER + 107) {
		return OpenKeyHelper::handleIPCForeground(hwnd);
	}
	
	if (msg == WM_NCHITTEST) {
		LRESULT result = SciterHelper::handleWindowDrag(hwnd, lParam, 40);
		if (result == HTCAPTION) return result;
		return DefSubclassProc(hwnd, msg, wParam, lParam);
	}
	
	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

bool MacroDialogSciter::handle_event(HELEMENT he, BEHAVIOR_EVENT_PARAMS& params) {
	// Handle DOCUMENT_READY to populate macro list
	if (params.cmd == DOCUMENT_READY) {

		fillMacroList();
		
		// Load background opacity from ConfigManager (subprocess must read from config)
		int bgOpacity = ConfigManager::instance().getInt("system", "backgroundOpacity", 80);
		
		// Apply dark/light theme based on Windows setting
		sciter::dom::element root = get_root();
		bool isDarkMode = OpenKeyHelper::isWindowsDarkMode();
		sciter::dom::element body = root.find_first("body");
		if (body) {
			if (isDarkMode) {
				body.set_attribute("class", L"dark");
			} else {
				body.remove_attribute("class");
			}
		}
		
		// Apply opacity via DOM element style
		sciter::dom::element container = root.find_first(".container");
		if (container) {
			wchar_t bgColor[64];
			double opacity = bgOpacity / 100.0;
			if (isDarkMode) {
				swprintf_s(bgColor, L"rgba(18, 20, 28, %.2f)", opacity * 0.9);
			} else {
				swprintf_s(bgColor, L"rgba(255, 255, 255, %.2f)", opacity);
			}
			container.set_style_attribute("background-color", bgColor);
		}
		
		// Fixed window size - no need to recalc
		return true;
	}
	
	// Handle VALUE_CHANGED for hidden inputs (JS sets these, C++ reads them)
	if (params.cmd == VALUE_CHANGED) {
		sciter::dom::element el(params.heTarget);
		std::wstring id = el.get_attribute("id");
		
		if (id == L"val-action") {
			sciter::dom::element root = this->root();
			sciter::value actionVal = el.get_value();
			std::wstring action = actionVal.is_string() ? actionVal.get<std::wstring>() : L"";
			

			
			if (action == L"add") {
				sciter::dom::element nameEl = root.find_first("#val-macro-name");
				sciter::dom::element contentEl = root.find_first("#val-macro-content");
				
				if (nameEl.is_valid() && contentEl.is_valid()) {
					sciter::value nameVal = nameEl.get_value();
					sciter::value contentVal = contentEl.get_value();
					std::wstring name = nameVal.is_string() ? nameVal.get<std::wstring>() : L"";
					std::wstring content = contentVal.is_string() ? contentVal.get<std::wstring>() : L"";
					
					if (!name.empty() && !content.empty()) {

						onAddMacro(name, content);
					}
				}
				// Clear action
				el.set_value(sciter::value(L""));
				return true;
			}
			
			if (action == L"delete") {
				sciter::dom::element nameEl = root.find_first("#val-macro-name");
				if (nameEl.is_valid()) {
					sciter::value nameVal = nameEl.get_value();
					std::wstring name = nameVal.is_string() ? nameVal.get<std::wstring>() : L"";
					
					if (!name.empty()) {

						onDeleteMacro(name);
					}
				}
				el.set_value(sciter::value(L""));
				return true;
			}
			
			if (action == L"import") {

				onImportMacro();
				el.set_value(sciter::value(L""));
				return true;
			}
			
			if (action == L"export") {

				onExportMacro();
				el.set_value(sciter::value(L""));
				return true;
			}
			
			if (action == L"close") {

				PostMessage(get_hwnd(), WM_CLOSE, 0, 0);
				return true;
			}
		}
	}
	
	// Handle button clicks - only for btn-close (direct click without JS)
	if (params.cmd == BUTTON_CLICK) {
		sciter::dom::element el(params.heTarget);
		std::wstring id = el.get_attribute("id");
		
		if (id == L"btn-close") {

			PostMessage(get_hwnd(), WM_CLOSE, 0, 0);
			return true;
		}
	}
	
	// Handle macro item click
	if (params.cmd == HYPERLINK_CLICK || params.cmd == BUTTON_CLICK) {
		sciter::dom::element el(params.heTarget);
		std::wstring className = el.get_attribute("class");
		
		if (className.find(L"macro-item") != std::wstring::npos) {
			// Get macro data from attributes
			std::wstring name = el.get_attribute("data-name");
			std::wstring content = el.get_attribute("data-content");
			
			// Fill input fields
			sciter::dom::element root = this->root();
			sciter::dom::element nameEl = root.find_first("#macro-name");
			sciter::dom::element contentEl = root.find_first("#macro-content");
			sciter::dom::element btnAdd = root.find_first("#btn-add");
			
			if (nameEl.is_valid()) nameEl.set_value(sciter::value(name));
			if (contentEl.is_valid()) contentEl.set_value(sciter::value(content));
			if (btnAdd.is_valid()) btnAdd.set_text(L"+ Sửa");
			
			// Update selection
			sciter::dom::element list = root.find_first("#macro-list");
			if (list.is_valid()) {
				for (int i = 0; i < (int)list.children_count(); i++) {
					sciter::dom::element item = list.child(i);
					item.set_attribute("class", L"macro-item");
				}
				el.set_attribute("class", L"macro-item selected");
			}
			return true;
		}
	}
	
	return false;
}

void MacroDialogSciter::fillMacroList() {

	
	// Get all macros from engine
	keys.clear();
	macroText.clear();
	macroContent.clear();
	getAllMacro(keys, macroText, macroContent);
	

	// Call JS function to clear list (use call_function inherited from sciter::window)
	call_function("clearMacroList");
	
	// Add items via JS (in reverse order - newest first)
	for (size_t i = 0; i < macroText.size(); i++) {
		size_t idx = macroText.size() - 1 - i;
		// Convert UTF-8 to wide string for proper Vietnamese display in Sciter
		std::wstring wName = utf8ToWideString(macroText[idx]);
		std::wstring wContent = utf8ToWideString(macroContent[idx]);
		call_function("addMacroToList", wName.c_str(), wContent.c_str());
	}
	

}

void MacroDialogSciter::saveAndReload() {
	// Central Writer: Send macros via IPC instead of saving directly
	// Main process will merge and debounce save
	auto macrosList = getAllMacrosAsList();
	
	// Serialize and send via WM_COPYDATA
	HWND mainWnd = FindWindow(APP_CLASS, NULL);
	if (mainWnd) {
		auto buffer = serializeMacros(macrosList);
		if (sendConfigIntent(mainWnd, ConfigIntentType::UPDATE_MACROS, buffer)) {
			LOG(L"[MacroDialog] Sent %zu macros via IPC\n", macrosList.size());
		} else {
			LOG(L"[MacroDialog] Failed to send macros via IPC\n");
		}
	}
	
	// Reload list (local display, main process handles persistence)
	fillMacroList();
	
	// Reset button text via JS
	call_function("updateAddButtonText");
}

void MacroDialogSciter::onAddMacro(const std::wstring& name, const std::wstring& content) {
	addMacro(wideStringToUtf8(name), wideStringToUtf8(content));
	saveAndReload();
}

void MacroDialogSciter::onDeleteMacro(const std::wstring& name) {
	std::string utf8Name = wideStringToUtf8(name);
	if (deleteMacro(utf8Name)) {
		saveAndReload();
	}
}

void MacroDialogSciter::onImportMacro() {
	OPENFILENAME ofn;
	TCHAR szFile[MAX_PATH] = { 0 };
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = get_hwnd();
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = _T("Text file (*.txt)\0*.txt\0All (*.*)\0*.*\0");
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn) == TRUE) {
		// Use Unicode escape sequences for proper display
		// "Bạn có muốn giữ lại dữ liệu hiện tại không?"
		// "Dữ liệu gõ tắt"
		int msgboxID = MessageBoxW(
			get_hwnd(),
			L"B\u1EA1n c\u00F3 mu\u1ED1n gi\u1EEF l\u1EA1i d\u1EEF li\u1EC7u hi\u1EC7n t\u1EA1i kh\u00F4ng?",
			L"D\u1EEF li\u1EC7u g\u00F5 t\u1EAFt",
			MB_ICONEXCLAMATION | MB_YESNO
		);
		std::wstring path = ofn.lpstrFile;
		readFromFile(wideStringToUtf8(path), msgboxID == IDYES);
		saveAndReload();
	}
}

void MacroDialogSciter::onExportMacro() {
	OPENFILENAME ofn;
	TCHAR szFile[MAX_PATH] = { 'O', 'p', 'e', 'n', 'K', 'e', 'y', 'M', 'a', 'c', 'r', 'o' };
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = get_hwnd();
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = _T("Text file (*.txt)\0*.txt\0");
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrDefExt = (LPCWSTR)L"txt";
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
	
	if (GetSaveFileName(&ofn) == TRUE) {
		std::wstring path = ofn.lpstrFile;
		saveToFile(wideStringToUtf8(path));
	}
}
