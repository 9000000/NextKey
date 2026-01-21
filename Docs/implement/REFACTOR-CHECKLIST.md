# Refactor Checklist

> **Mục đích**: Danh sách các quy tắc quan trọng cần tuân thủ khi refactor code để tránh gây lại các bug đã fix.

---

## 1. DPI Scaling cho Sciter Dialogs

> [!IMPORTANT]
> **PHẢI** sử dụng `ScaleHelper.h` khi set kích thước window cho tất cả Sciter dialogs.

### Vấn đề
Khi Windows display scaling > 100% (125%, 150%), Sciter renders content lớn hơn nhưng window size không thay đổi → UI bị cắt.

### Pattern đúng

```cpp
#include "ScaleHelper.h"

// Trong constructor hoặc recalcWindowSize():
int scaledWidth, scaledHeight;
ScaleHelper::getScaledSize(baseWidth, baseHeight, scaledWidth, scaledHeight);
SetWindowPos(get_hwnd(), NULL, 0, 0, scaledWidth, scaledHeight, SWP_NOMOVE | SWP_NOZORDER);
```

### Các dialog phải tuân thủ

| Dialog | File |
|--------|------|
| SettingsDialog | `SettingsDialog.cpp` |
| AboutDialog | `AboutDialog.cpp` |
| ConvertToolDialogSciter | `ConvertToolDialogSciter.cpp` |
| MacroDialogSciter | `MacroDialogSciter.cpp` |
| ExcludedAppsDialogSciter | `ExcludedAppsDialogSciter.cpp` |
| SpecialAppsDialogSciter | `SpecialAppsDialogSciter.cpp` |
| ClipboardAppsDialogSciter | `ClipboardAppsDialogSciter.cpp` |

### Khi tạo dialog mới
- [ ] Include `ScaleHelper.h`
- [ ] Sử dụng `ScaleHelper::getScaledSize()` trong constructor
- [ ] Nếu có `recalcWindowSize()`, cũng phải scale trước `SetWindowPos()`

---

## 2. Sciter Subprocess Exit

> [!CAUTION]
> **PHẢI** dùng `ExitProcess(0)` cho Sciter dialog subprocesses.  
> **KHÔNG** dùng `PostQuitMessage(0)` - sẽ gây assertion failure.

Xem chi tiết trong `coderule.md`.

---

## 3. Foreground Window Patterns

Có 2 patterns đang dùng trong project, cả 2 đều hoạt động tốt:

### Pattern A: AttachThreadInput (local `forceForegroundWindow()`)

Dùng trong: `ExcludedAppsDialogSciter.cpp`, `SpecialAppsDialogSciter.cpp`, `ClipboardAppsDialogSciter.cpp`

```cpp
if (dwCurrentThread != dwForegroundThread) {
    AttachThreadInput(dwCurrentThread, dwForegroundThread, TRUE);
}
SetForegroundWindow(hwnd);
BringWindowToTop(hwnd);
SetActiveWindow(hwnd);
if (dwCurrentThread != dwForegroundThread) {
    AttachThreadInput(dwCurrentThread, dwForegroundThread, FALSE);
}
```

### Pattern B: Alt Key + TOPMOST Trick (`OpenKeyHelper::handleIPCForeground`)

Dùng cho IPC (WM_USER+107) trong tất cả dialogs:

```cpp
keybd_event(VK_MENU, 0, 0, 0);
SetForegroundWindow(hwnd);
keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
BringWindowToTop(hwnd);
```

> [!WARNING]  
> **KHÔNG** refactor các pattern này - đã được test kỹ.
