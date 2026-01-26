/*----------------------------------------------------------
NextKey - The Modern Vietnamese Input Method Engine.
Based on OpenKey architecture.

Copyright (C) 2026 NextKey Project
Author: Mai Tan Phat
License: GPL (Inherited from OpenKey)
-----------------------------------------------------------*/
#pragma once
#include "stdafx.h"

// Result of QuickConvert operations
struct QuickConvertResult {
	int utf16Length;    // wstring.length() - UTF-16 code units for reselect
	bool success;       // true if conversion completed
};

// Selection anchor - captured BEFORE copy for reliable reselect
struct SelectionAnchor {
	DWORD start;    // Selection start position
	DWORD end;      // Selection end position
	bool valid;     // true = Edit control (EM_GETSEL worked), false = generic app
};

// Cutoff for reselect - skip keystroke for long text (>150 chars)
constexpr int QUICK_CONVERT_RESELECT_CUTOFF = 150;

namespace QuickConvert {
	// === Clipboard Operations ===
	QuickConvertResult convert();                            // Convert clipboard content
	void simulateCopy();                                     // Simulate Ctrl+C
	void simulatePaste();                                    // Simulate Ctrl+V
	bool waitForCopy(int delayMs = 50);                      // Wait for copy to complete
	
	// Clipboard text helpers (DRY - used by both normal and sequential modes)
	std::wstring readClipboardText();
	void writeClipboardText(const std::wstring& text);
	
	// Wait for user to release all modifier keys (Ctrl, Shift, Alt, Win)
	bool waitForModifiersRelease(int maxWaitMs = 500);
	
	// === Selection/Reselect ===
	// Get anchor BEFORE copy - returns valid=true for Edit controls
	SelectionAnchor getSelectionAnchor(HWND hwnd);
	
	// Anchor-based reselect with auto-detection:
	// - Tier 1 (Edit controls): EM_SETSEL using pastedLength (exact)
	// - Tier 2 (Generic apps): SendInput using originalSelLength (workaround for Unicode normalization)
	// Returns: true = success/attempted, false = failed
	bool tryReselect(HWND hwnd, SelectionAnchor anchor, int pastedLength, int originalSelLength);
	
	// === Smart Timing Detection ===
	// Wait for clipboard to have Unicode text (reduce fixed delays)
	bool waitForClipboardUnicode(int maxWaitMs = 200, int checkIntervalMs = 10);
	
	// Check if a window is likely an Office app (needs longer delays)
	bool isOfficeApp(HWND hwnd);
	
	// Wait for window to regain focus after paste (reduce paste delays)
	bool waitForWindowFocus(HWND targetHwnd, int maxWaitMs = 150, int checkIntervalMs = 10);
	
	// === UI ===
	void showToast(LPCWSTR message);
}
