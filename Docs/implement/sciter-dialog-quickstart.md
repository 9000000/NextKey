# Sciter Dialog Quickstart Guide

> **Tài liệu này dành cho dev mới** - Hướng dẫn ngắn gọn để tạo/sửa Sciter dialog mà không bị crash hay lỗi màn đen.

## ⚠️ CẤM LÀM (Anti-Patterns)

### 1. KHÔNG dùng `PostQuitMessage()` trong subprocess
```cpp
// ❌ SAI - Gây assertion failure trong Sciter
if (msg == WM_CLOSE) {
    PostQuitMessage(0);  // CRASH!
    return 0;
}

// ✅ ĐÚNG - Dùng ExitProcess cho subprocess
if (msg == WM_CLOSE) {
    ExitProcess(0);  // OK!
    return 0;
}
```

### 2. KHÔNG dùng `SW_ALPHA` - Dùng `SciterSetOption` thay thế
```cpp
// ❌ SAI - SW_ALPHA có thể gây xung đột với một số Sciter version
: sciter::window(SW_POPUP | SW_ALPHA, ...) {
    enableAcrylicEffect();  // Local blur - khó maintain
}

// ✅ ĐÚNG - Dùng SciterSetOption + SciterHelper
: sciter::window(SW_POPUP, ...) {  // Không có SW_ALPHA
    SciterSetOption(get_hwnd(), SCITER_TRANSPARENT_WINDOW, 1);  // TRƯỚC load()
    load(...);
    expand();
    SciterHelper::enableWindowBlur(get_hwnd(), SciterBlurMode::BM_BLUR);  // SAU expand()
}
```

### 3. KHÔNG gọi `enableWindowBlur()` TRƯỚC khi load HTML
```cpp
// ❌ SAI - Gây màn đen
SciterHelper::enableWindowBlur(hwnd, ...);  // TRƯỚC load
load("this://app/...");  // Render fail!

// ✅ ĐÚNG - Blur sau khi show/size
SciterSetOption(get_hwnd(), SCITER_TRANSPARENT_WINDOW, 1);  // Trước load
load("this://app/...");
expand();
SetWindowPos(...);
SciterHelper::enableWindowBlur(hwnd, ...);  // Cuối cùng!
```

---

## ✅ Template Chuẩn (Copy & Paste)

### Constructor Pattern (Dùng SciterHelper)
```cpp
MyDialog::MyDialog()
    : sciter::window(SW_POPUP, RECT{0, 0, 400, 500}) {
    
    // 1. Load config (nếu cần)
    ConfigManager::instance().init();
    
    // 2. CRITICAL: Set transparent BEFORE load()
    SciterSetOption(get_hwnd(), SCITER_TRANSPARENT_WINDOW, 1);
    
    // 3. Load HTML
#ifdef NDEBUG
    if (!load(WSTR("this://app/mydialog/dialog.html"))) {
        MessageBoxW(NULL, L"Failed to load", L"Error", MB_OK);
        return;
    }
#else
    WCHAR exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    WCHAR* lastSlash = wcsrchr(exePath, L'\\');
    if (lastSlash) *lastSlash = L'\0';
    
    WCHAR htmlPath[MAX_PATH];
    swprintf_s(htmlPath, MAX_PATH, L"%s\\Resources\\Sciter\\mydialog\\dialog.html", exePath);
    
    if (!load(htmlPath)) {
        MessageBoxW(NULL, htmlPath, L"Failed to load", MB_OK);
        return;
    }
#endif

    // 4. Show window
    expand();
    
    // 5. Set title (cho single-instance detection)
    SetWindowTextW(get_hwnd(), L"My Dialog Title");
    
    // 6. Size & Center
    int scaledWidth, scaledHeight;
    ScaleHelper::getScaledSize(400, 500, scaledWidth, scaledHeight);
    SetWindowPos(get_hwnd(), NULL, 0, 0, scaledWidth, scaledHeight, SWP_NOMOVE | SWP_NOZORDER);
    
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    RECT rc;
    GetWindowRect(get_hwnd(), &rc);
    int x = (screenWidth - (rc.right - rc.left)) / 2;
    int y = (screenHeight - (rc.bottom - rc.top)) / 2;
    SetWindowPos(get_hwnd(), HWND_NOTOPMOST, x, y, 0, 0, SWP_NOSIZE);
    
    // 7. Enable blur (CUỐI CÙNG!) - Dùng SciterHelper
    SciterHelper::enableWindowBlur(get_hwnd(), SciterBlurMode::BM_BLUR);
    
    // 8. Subclass for drag/close
    SetWindowSubclass(get_hwnd(), MyDialog::SubclassProc, 1, (DWORD_PTR)this);
}
```

### SubclassProc Pattern (Dùng SciterHelper)
```cpp
LRESULT CALLBACK MyDialog::SubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    // WM_CLOSE: PHẢI dùng ExitProcess!
    if (msg == WM_CLOSE) {
        ExitProcess(0);
        return 0;
    }
    
    // IPC: Bring to foreground
    if (msg == WM_USER + 107) {
        return OpenKeyHelper::handleIPCForeground(hwnd);
    }
    
    // Window dragging - Dùng SciterHelper
    if (msg == WM_NCHITTEST) {
        LRESULT result = SciterHelper::handleWindowDrag(hwnd, lParam, 50);  // titleHeight = 50
        if (result == HTCAPTION) return result;
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }
    
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}
```

---

## 📋 Checklist Trước Khi Build

- [ ] Constructor dùng `SW_POPUP` (không có `SW_ALPHA`)?
- [ ] `SciterSetOption(SCITER_TRANSPARENT_WINDOW, 1)` được gọi TRƯỚC `load()`?
- [ ] `load()` được gọi TRƯỚC `expand()`?
- [ ] `SciterHelper::enableWindowBlur()` được gọi CUỐI CÙNG sau `SetWindowPos`?
- [ ] `SubclassProc` dùng `ExitProcess(0)` cho `WM_CLOSE`?
- [ ] Header có `#undef KEY_DOWN` và `#undef KEY_UP` TRƯỚC include Sciter?
- [ ] Header include `SciterHelper.h`?

---

## 🔧 Debug Tips

### Màn đen/trắng?
1. Kiểm tra thứ tự: `load()` → `expand()` → `SetWindowPos()` → `enableAcrylicEffect()`
2. Kiểm tra HTML path có đúng không (dùng MessageBox để debug)
3. Kiểm tra CSS có `background: transparent;` cho html/body không

### Assertion failure?
1. Kiểm tra `WM_CLOSE` dùng `ExitProcess(0)` chưa
2. Kiểm tra không gọi Sciter API sau khi window đã destroy

### Window không drag được?
1. Kiểm tra `WM_NCHITTEST` trong SubclassProc
2. Kiểm tra `SetWindowSubclass()` được gọi

---

## 📚 Tham Khảo

- **`AboutDialog.cpp`** - ⭐ Mẫu chuẩn nhất, dùng `SciterHelper` đầy đủ
- `MacroDialogSciter.cpp` - Mẫu dialog có list/input
- `SettingsDialog.cpp` - Phức tạp hơn, có resize logic và tab
- `SciterHelper.h/.cpp` - Shared functions cho blur và drag
- `sciter-troubleshooting.md` - Debug chi tiết
