/*----------------------------------------------------------
NextKey - The Modern Vietnamese Input Method Engine.
Based on OpenKey architecture.

Copyright (C) 2026 NextKey Project
Author: Mai Tan Phat
License: GPL (Inherited from OpenKey)
-----------------------------------------------------------*/
#pragma once

// Undefine Windows macros that conflict with Sciter enums
#ifdef KEY_DOWN
#undef KEY_DOWN
#endif
#ifdef KEY_UP
#undef KEY_UP
#endif

#include "sciter-x-window.hpp"
#include <string>

class SettingsDialog : public sciter::window {
public:
	SettingsDialog();
	~SettingsDialog();

	// Show the settings dialog
	void show();

	// Override to avoid dependency on sciter::application
	HINSTANCE get_resource_instance() const { return NULL; }

	// Handle Sciter DOM events (button clicks, toggle changes, etc.)
	bool handle_event(HELEMENT he, BEHAVIOR_EVENT_PARAMS& params) override;
	
	// SOM functions - exposed to JavaScript
	void onLanguageToggle(bool isEnglish);
	void onInputTypeChange(int value);
	void onCodeTableChange(int value);
	void onSwitchKeyCtrlChange(bool enabled);
	void onSwitchKeyAltChange(bool enabled);
	void onSwitchKeyWinChange(bool enabled);
	void onSwitchKeyShiftChange(bool enabled);
	void onSwitchKeyCharChange(int charCode);
	void onBeepChange(bool enabled);
	void onSmartSwitchChange(bool enabled);
	void onOpenAdvancedSettings();
	void onExpandChange(bool isExpanded);
	void onBlurModeChange(int value);
	
	// SOM passport for JavaScript binding
	SOM_PASSPORT_BEGIN(SettingsDialog)
		SOM_FUNCS(
			SOM_FUNC(onLanguageToggle),
			SOM_FUNC(onInputTypeChange),
			SOM_FUNC(onCodeTableChange),
			SOM_FUNC(onSwitchKeyCtrlChange),
			SOM_FUNC(onSwitchKeyAltChange),
			SOM_FUNC(onSwitchKeyWinChange),
			SOM_FUNC(onSwitchKeyShiftChange),
			SOM_FUNC(onSwitchKeyCharChange),
			SOM_FUNC(onBeepChange),
			SOM_FUNC(onSmartSwitchChange),
			SOM_FUNC(onOpenAdvancedSettings),
			SOM_FUNC(onExpandChange),
			SOM_FUNC(onBlurModeChange)
		)
	SOM_PASSPORT_END

private:
	// Load settings from registry and update UI
	void loadSettings();
	
	// Save settings to registry
	void saveSettings();
	
	// Enable Windows Acrylic blur effect
	void enableAcrylicEffect();
	
	// Open advanced settings dialog
	void openAdvancedSettings();
	
	// Resize window after expand/collapse animation
	void recalcWindowSize();
	
	// Track expanded state
	bool m_isExpanded = false;
	
	// Track always-on-top (pinned) state
	bool m_isPinned = false;
	
	// SharedState version tracking for polling
	int m_lastSharedStateVersion = 0;
	
	// Subclass procedure for WM_NCHITTEST (window dragging) and WM_CLOSE
	static LRESULT CALLBACK SubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

public:
	// Diagnostic flags
	enum class BlurMode {
		None = 0,       // Solid (Mode 1)
		Aero = 1,       // Glassy (13MB RAM)
		Layered = 2     // Old Realtime (120MB RAM)
	};
	static BlurMode s_blurMode;
};
