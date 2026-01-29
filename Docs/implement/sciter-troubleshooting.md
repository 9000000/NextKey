# Sciter Troubleshooting Guide

> **Purpose**: Common issues, their fixes, and how to enable Sciter Inspector for debugging.

---

## Sciter Inspector

Inspector is the essential tool for debugging Sciter UI. It allows live DOM inspection, CSS editing, and JavaScript console.

### Enable Inspector

In `main.cpp` before creating dialog:

```cpp
// Enable debug mode for Inspector connection
SciterSetOption(NULL, SCITER_SET_DEBUG_MODE, TRUE);

// Enable required runtime features (socket I/O is required for Inspector)
SciterSetOption(NULL, SCITER_SET_SCRIPT_RUNTIME_FEATURES,
    ALLOW_FILE_IO |
    ALLOW_SOCKET_IO |    // REQUIRED for Inspector
    ALLOW_EVAL |
    ALLOW_SYSINFO);
```

OR add `SW_ENABLE_DEBUG` flag in window creation:
```cpp
: sciter::window(SW_POPUP | SW_ALPHA | SW_ENABLE_DEBUG, RECT{0, 0, 380, 450})
```

### Connect Inspector

1. Run `inspector.exe` from Sciter SDK folder first
2. Open your dialog
3. Press **Ctrl+Shift+I** in the dialog window
4. Inspector will show: DOM tree, CSS styles, Console

> [!TIP]
> **Ctrl+Shift+Click** on any element to select it directly in Inspector.

### Inspector Not Connecting?

| Issue | Solution |
|-------|----------|
| Nothing happens on Ctrl+Shift+I | Add `ALLOW_SOCKET_IO` in `SciterSetOption` |
| Port conflict | Check if another Inspector is already running |
| Wrong SDK version | Use Inspector from the same SDK as sciter.dll |

---

## Common Issues & Fixes

### Layout Issues

| Issue | Cause | Fix |
|-------|-------|-----|
| Content overflows window | No height limit | Set fixed `height` + `overflow: hidden` |
| Footer buttons cut off | List too tall | Reduce list `height`, calculate height budget |
| Background overflows | Container too large | Match container height to window height |
| Scrollbar appears | Content overflow | Add `overflow: hidden` to containers |
| Extra space at bottom | Window too large | Use auto-fit or correct fixed height |
| UI blank/ghost on resize | Sciter layout is async | Use **Ghost Reflow** in JS: `el.box("dimension")` or `container.offsetHeight` before notifying C++ to resize |

### Element Issues

| Issue | Cause | Fix |
|-------|-------|-----|
| Toggle not clickable | `<label>` wrapping | Use `<div>` not `<label>` |
| Toggle ::before/::after not working | Sciter limitation | Use div-based toggles |
| Button click not working | SVG captures click | Add `pointer-events: none` to SVG |
| Close button not responding | Wrong event handler | Handle in C++ `BUTTON_CLICK` |
| Hidden input not triggering | No change event | Call `dispatchEvent(new Event("change", { bubbles: true }))` |

### Styling Issues

| Issue | Cause | Fix |
|-------|-------|-----|
| Gray corners on blur | border-radius too large | Use `8px` max |
| Blur not visible | CSS background too opaque | Use `rgba(255,255,255,0.65)` |
| Jagged borders | Sciter anti-aliasing | Use `box-shadow: inset` instead of `border` |
| Text jumping in input | Sciter's std-edit uses `height: 1.4em` | Use `!important` on `height`, `line-height: height-2px`, `overflow: hidden` |
| Layout shift on toggle | Inline-block whitespace | Use `font-size: 0` on container |
| **Toggle thumb color blending** | `position: absolute` with background inherits/blends colors from parent | Use `display: inline-block` + `margin` instead of `position: absolute` + `left` |

### C++ / Integration Issues

| Issue | Cause | Fix |
|-------|-------|-----|
| Assertion on delete | Sciter cleanup issue | Use subprocess pattern, call `ExitProcess(0)` |
| Multiple dialogs open | FindWindow timing issue | Use Named Mutex + FindWindow fallback |
| find_first returns null | Wrong string type | Use `char*` not `wchar_t*`: `find_first("#id")` |
| Settings not saving | Registry not synced | Call `notifyMainProcess()` with WM_USER+101 |
| Main process not updating | Not reloading registry | Add `APP_GET_DATA` in WM_USER+101 handler |
| Heap corruption | getRegBinary memory issue | Don't `delete[]` returned pointer (it's static) |
| Vietnamese text garbled | Source file encoding | Use Unicode escape sequences `\uXXXX` |
| Toggles show default values | Subprocess has empty globals | Add `APP_GET_DATA` in constructor for all settings |
| Function does not take 0 args | Wrong function signature | Check `.h` file: `registerRunOnStartup(int)` takes 1=on, 0=off |
| Function is not a member | Function doesn't exist | Check `.h` file before calling. Some features need `notifyMainProcess()` |

### Window Issues

| Issue | Cause | Fix |
|-------|-------|-----|
| Always on top | SW_POPUP default | Use `HWND_NOTOPMOST` in SetWindowPos |
| Can't drag window | No hit-test handler | Implement `WM_NCHITTEST` in SubclassProc |
| Drag blocked by close button | Drag zone too large | Exclude close button area in `WM_NCHITTEST` |
| Window doesn't close | No WM_CLOSE handler | Handle in SubclassProc with `ExitProcess(0)` |

---

## Debugging with DebugView

