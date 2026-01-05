# Task 11: Fix UI Height Auto-Expand Issue

## Problem
Adding new options to tabs causes UI overflow. Changing hardcoded height in C++ doesn't seem to take effect immediately.

## Current Behavior
- Tab "Hệ thống" has 9+ toggles → overflow at bottom
- Changed `RECT{0, 0, 350, 460}` → `RECT{0, 0, 350, 490}` but UI still clips

## Workaround Applied
Moved "Tự động kiểm tra cập nhật" to "Thông tin" tab to avoid overflow.

---

## Investigation Areas

### 1. Initial Window Size (SettingsDialog.cpp:38)
```cpp
: sciter::window(SW_POPUP | SW_ALPHA | SW_ENABLE_DEBUG, RECT{0, 0, 350, 460})
```
**Question:** Is this initial size overwritten later?

### 2. Auto-fit Code (SettingsDialog.cpp:112-127)
```cpp
RECT contentRect = container.get_location(CONTENT_BOX);
contentWidth = max(contentWidth, 350);
contentHeight = max(contentHeight, 200);
SetWindowPos(get_hwnd(), NULL, 0, 0, contentWidth, contentHeight, SWP_NOMOVE | SWP_NOZORDER);
```
**Question:** Does `container.get_location()` return correct height? Maybe CSS is constraining it.

### 3. Container CSS (settings.css:24-37)
```css
.container {
    width: 350px;
    height: auto;
    overflow: hidden;
}
```
**Question:** Is `overflow: hidden` clipping content before auto-fit measures it?

### 4. Tab Body CSS (settings.css:677-690)
```css
.tab-body {
    height: auto;
    overflow-y: auto;
}
```
**Question:** Is `height: auto` working correctly in Sciter?

### 5. recalcWindowSize() (SettingsDialog.cpp:1399-1421)
```cpp
RECT contentRect = container.get_location(CONTENT_BOX);
newHeight = max(contentRect.bottom - contentRect.top, 200);
SetWindowPos(get_hwnd(), NULL, x, y, newWidth, newHeight, SWP_NOZORDER);
```
**Question:** Is this being called at the right time? After content is fully rendered?

---

## Debugging Steps

### Step 1: Add Debug Logging
```cpp
// In auto-fit section (line 117)
char log[256];
sprintf_s(log, "Auto-fit: contentRect = %d x %d", contentWidth, contentHeight);
OutputDebugString(log);
```
Check if `contentRect` reports correct size.

### Step 2: Force Height via CSS
```css
#tab-panel-3 .tab-body {
    min-height: 400px !important;
}
```
See if CSS min-height affects the measured size.

### Step 3: Check Container Overflow
Try changing:
```css
.container {
    overflow: visible;  /* Instead of hidden */
}
```

### Step 4: Delay Measurement
```cpp
// Maybe content not fully rendered yet
SetTimer(get_hwnd(), TIMER_MEASURE, 100, NULL);
// Then measure in WM_TIMER handler
```

---

## Potential Root Causes

| Cause | Likelihood | Fix |
|-------|------------|-----|
| CSS `overflow: hidden` clips before measure | High | Change to `overflow: visible` |
| Sciter measures before DOM fully rendered | Medium | Add delay before SetWindowPos |
| Tab content not included in CONTENT_BOX | Medium | Measure active tab separately |
| Hardcoded min-height overrides content | Low | Find and remove it |

---

## Action Items
1. [ ] Add debug logging to see actual contentRect values
2. [ ] Test CSS changes to container overflow
3. [ ] Test delayed measurement
4. [ ] Consider using fixed tab heights based on content

## Effort
~1-2 hours for proper investigation and fix
