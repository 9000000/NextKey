# Task 7: Refactor Duplicate Hotkey Logic

## Status
**⏸️ Deferred** - Watch for bugs

## Decision Notes
**Analyzed:** 2026-01-05

**Reason to skip:**
- Hot path (keyboard hook) - function call overhead not ideal
- English/Vietnamese modes have subtle differences (`_lastFlag`, `vTempOffSpelling`)
- Current code is readable, duplication is minor (~30 lines)
- Low ROI: 20-45 min effort for marginal benefit

**When to revisit:**
- If adding new hotkeys (3rd, 4th hotkey) → refactor becomes valuable
- If bug found in one location → check other location too
- If mode-specific logic becomes more complex

**Duplicate locations:**
| Type | English Mode | Vietnamese Mode |
|------|--------------|-----------------|
| Keydown | Lines 684-698 | Lines 772-783 |
| Flag release | Lines 705-712 | Lines 791-798 |

## Goal
Extract common hotkey handling into a shared function to reduce code duplication.

## Problem
`keyboardHookProcess()` in `OpenKey.cpp` has duplicated hotkey handling code:
- Lines 670-705: English mode path
- Lines 755-798: Vietnamese mode path

Both paths check the same hotkeys (switch language, convert tool) with nearly identical logic.

## Solution
Extract into `handleGlobalHotkeys()`:

```cpp
// Returns: true if a hotkey was handled (should block key), false otherwise
static bool handleGlobalHotkeys(WPARAM wParam) {
    if (!((wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) && !_isFlagKey && _keycode != 0)) {
        return false;
    }
    
    // Check switch language hotkey
    if (GET_SWITCH_KEY(vSwitchKeyStatus) == _keycode && 
        checkHotKey(vSwitchKeyStatus, GET_SWITCH_KEY(vSwitchKeyStatus) != 0xFE)) {
        switchLanguage();
        _hasJustUsedHotKey = true;
        _keycode = 0;
        return true;
    }
    
    // Check convert tool hotkey
    if (GET_SWITCH_KEY(convertToolHotKey) == _keycode && 
        checkHotKey(convertToolHotKey, GET_SWITCH_KEY(convertToolHotKey) != 0xFE)) {
        AppDelegate::getInstance()->onQuickConvert();
        _hasJustUsedHotKey = true;
        _keycode = 0;
        return true;
    }
    
    return false;
}
```

## Usage After Refactor
```cpp
// English mode early exit (line ~668)
if (vLanguage == 0) {
    if (handleGlobalHotkeys(wParam)) {
        return -1;  // Block key
    }
    // ... rest of English mode handling
}

// Vietnamese mode (line ~755)
if (handleGlobalHotkeys(wParam)) {
    return -1;
}
```

## Effort
~30 minutes