Use `OutputDebugStringW` for logging, view with [DebugView](https://learn.microsoft.com/en-us/sysinternals/downloads/debugview):

```cpp
wchar_t msg[256];
swprintf_s(msg, L"Dialog: value = %d\n", someValue);
OutputDebugStringW(msg);
```

**DebugView setup:**
1. Run as Administrator
2. Enable **Capture → Capture Win32** (Ctrl+W)
3. Filter: add `OpenKey:*` or your prefix

---

## Height Budget Calculation

For fixed layouts, calculate all heights explicitly:

```
Container Height = Title + Content + Padding + Margins

Example (450px window):
- Title bar:        36px
- Content padding:  32px (16px × 2)
- Input card:      120px
- List header:      30px
- List area:       150px
- Footer buttons:   40px
- Margins:          24px
-----------------------
  Total:           432px ✅ (fits in 450px)
```

> [!TIP]
> Leave 10-20px safety margin. If footer is cut off, reduce list height.

---

## Sciter CSS Differences

Properties unique to Sciter:

| Property | Use |
|----------|-----|
| `font-rendering-mode: snap-pixel` | Fix text jumping in edit fields |
| `overflow-y: scroll-indicator` | Mobile-style scrollbar (may not work in all versions) |
| `flow: horizontal` | Horizontal layout (alternative to flexbox) |
| `size: 100px 200px` | Shorthand for width + height |

---

## Quick Reference

### DO ✅

| Task | Code |
|------|------|
| Find element | `root.find_first("#id")` (char*, not wchar_t*) |
| Get value | `el.get_value().get<int>()` |
| Set value | `el.set_value(sciter::value(val))` |
| Set class | `el.set_attribute("class", L"myclass")` |
| Get size | `el.get_location(CONTENT_BOX)` |
| Call JS function | `call_function("funcName", arg1, arg2)` |
| Exit dialog | `ExitProcess(0)` in WM_CLOSE |

### DON'T ❌

| Don't | Why | Instead |
|-------|-----|---------|
| `delete sciterWindow` | Assertion error | Use subprocess |
| `find_first(L"#id")` | Wrong type | Use `char*` |
| `<label>` around toggles | Blocks clicks | Use `<div>` |
| `<input type="checkbox">` | ::before broken | Use div toggles |
| `delete[] getRegBinary()` | Static pointer | Don't delete |
| `height: 100%` on container | Breaks layout | Use fixed height |

---

## ⚠️ Settings Dialog Layout - DO NOT CHANGE

> **CRITICAL**: These values have been carefully tuned. Changing them WILL break UI.

### CSS (`settings.css`) - Frozen Values

| Property | Value | Reason |
|----------|-------|--------|
| `.container { font-size: 0; }` | `0` | Eliminates inline-block whitespace |
| `.container.expanded { width: 750px; }` | `750px` | Must equal 350px + 400px exactly |
| `.container.expanded { overflow: hidden; }` | Required | Clears float layout |
| `.compact-section { float: left; width: 350px; }` | Exact values | Side-by-side layout |
| `.advanced-section { float: left; width: 400px; display: block; }` | Exact values | `display: block` overrides `display: none` |

### CSS - DO NOT USE

| ❌ Don't Use | Why | ✅ Use Instead |
|-------------|-----|---------------|
| `flow: horizontal` | Sciter-specific, breaks content |`float: left` |
| `height: *` (flex units) | Sciter-specific, may hide content | Fixed height or `height: auto` |
| `display: inline-block` | Whitespace issues, wrap problems | `float: left` |
| `height: 100%` | Breaks layout calculation | `height: auto` |
| `window-blurbehind` HTML attr | Requires specific CSS setup | DWM API in C++ |

### C++ (`SettingsDialog.cpp`) - Frozen Logic

| Code Section | Reason |
|--------------|--------|
| `recalcWindowSize()` uses `MARGIN_BOX` | Includes padding in measurement |
| Width constants: `350 * dpiScale`, `400 * dpiScale` | Must match CSS exactly |
| `enableAcrylicEffect()` uses `SetWindowCompositionAttribute` | Native Sciter blur doesn't work with current CSS |
| Constructor uses `SciterSetOption(SCITER_TRANSPARENT_WINDOW, 1)` | Required for blur effect |

### C++ - DO NOT USE

| ❌ Don't Use | Why |
|-------------|-----|
| `CONTENT_BOX` for height measurement | Excludes padding |
| `SAFETY_PADDING` addition | CSS already has `padding-bottom` |
| `window-blurbehind` with current CSS | Body is `background: transparent`, causes invisible UI |

### BlurMode Enum (Simplified)

```cpp
enum class BlurMode {
    None = 0,   // Solid - no blur, no transparency
    Glass = 1   // Native DWM blur (Mica on W11, Acrylic on W10)
};
// ❌ REMOVED: Layered = 2 (used 120MB RAM for same visual effect)
```

---

## Sample Debug Logging

Add to `handle_event` to see all events:

```cpp
bool DialogName::handle_event(HELEMENT he, BEHAVIOR_EVENT_PARAMS& params) {
    sciter::dom::element el(params.heTarget);
    std::wstring id = el.get_attribute("id") ? el.get_attribute("id") : L"";
    std::wstring cls = el.get_attribute("class") ? el.get_attribute("class") : L"";
    
    wchar_t msg[512];
    swprintf_s(msg, L"Event: cmd=%d target=[%s#%s.%s]\n",
        params.cmd,
        el.get_element_type(),
        id.c_str(),
        cls.c_str());
    OutputDebugStringW(msg);
    
    // Your event handling...
    return false;
}
```

**Common event codes:**
- `DOCUMENT_READY` (193) - HTML loaded
- `BUTTON_CLICK` (0) - Button clicked  
- `VALUE_CHANGED` (2) - Input value changed
- `HYPERLINK_CLICK` (128) - Link clicked
