/*----------------------------------------------------------
NextKey - The Modern Vietnamese Input Method Engine.
Based on OpenKey architecture.

Copyright (C) 2026 NextKey Project
Author: Mai Tan Phat
License: GPL (Inherited from OpenKey)

-----------------------------------------------------------*/
#pragma once
#include "sciter-x.h"
#include "sciter-x-window.hpp"
#include <string>
#include <vector>

class MacroDialogSciter : public sciter::window {
public:
	MacroDialogSciter();
	virtual ~MacroDialogSciter();

	// Event handler
	virtual bool handle_event(HELEMENT he, BEHAVIOR_EVENT_PARAMS& params) override;
	
	// Show the dialog
	void show();
	
	// Window subclass procedure for drag and close
	static LRESULT CALLBACK SubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

private:
	// Data
	std::vector<std::vector<unsigned int>> keys;
	std::vector<std::string> macroText;
	std::vector<std::string> macroContent;
	
	// UI helpers
	void fillMacroList();
	void saveAndReload();
	
	// Actions
	void onAddMacro(const std::wstring& name, const std::wstring& content);
	void onDeleteMacro(const std::wstring& name);
	void onImportMacro();
	void onExportMacro();
	
	// DWM blur effect
	void enableAcrylicEffect();
};
