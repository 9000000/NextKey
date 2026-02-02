/*----------------------------------------------------------
NextKey - The Modern Vietnamese Input Method Engine.
Based on OpenKey architecture.

Copyright (C) 2026 NextKey Project
Author: Mai Tan Phat
License: GPL (Inherited from OpenKey)
-----------------------------------------------------------*/
#include "stdafx.h"
#include "SequentialConvert.h"
#include "AppDelegate.h"  // For vQuickConvertAutoPaste, vQuickConvertSequential
#include "PerformanceLogger.h"  // For DEBUG_LOG
#include "../../../engine/ConvertTool.h"
#include "../../../engine/Engine.h"
#include <functional>  // For std::hash

// Global setting definition
int vQuickConvertSequential = 0;

SequentialConvert& SequentialConvert::instance() {
    static SequentialConvert inst;
    return inst;
}

void SequentialConvert::reset() {
    DEBUG_LOG("QC_SEQ", "reset() called - clearing state");
    _originText.clear();
    _currentIndex = -1;
    _lastAppliedOption = -1;
    _lastActivation = 0;
    _lastAnchor = {0, 0, false};
    _lastHwnd = NULL;
    _enabledOptions.clear();
    _hasCursorPos = false;
    _originHash = 0;
    _conversionHashes.clear();
}

bool SequentialConvert::isEnabled() const {
    // Sequential mode requires both: auto-paste ON + sequential toggle ON
    bool enabled = (vQuickConvertAutoPaste != 0) && (vQuickConvertSequential != 0);
    DEBUG_LOG_FMT("QC_SEQ", "isEnabled: autoPaste=%d, sequential=%d, result=%d",
        vQuickConvertAutoPaste, vQuickConvertSequential, enabled);
    return enabled;
}

bool SequentialConvert::isNewSelection(const SelectionAnchor& current) const {
    // If both anchors are valid, compare by position (preferred - more reliable)
    if (_lastAnchor.valid && current.valid) {
        // Only compare START position (not end) because text length changes after conversion
        return (current.start != _lastAnchor.start);
    }
    
    // If either anchor is invalid (Office apps), check cursor position
    POINT currentCursorPos;
    if (GetCaretPos(&currentCursorPos)) {
        // Convert to screen coordinates for comparison
        ClientToScreen(GetForegroundWindow(), &currentCursorPos);
        
        if (_hasCursorPos) {
            // Compare cursor position - if moved > 5 pixels, consider new selection
            int deltaX = abs(currentCursorPos.x - _lastCursorPos.x);
            int deltaY = abs(currentCursorPos.y - _lastCursorPos.y);
            return (deltaX > 5) || (deltaY > 5);
        }
    }
    
    // No reliable way to detect new selection - assume same selection
    return false;
}

// Content-based comparison for apps where EM_GETSEL doesn't work (Office, etc.)
// Uses hash comparison: if clipboard matches origin OR any conversion variant → same selection
bool SequentialConvert::isNewSelectionByContent(const std::wstring& clipboardText) const {
    if (_originText.empty() || clipboardText.empty()) {
        DEBUG_LOG("QC_DETECT", "isNewSelectionByContent: empty text → new selection");
        return true;
    }
    
    std::size_t currentHash = std::hash<std::wstring>{}(clipboardText);
    
    // Same as origin?
    if (currentHash == _originHash) {
        DEBUG_LOG("QC_DETECT", "isNewSelectionByContent: matches origin → same selection");
        return false;
    }
    
    // Same as any of our conversions?
    for (std::size_t h : _conversionHashes) {
        if (currentHash == h) {
            DEBUG_LOG("QC_DETECT", "isNewSelectionByContent: matches conversion variant → same selection");
            return false;
        }
    }
    
    // Different content → new selection
    DEBUG_LOG("QC_DETECT", "isNewSelectionByContent: no match → new selection");
    return true;
}

