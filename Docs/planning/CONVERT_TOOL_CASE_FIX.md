# Convert Tool Case Preservation Bug Fix

**Date:** 2026-01-08
**Status:** Pending Fix

## Problem

When converting Unicode → Unicode with **NO options enabled**, the quick convert hotkey incorrectly lowercases text:

- Dialog button: "Xin chào" → "Xin chào" ✅
- Quick convert: "Xin chào" → "xin chào" ❌

## Root Cause

In `engine/ConvertTool.cpp`, the condition `!shouldUpperCase` in the lowercase branch always evaluates to `true` when no options are enabled, causing all uppercase chars to be converted to lowercase.

### Buggy Code (3 locations):

**Lines 93 & 136 (Vietnamese chars):**
```cpp
else if ((convertToolToAllNonCaps || !shouldUpperCase) && k % 2 == 0) {
    target = _codeTable[convertToolToCode][j][k+1];  // Force lowercase
}
```

**Line 159 (Normal chars):**
```cpp
else if (convertToolToAllNonCaps || !shouldUpperCase)
    _temp.push_back(towlower(data[i]));  // Force lowercase
```

### Why Bug Happens:
- `shouldUpperCase` defaults to `false`
- `!shouldUpperCase` = `!false` = `true`
- Condition becomes `(false || true)` = `true` → always lowercase

## Fix

Remove `|| !shouldUpperCase` from all 3 locations:

**Lines 93 & 136:**
```diff
-else if ((convertToolToAllNonCaps || !shouldUpperCase) && k % 2 == 0) {
+else if (convertToolToAllNonCaps && k % 2 == 0) {
```

**Line 159:**
```diff
-else if (convertToolToAllNonCaps || !shouldUpperCase)
+else if (convertToolToAllNonCaps)
```

## Rollback Instructions

If this fix causes issues, revert by adding back `|| !shouldUpperCase`:

```cpp
// Line 93:
else if ((convertToolToAllNonCaps || !shouldUpperCase) && k % 2 == 0)

// Line 136:
else if ((convertToolToAllNonCaps || !shouldUpperCase) && k % 2 == 0)

// Line 159:
else if (convertToolToAllNonCaps || !shouldUpperCase)
```

## Test Cases After Fix

| Input | Options | Expected Output |
|-------|---------|-----------------|
| "Xin chào" | None | "Xin chào" |
| "xin chào" | AllCaps | "XIN CHÀO" |
| "Xin Chào" | AllNonCaps | "xin chào" |
| "xin chào. xin chào" | CapsFirst | "Xin chào. Xin chào" |
| "xin chào xin chào" | CapsEachWord | "Xin Chào Xin Chào" |
