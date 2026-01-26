/*----------------------------------------------------------
NextKey - The Modern Vietnamese Input Method Engine.
Based on OpenKey architecture.

Copyright (C) 2026 NextKey Project
Author: Mai Tan Phat
License: GPL (Inherited from OpenKey)
-----------------------------------------------------------*/
#pragma once
#include <string>
#include <vector>
#include <Windows.h>
#include "QuickConvert.h"  // For SelectionAnchor

/**
 * Sequential Convert State Machine
 * 
 * Manages cycling through convert options one by one instead of applying all at once.
 * When user presses hotkey repeatedly, cycles through enabled options:
 *   toAllCaps -> toCapsFirstLetter -> toAllNonCaps -> toCapsEachWord -> removeMark -> origin
 * 
 * NOTE: All state is RUNTIME ONLY - not persisted to config.
 *       Only vQuickConvertSequential (ON/OFF toggle) is persisted.
 */
class SequentialConvert {
public:
    // Singleton access
    static SequentialConvert& instance();
    
    // Reset state (when selecting new text or timeout)
    void reset();
    
    // Check if sequential mode is enabled (requires vQuickConvertAutoPaste + vQuickConvertSequential)
    bool isEnabled() const;
    
    // Check if currently active (has stored origin text)
    bool isActive() const { return _currentIndex >= 0; }
    
    // Check if this is a new selection (anchor or cursor position changed)
    bool isNewSelection(const SelectionAnchor& current) const;
    
    // Content-based check for Office apps where EM_GETSEL doesn't work
    // DEPRECATED: Unreliable for converted text, use cursor position instead
    bool isNewSelectionByContent(const std::wstring& clipboardText) const;
    
    // Check timeout - returns true if 3 seconds elapsed since last activation
    bool hasTimedOut() const;
    
    // Store origin text, anchor, and window handle (called when fresh copy is done)
    void setOrigin(const std::wstring& text, const SelectionAnchor& anchor, HWND hwnd);

    // Apply current option and advance to next
    // Returns: converted text to paste
    std::wstring applyCurrentAndAdvance();

    // Get current step name for toast display (e.g., "HOA", "Thường", "Gốc")
    std::wstring getCurrentStepName() const;

    // Get the anchor that was saved with origin text (for reselect in subsequent presses)
    const SelectionAnchor& getLastAnchor() const { return _lastAnchor; }

    // Get origin text (for comparison to detect new selection)
    const std::wstring& getOriginText() const { return _originText; }
    
    // Check if window changed since last activation
    bool isWindowChanged(HWND currentHwnd) const;
    
    // Get last HWND (for debug)
    HWND getLastHwnd() const { return _lastHwnd; }
    
private:
    SequentialConvert() = default;
    
    // Build list of enabled options based on current convert tool settings
    void buildEnabledOptions();
    
    // Apply a single conversion option (0-4) to origin text
    // Returns converted text
    std::wstring applySingleOption(int optionIndex) const;
    
    // Check if text matches any of our conversions of _originText
    bool isOurConversion(const std::wstring& text) const;
    
    std::wstring _originText;           // Original selected text (RUNTIME)
    int _currentIndex = -1;             // Position in enabledOptions, -1 = idle
    int _lastAppliedOption = -1;        // Option that was last applied (for toast display)
    DWORD _lastActivation = 0;          // GetTickCount() - for timeout check
    SelectionAnchor _lastAnchor = {0, 0, false};  // For new selection detection
    HWND _lastHwnd = NULL;              // Window handle to detect app switch
    std::vector<int> _enabledOptions;   // Indices of enabled options (0-4, plus 5=origin)
    
    // Cursor position tracking for apps without anchor support
    POINT _lastCursorPos = {0, 0};    // Last known cursor position
    bool _hasCursorPos = false;         // Whether we have a valid cursor position
    
    static const DWORD TIMEOUT_MS = 3000;  // 3 seconds without press → reset to IDLE
};

// Option indices for cycle order
enum SequentialOption {
    SEQ_OPT_ALL_CAPS = 0,        // convertToolToAllCaps
    SEQ_OPT_CAPS_FIRST = 1,      // convertToolToCapsFirstLetter
    SEQ_OPT_ALL_NON_CAPS = 2,    // convertToolToAllNonCaps
    SEQ_OPT_CAPS_EACH_WORD = 3,  // convertToolToCapsEachWord
    SEQ_OPT_REMOVE_MARK = 4,     // convertToolRemoveMark
    SEQ_OPT_ORIGIN = 5           // Return to original text
};

// Global setting (ONLY this is persisted to config)
extern int vQuickConvertSequential;  // 0 = OFF, 1 = ON