bool SequentialConvert::isWindowChanged(HWND currentHwnd) const {
    if (!isActive()) return false;
    return _lastHwnd != currentHwnd;
}

bool SequentialConvert::isOurConversion(const std::wstring& text) const {
    if (_originText.empty()) return false;
    
    // Check if text matches any enabled conversion option
    for (int opt : _enabledOptions) {
        std::wstring converted = applySingleOption(opt);
        if (text == converted) {
            return true;
        }
    }
    return false;
}

bool SequentialConvert::hasTimedOut() const {
    if (_currentIndex < 0) {
        return false;  // Not active, no timeout
    }
    DWORD now = GetTickCount();
    return (now - _lastActivation) > TIMEOUT_MS;
}

void SequentialConvert::buildEnabledOptions() {
    _enabledOptions.clear();
    
    DEBUG_LOG_FMT("QC_SEQ", "buildEnabledOptions: toAllCaps=%d, toCapsFirst=%d, toAllNonCaps=%d, toCapsEach=%d, removeMark=%d",
        convertToolToAllCaps, convertToolToCapsFirstLetter, convertToolToAllNonCaps, 
        convertToolToCapsEachWord, convertToolRemoveMark);
    
    // Add options in fixed order, skipping disabled ones
    if (convertToolToAllCaps) {
        _enabledOptions.push_back(SEQ_OPT_ALL_CAPS);
    }
    if (convertToolToCapsFirstLetter) {
        _enabledOptions.push_back(SEQ_OPT_CAPS_FIRST);
    }
    if (convertToolToAllNonCaps) {
        _enabledOptions.push_back(SEQ_OPT_ALL_NON_CAPS);
    }
    if (convertToolToCapsEachWord) {
        _enabledOptions.push_back(SEQ_OPT_CAPS_EACH_WORD);
    }
    if (convertToolRemoveMark) {
        _enabledOptions.push_back(SEQ_OPT_REMOVE_MARK);
    }
    
    // Always add origin as last option (wrap back to original)
    _enabledOptions.push_back(SEQ_OPT_ORIGIN);
    
    DEBUG_LOG_FMT("QC_SEQ", "buildEnabledOptions: totalOptions=%d", (int)_enabledOptions.size());
}

std::wstring SequentialConvert::applySingleOption(int optionIndex) const {
    if (_originText.empty()) {
        return L"";
    }
    
    // For origin, just return the original text
    if (optionIndex == SEQ_OPT_ORIGIN) {
        return _originText;
    }
    
    // Temporarily set ONLY the requested option, clear others
    bool savedAllCaps = convertToolToAllCaps;
    bool savedNonCaps = convertToolToAllNonCaps;
    bool savedCapsFirst = convertToolToCapsFirstLetter;
    bool savedCapsEach = convertToolToCapsEachWord;
    bool savedRemoveMark = convertToolRemoveMark;
    
    // Clear all options and set ONLY the requested one (Branchless)
    convertToolToAllCaps       = (optionIndex == SEQ_OPT_ALL_CAPS);
    convertToolToCapsFirstLetter = (optionIndex == SEQ_OPT_CAPS_FIRST);
    convertToolToAllNonCaps    = (optionIndex == SEQ_OPT_ALL_NON_CAPS);
    convertToolToCapsEachWord  = (optionIndex == SEQ_OPT_CAPS_EACH_WORD);
    convertToolRemoveMark      = (optionIndex == SEQ_OPT_REMOVE_MARK);
    
    // Convert using engine
    std::string utf8Origin = wideStringToUtf8(_originText);
    std::string utf8Result = convertUtil(utf8Origin);
    std::wstring result = utf8ToWideString(utf8Result);
    
    // Restore original settings
    convertToolToAllCaps = savedAllCaps;
    convertToolToAllNonCaps = savedNonCaps;
    convertToolToCapsFirstLetter = savedCapsFirst;
    convertToolToCapsEachWord = savedCapsEach;
    convertToolRemoveMark = savedRemoveMark;
    
    return result;
}

