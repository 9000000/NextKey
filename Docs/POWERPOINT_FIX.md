# PowerPoint Vietnamese Input Fix

## Problem

Vietnamese input không hoạt động trong Microsoft PowerPoint khi sử dụng OpenKey. User gõ tiếng Việt nhưng chỉ hiện ký tự gốc (ví dụ: "gox" thay vì "gõ").

## Root Causes

### 1. IME Check Block (Primary)

PowerPoint trả về `isImeON=1` thông qua `ImmGetDefaultIMEWnd()` + `WM_IME_CONTROL`, dù không có IME thật sự được kích hoạt. Điều này khiến OpenKey skip toàn bộ Vietnamese processing.

```cpp
// Before: Skip all processing if any IME is detected
if (isImeON) {
    return CallNextHookEx(...);
}
```

### 2. dwExtraInfo Issue (Secondary)

OpenKey sử dụng `dwExtraInfo = 1` để đánh dấu events do chính nó tạo ra (tránh hook re-entry). Tuy nhiên, hook check `!= 0` quá broad, có thể gây conflict với các apps khác.

## Solution

### 1. Skip IME Check for MS Office Apps

Thêm danh sách apps cần skip IME check:

```cpp
static vector<string> _skipImeCheckApps = {
    "POWERPNT.EXE", "powerpnt.exe",
    "WINWORD.EXE", "winword.exe",
    "EXCEL.EXE", "excel.exe"
};

static bool shouldSkipImeCheck() {
    string& appName = OpenKeyHelper::getLastAppExecuteName();
    return std::find(_skipImeCheckApps.begin(), _skipImeCheckApps.end(), appName) 
           != _skipImeCheckApps.end();
}
```

### 2. Magic Number for Event Identification

Sử dụng magic number thay vì hardcode value `1`:

```cpp
#define OPENKEY_EXTRA_INFO 0x4F4B  // 'OK' in ASCII

// Hook check giờ chỉ skip events của OpenKey
if (keyboardData->dwExtraInfo == OPENKEY_EXTRA_INFO) {
    return CallNextHookEx(...);
}
```

## Technical Notes

### Why Magic Number?

- **Industry standard**: UniKey, EVKey, GoTiengViet đều dùng approach tương tự
- **Microsoft recommended**: `dwExtraInfo` được thiết kế để identify injected events
- **Collision-resistant**: `0x4F4B` đủ unique để không trùng với các apps khác

### Why Skip IME Check for Office?

- MS Office apps report `isImeON=1` dù không có East Asian IME active
- Có thể do Office tự implement IME interface cho các tính năng như autocomplete
- Workaround này an toàn vì Vietnamese input không conflict với Office's internal IME

## Files Changed

- `Sources/OpenKey/win32/OpenKey/OpenKey/OpenKey.cpp`
  - Added `_skipImeCheckApps` list
  - Added `shouldSkipImeCheck()` function
  - Added `OPENKEY_EXTRA_INFO` macro
  - Updated all `dwExtraInfo` usages
  - Updated hook check logic
  - Updated IME check logic

## Testing

1. Mở PowerPoint
2. Click vào text area (Title, Subtitle, etc.)
3. Đảm bảo OpenKey đang ở chế độ Vietnamese (V)
4. Gõ tiếng Việt (ví dụ: "Xin chaof")
5. Verify: Kết quả hiển thị "Xin chào" với dấu đúng