void SequentialConvert::setOrigin(const std::wstring& text, const SelectionAnchor& anchor, HWND hwnd) {
    _originText = text;
    _lastAnchor = anchor;
    _lastHwnd = hwnd;
    _currentIndex = 0;  // Start at first option
    _lastActivation = GetTickCount();
    buildEnabledOptions();
    
    // Compute hash of origin text for content-based detection
    _originHash = std::hash<std::wstring>{}(text);
    
    // Pre-compute hashes of all possible conversion variants
    // This allows us to detect if user's copied text matches any conversion
    _conversionHashes.clear();
    _conversionHashes.reserve(_enabledOptions.size());
    for (int opt : _enabledOptions) {
        std::wstring converted = applySingleOption(opt);
        _conversionHashes.push_back(std::hash<std::wstring>{}(converted));
    }
    
    DEBUG_LOG_FMT("QC_SEQ", "setOrigin: text='%.20ls...', len=%d, options=%d, hash=%zu",
        text.c_str(), (int)text.length(), (int)_enabledOptions.size(), _originHash);
    
    // Save current cursor position for future comparison
    if (GetCaretPos(&_lastCursorPos)) {
        ClientToScreen(hwnd, &_lastCursorPos);
        _hasCursorPos = true;
    } else {
        _hasCursorPos = false;
    }
}

std::wstring SequentialConvert::applyCurrentAndAdvance() {
    // Apply current option
    if (_enabledOptions.empty()) {
        _lastAppliedOption = SEQ_OPT_ORIGIN;
        return _originText;  // No options enabled, return origin
    }
    
    // Make sure currentIndex is valid
    if (_currentIndex < 0) {
        _currentIndex = 0;
    }
    if (_currentIndex >= (int)_enabledOptions.size()) {
        _currentIndex = 0;
    }
    
    int optionToApply = _enabledOptions[_currentIndex];
    _lastAppliedOption = optionToApply;  // Store for getCurrentStepName()
    std::wstring result = applySingleOption(optionToApply);
    
    int prevIndex = _currentIndex;
    
    // Update state
    _lastActivation = GetTickCount();
    
    // Advance to next option for next call
    _currentIndex++;
    if (_currentIndex >= (int)_enabledOptions.size()) {
        _currentIndex = 0;  // Wrap around
    }
    
    DEBUG_LOG_FMT("QC_SEQ", "applyCurrentAndAdvance: appliedOpt=%d, prevIdx=%d, nextIdx=%d, totalOpts=%d",
        optionToApply, prevIndex, _currentIndex, (int)_enabledOptions.size());
    
    return result;
}

std::wstring SequentialConvert::getCurrentStepName() const {
    // Use lastAppliedOption which was set in applyCurrentAndAdvance()
    switch (_lastAppliedOption) {
        case SEQ_OPT_ALL_CAPS:
            return L"HOA"; // Không dấu, giữ nguyên OK
        case SEQ_OPT_CAPS_FIRST:
            // "Hoa đầu câu" -> Chữ đ(\u0111), ầ(\u1EA7), u, c, â(\u00E2), u
            return L"Hoa \u0111\u1EA7u c\u00E2u"; 
        case SEQ_OPT_ALL_NON_CAPS:
            // "thường" -> ư(\u01B0), ờ(\u1EDD)
            return L"th\u01B0\u1EDDng";
        case SEQ_OPT_CAPS_EACH_WORD:
            // "Hoa Mỗi Từ" -> ỗ(\u1ED7), ừ(\u1EEB)
            return L"Hoa M\u1ED7i T\u1EEB";
        case SEQ_OPT_REMOVE_MARK:
            // "Bỏ dấu" -> ấ(\u1EA5)
            return L"B\u1ECF d\u1EA5u";
        case SEQ_OPT_ORIGIN:
            // "Gốc" -> ố(\u1ED1)
            return L"G\u1ED1c";
        default:
            return L"";
    }
}
