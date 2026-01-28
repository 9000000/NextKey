# NextKey Code Review Issues & Fixes

> **Generated:** 2026-01-28T17:43:22+07:00
> **Skill Applied:** C++ Best Practices (RAII, Smart Pointers, STL)
> **Status:** 🔄 In Progress

---

## 📊 Summary

| Severity | Count | Description |
|----------|-------|-------------|
| 🔴 Critical | 8 | Must fix - causes bugs or violates coding standards |
| 🟡 Medium | 8 | Should fix - maintainability/DRY violations |
| 🟢 Low | 8 | Nice to have - style/documentation |

**Total: 24 issues identified**

### 🛠️ Helper Files Created (Ready to Use)

| File | Purpose | Fixes | Status |
|------|---------|-------|--------|
| `StringUtils.h` | Centralized string utilities | CR-007, CR-008 | ✅ Created |
| `DwmConstants.h` | DWM attribute constants | CR-009, CR-017 | ✅ Created |

---

## � Quick Start Guide (For Junior Developers)

### Cách đọc document này:

1. **Mỗi issue có ID** (e.g., `CR-001`) - dùng để track trong commit message
2. **Files** - Danh sách file cần sửa VÀ line number cụ thể
3. **Problem** - Giải thích TẠI SAO đây là vấn đề
4. **Fix** - Code example với ❌ BEFORE và ✅ AFTER
5. **Priority** - Khi nào nên fix

### Workflow khi fix issue:

```bash
# Step 1: Tạo branch mới
git checkout -b fix/CR-XXX-short-description

# Step 2: Mở file theo line number trong document
# (VSCode: Ctrl+G để jump to line)

# Step 3: Áp dụng fix theo code example

# Step 4: Build và test
# Mở Visual Studio -> Build -> Build Solution (F7)

# Step 5: Commit với ID
git commit -m "Fix CR-XXX: Short description"
```

### ⚠️ Lưu ý quan trọng:

1. **Đừng fix nhiều issues cùng 1 commit** - Mỗi CR-XXX = 1 commit riêng
2. **Build sau mỗi thay đổi** - Đảm bảo không break code
3. **Tìm-thay toàn bộ file** - Khi remove `using namespace`, phải thêm prefix cho TẤT CẢ các usage trong file đó
4. **Update document** - Đánh dấu ✅ trong bảng "Completed Fixes" sau khi xong

### 🧪 Checklist trước khi commit:

- [ ] Build thành công (cả Debug và Release)
- [ ] Không có warning mới
- [ ] Test thủ công: Mở app, gõ tiếng Việt, kiểm tra tính năng liên quan
- [ ] Code format đúng (tabs, không trailing whitespace)

---

## 🔴 Critical Issues

### CR-001: `using namespace Gdiplus;` in Source Files

**Files:** 
- `ModernMenu.cpp:22`
- `SystemTrayHelper.cpp:43`

**Problem:** 
Per `coding.md` rule: "NO `using namespace std;` in headers." Same applies to Gdiplus - can cause naming conflicts (e.g., `Gdiplus::Font` vs Windows `FONT`).

**Step-by-step Fix:**

1. Mở file (e.g., `ModernMenu.cpp`)
2. Xóa dòng `using namespace Gdiplus;`
3. Thêm prefix `Gdiplus::` cho TẤT CẢ các type sau trong file:

   | Type to find | Replace with |
   |--------------|--------------|
   | `Graphics` | `Gdiplus::Graphics` |
   | `SolidBrush` | `Gdiplus::SolidBrush` |
   | `Pen` | `Gdiplus::Pen` |
   | `Color` | `Gdiplus::Color` |
   | `Font` | `Gdiplus::Font` |
   | `FontFamily` | `Gdiplus::FontFamily` |
   | `StringFormat` | `Gdiplus::StringFormat` |
   | `RectF` | `Gdiplus::RectF` |
   | `PointF` | `Gdiplus::PointF` |
   | `LinearGradientBrush` | `Gdiplus::LinearGradientBrush` |
   | `GdiplusStartupInput` | `Gdiplus::GdiplusStartupInput` |

4. **Cách làm nhanh với VSCode:**
   - Ctrl+H (Find and Replace)
   - Enable "Match Whole Word" (Alt+W)
   - Tìm: `SolidBrush` → Thay: `Gdiplus::SolidBrush`
   - Click "Replace All" 
   - Lặp lại cho từng type

5. Build và fix bất kỳ lỗi nào còn sót

**⚠️ Cẩn thận:** 
- `Color` có thể conflict với Windows `COLORREF` - đảm bảo thay đúng
- Argument types không cần prefix (e.g., `Color(255,0,0)` → `Gdiplus::Color(255,0,0)`)

```cpp
// ❌ BEFORE
using namespace Gdiplus;
SolidBrush brush(Color(255, 0, 0));

// ✅ AFTER
Gdiplus::SolidBrush brush(Gdiplus::Color(255, 0, 0));
```

**Priority:** HIGH - Fix when touching these files

---

### CR-002: `using namespace std;` in Source Files

**Files:**
- `Macro.cpp:18`

**Problem:**
Same as CR-001 - namespace pollution. Có thể gây conflict với `std::byte` và Windows `byte`.

**Step-by-step Fix:**

1. Mở file `Macro.cpp`
2. Xóa dòng `using namespace std;` (line 18)
3. Thêm prefix `std::` cho các type sau:

   | Type to find | Replace with |
   |--------------|--------------|
   | `string` | `std::string` |
   | `wstring` | `std::wstring` |
   | `vector` | `std::vector` |
   | `map` | `std::map` |
   | `cout` | `std::cout` |
   | `endl` | `std::endl` |
   | `hex` | `std::hex` |
   | `dec` | `std::dec` |
   | `ofstream` | `std::ofstream` |
   | `ifstream` | `std::ifstream` |

4. **Cách làm nhanh:** 
   - Ctrl+H, enable "Match Whole Word"
   - Tìm: `string` → Thay: `std::string`
   - ⚠️ **Cẩn thận:** Đừng replace trong `wstring` thành `wstd::string`!

5. Build và fix lỗi còn sót

```cpp
// ❌ BEFORE  
using namespace std;
map<vector<Uint32>, MacroData> macroMap;

// ✅ AFTER
std::map<std::vector<Uint32>, MacroData> macroMap;
```

**Priority:** HIGH - Fix when touching this file

---

### CR-003: Cryptic Global Loop Variables

**File:** `Engine.cpp:169-172`

```cpp
static int i, ii, iii;
static int j;
static int k, kk;
static int l;
```

**Problem:**
Per `coding.md` rule: "DO NOT leave cryptic variable names untouched if you understand their purpose."

**Fix:**
```cpp
// Option A: Use local variables (preferred)
for (int i = 0; i < size; i++) { ... }

// Option B: Rename if global is required
static int spellingCheckIdx;
static int consonantCheckIdx;
static int vowelCheckIdx;
static int markCheckIdx;
```

**Priority:** MEDIUM - Rename incrementally when modifying related functions

---

### CR-004: Static Loop Variables (Thread Safety Risk)

**File:** `Engine.cpp:169-172`

**Problem:**
Using `static int i, j, k` as loop counters is dangerous - can cause subtle bugs if called recursively or concurrently.

**Why this matters:**
```cpp
// ❌ BAD: Static variable keeps value between calls!
static int i;
void processA() {
    for (i = 0; i < 5; i++) {  // i is 0,1,2,3,4
        processB();  // This CHANGES i!
    }
}
void processB() {
    for (i = 0; i < 3; i++) { }  // After this, i = 3
    // When we return to processA, i is now 3, not where we left off!
}
```

**Step-by-step Fix:**

1. Mở file `Engine.cpp`, đi đến line 169-172
2. Xóa các dòng khai báo static:
   ```cpp
   // ❌ XÓA:
   static int i, ii, iii;
   static int j;
   static int k, kk;
   static int l;
   ```
3. Trong MỖI function sử dụng biến này, khai báo local variable:
   ```cpp
   // ✅ THÊM vào trong mỗi function
   void checkSpelling(const bool& forceCheckVowel) {
       int i, j, k, l;  // Local to this function
       // ... rest of code
   }
   ```
4. Search toàn file cho các function khác dùng `i`, `j`, `k` và thêm local declaration

**⚠️ Cẩn thận:** 
- Đây là thay đổi lớn, ảnh hưởng nhiều function
- Recommend: Fix từng function một, build và test sau mỗi lần

**Priority:** HIGH - Critical for thread safety

---

### CR-005: Duplicate GDI+ Initialization

**Files:**
- `ModernMenu.cpp:28` - `static ULONG_PTR ModernMenu::s_gdiToken = 0;`
- `SystemTrayHelper.cpp:819` - `static ULONG_PTR gdiplusToken = 0;`

**Problem:**
Two independent GDI+ initialization tokens. Both files call `GdiplusStartup()` separately.

**Fix:**
Centralize in a singleton or shared initialization:
```cpp
// In GdiPlusManager.h
namespace GdiPlusManager {
    void init();
    void shutdown();
    bool isInitialized();
}
```

**Priority:** MEDIUM - Potential resource leak

---

### CR-006: Missing `break` / Indentation Issue in Switch

**File:** `SystemTrayHelper.cpp:477`

**Problem:**
The `case ConfigIntentType::UPDATE_CONVERT_TOOL:` appears to be inside wrong scope due to indentation. This could cause **fallthrough** - code from one case accidentally runs into the next.

**Why this matters:**
```cpp
// ❌ BAD: No break = fallthrough!
switch (type) {
    case TYPE_A:
        doA();
        // Missing break! Will ALSO run doB()!
    case TYPE_B:
        doB();
        break;
}
```

**Step-by-step Fix:**

1. Mở file `SystemTrayHelper.cpp`, đi đến line ~477
2. Tìm đoạn `switch (intent.intentType)` 
3. Kiểm tra TỪNG `case` có `break;` ở cuối không
4. Thêm braces `{}` cho mỗi case để dễ đọc:

```cpp
// ✅ GOOD: Braces + explicit break
switch (intent.intentType) {
    case ConfigIntentType::UPDATE_SETTINGS: {
        // Handle settings
        loadConfigFromPayload(payload);
        break;  // ← QUAN TRỌNG!
    }
    case ConfigIntentType::UPDATE_CONVERT_TOOL: {
        // Handle convert tool
        updateConvertToolSettings();
        break;  // ← QUAN TRỌNG!
    }
    default: {
        break;
    }
}
```

**⚠️ Cẩn thận:**
- Nếu intentional fallthrough (muốn chạy qua), thêm comment `// fallthrough` để rõ ràng
- Compiler warning `-Wimplicit-fallthrough` sẽ báo nếu thiếu break

**Priority:** HIGH - Potential fallthrough bug

---

## 🟡 Medium Priority Issues

### CR-007: Duplicate `toLower()` Function

**Files:**
- `OpenKey.cpp:104` - `static string strToLower(...)`
- `RuntimeProfile.cpp:32` - `static std::string toLower(...)`
- Potentially others

**Fix:**
Create shared utility:
```cpp
// StringUtils.h
#pragma once
#include <string>
#include <algorithm>

namespace StringUtils {
    inline std::string toLower(const std::string& s) {
        std::string result = s;
        std::transform(result.begin(), result.end(), result.begin(),
            [](unsigned char c) { return std::tolower(c); });
        return result;
    }
}
```

**Priority:** MEDIUM - DRY violation

---

### CR-008: Duplicate `wideToUtf8()` Function

**Files:**
- `OpenKey.cpp:95` - `static string wideToUtf8(...)`
- `ConfigManager.cpp:122` - `std::string wideToUtf8(...)`
- `Engine.cpp:202` - `string wideStringToUtf8(...)`

**Fix:**
Centralize in `StringUtils.h`:
```cpp
namespace StringUtils {
    std::string wideToUtf8(const std::wstring& wide);
    std::wstring utf8ToWide(const std::string& utf8);
}
```

**Priority:** MEDIUM - Code duplication

---

### CR-009: Magic Numbers for DWM Attributes

**Files:**
- `ModernMenu.cpp:111,114,118`
- `SettingsDialog.cpp` (multiple locations)

```cpp
DwmSetWindowAttribute(hwnd, 20, &dark, sizeof(dark));  // 20 = ???
DwmSetWindowAttribute(hwnd, 33, &corner, sizeof(corner));  // 33 = ???
DwmSetWindowAttribute(hwnd, 38, &backdrop, sizeof(backdrop));  // 38 = ???
```

**Fix:**
Create constants header:
```cpp
// DwmConstants.h
#pragma once

// DWM Window Attributes (Windows 10/11)
constexpr DWORD DWMWA_USE_IMMERSIVE_DARK_MODE = 20;
constexpr DWORD DWMWA_WINDOW_CORNER_PREFERENCE = 33;
constexpr DWORD DWMWA_SYSTEMBACKDROP_TYPE = 38;

// DWM Backdrop Types
constexpr DWORD DWMSBT_AUTO = 0;
constexpr DWORD DWMSBT_NONE = 1;
constexpr DWORD DWMSBT_MAINWINDOW = 2;       // Mica
constexpr DWORD DWMSBT_TRANSIENTWINDOW = 3;  // Acrylic
constexpr DWORD DWMSBT_TABBEDWINDOW = 4;     // Tabbed Mica

// DWM Corner Preferences
constexpr DWORD DWMWCP_DEFAULT = 0;
constexpr DWORD DWMWCP_DONOTROUND = 1;
constexpr DWORD DWMWCP_ROUND = 2;
constexpr DWORD DWMWCP_ROUNDSMALL = 3;
```

**Priority:** MEDIUM - Maintainability

---

### CR-010: Duplicate `WM_SHOW_MODERN_MENU` Definition

**Files:**
- `ModernMenu.cpp:26` - `#define WM_SHOW_MODERN_MENU (WM_USER + 1005)`
- `SystemTrayHelper.cpp:598` - Same definition

**Fix:**
Move to shared header with guard:
```cpp
// In stdafx.h or Messages.h
#ifndef WM_SHOW_MODERN_MENU
#define WM_SHOW_MODERN_MENU (WM_USER + 1005)
#endif
```

**Priority:** LOW - No bug but code smell

---

### CR-011: Unused Variable Warning

**File:** `SystemTrayHelper.cpp:766`

```cpp
ATOM atom = RegisterClassExW(&wcex);  // Unused
```

**Fix:**
```cpp
// Option A: Discard explicitly
(void)RegisterClassExW(&wcex);

// Option B: Check for error
if (!RegisterClassExW(&wcex)) {
    // Handle error
}
```

**Priority:** LOW - Compiler warning

---

### CR-012: COM Initialization Without Cleanup

**File:** `SystemTrayHelper.cpp:849`

```cpp
CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
// No matching CoUninitialize()
```

**Fix:**
Use RAII wrapper:
```cpp
struct ComInitializer {
    HRESULT hr;
    ComInitializer() : hr(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)) {}
    ~ComInitializer() { if (SUCCEEDED(hr)) CoUninitialize(); }
    bool succeeded() const { return SUCCEEDED(hr); }
};
```

**Priority:** MEDIUM - Potential COM leak

---

### CR-013: Long Functions (> 100 lines)

**Files:**
| Function | Lines | File |
|----------|-------|------|
| `WndProc()` | ~624 | SystemTrayHelper.cpp |
| `OpenKeyInit()` | ~132 | OpenKey.cpp |
| `checkSpelling()` | ~113 | Engine.cpp |
| `SubclassProc()` | ~292 | SettingsDialog.cpp |
| `handle_event()` | ~865 | SettingsDialog.cpp |

**Fix:**
Extract helper functions:
```cpp
// Before: Monolithic WndProc
LRESULT WndProc(...) {
    switch (message) {
        case WM_COPYDATA: /* 200 lines */ break;
        // ...
    }
}

// After: Extracted handlers
LRESULT HandleCopyData(HWND, LPARAM);
LRESULT HandleModernMenu(HWND, WPARAM, LPARAM);

LRESULT WndProc(...) {
    switch (message) {
        case WM_COPYDATA: return HandleCopyData(hWnd, lParam);
        case WM_SHOW_MODERN_MENU: return HandleModernMenu(hWnd, wParam, lParam);
    }
}
```

**Priority:** MEDIUM - Maintainability

---

### CR-014: Global Variables Pollution

**File:** `OpenKey.cpp:206-229`

30+ static global variables for hook state.

**Fix (Long-term):**
Encapsulate in class:
```cpp
class HookManager {
    HHOOK m_keyboardHook = nullptr;
    HHOOK m_mouseHook = nullptr;
    HWINEVENTHOOK m_systemEvent = nullptr;
    vKeyHookState* m_pData = nullptr;
    std::vector<Uint16> m_syncKey;
    // ...
public:
    static HookManager& instance();
    void init();
    void cleanup();
    void reinstallHooks();
};
```

**Priority:** LOW - Architecture improvement

---

## 🟢 Low Priority Issues

### CR-015: Inconsistent Naming Convention

**Pattern Observed:**
- camelCase: `_cachedImeState`
- snake_case: `_index`
- PascalCase: `TypingWord`
- ALL_CAPS: `MASK_SHIFT`

**Recommendation:**
Document and standardize:
- `m_member` for class members
- `s_static` for static class members
- `g_global` for global variables
- `kConstant` or `CONSTANT` for constants
- `functionName()` for functions

**Priority:** LOW - Style consistency

---

### CR-016: Dead Code / Commented Code

**File:** `OpenKey.cpp:600-601`

```cpp
/*if (!(vCodeTable == 3 && containUnicodeCompoundApp(FRONT_APP))) {
    SendInput(2, backspaceEvent, sizeof(INPUT));
}*/
```

**Fix:**
Remove or document why it's kept.

**Priority:** LOW - Code cleanliness

---

### CR-017: Duplicate `ACCENT_POLICY` Struct Definition

**Files:**
- `ModernMenu.cpp:33-38`
- `SettingsDialog.cpp:217-222`

**Fix:**
Move to shared header.

**Priority:** LOW - DRY violation

---

### CR-018: Raw Pointer Return from `AddSubMenu()`

**File:** `ModernMenu.cpp:69-76`

```cpp
ModernMenu* ModernMenu::AddSubMenu(const std::wstring& text) {
    ModernMenu* rawPtr = item.subMenu.get();  // ⚠️ Raw pointer escape
    return rawPtr;
}
```

**Fix:**
Document lifetime or return reference:
```cpp
// Option A: Document
// @returns Pointer valid until Clear() or destruction
ModernMenu* AddSubMenu(const std::wstring& text);

// Option B: Return reference
ModernMenu& AddSubMenu(const std::wstring& text);
```

**Priority:** LOW - Potential safety issue

---

### CR-019: Missing Header Guards Verification

**Action:**
Verify all `.h` files have proper `#pragma once` or include guards.

**Priority:** LOW - Standard practice

---

### CR-020: Debug `cout` Statements Left in Production Code

**File:** `Macro.cpp` - các line sau:
- Line 111: `cout << "[Macro] Init: key[0]=...`
- Line 168: `cout << "[Macro] Exact match found!...`
- Line 172: `cout << "[Macro] Exact match failed...`
- Line 182: `cout << "[Macro] Testing AutoCaps...`
- Line 184: `cout << "[Macro] Lowercased key[0]...`
- Line 189: `cout << "[Macro] AutoCaps match found!...`

**Problem:**
Console output in production code:
1. Pollutes stdout (affects parent processes) 
2. Performance overhead (~1-5ms mỗi lần write)
3. Exposes internal state (security risk)

**Step-by-step Fix (Simplest - Just Delete):**

1. Mở file `Macro.cpp`
2. Ctrl+G → nhập `111` → Enter (đi đến line 111)
3. Xóa toàn bộ dòng `cout << "[Macro] Init...` 
4. Lặp lại cho các line 168, 172, 182, 184, 189
5. Build và test

**⚠️ Cẩn thận:** 
- Sau khi xóa line 111, các line number khác sẽ shift xuống!
- **Tip:** Xóa từ line LỚN nhất trước (189 → 184 → 182 → 172 → 168 → 111)

**Alternative - Dùng Find & Delete:**
```
Ctrl+H
Find: cout << "\[Macro\].*endl;
Replace: (để trống)
Enable Regex: ✅
Click: Replace All
```

**Priority:** HIGH - Production code quality

---

### CR-021: Static Global Loop Variable `c`

**File:** `Macro.cpp:25`

```cpp
static int c = 0;
```

Used in multiple functions (`findMacro`, `getAllMacro`, etc.) as loop counter.

**Problem:**
Same as CR-004 - thread safety risk and unclear purpose.

**Fix:**
Replace with local loop variable in each function.

**Priority:** HIGH

---

### CR-022: Old-Style Iterator Loops

**File:** `Macro.cpp:120, 144, 220, 253, 263`

```cpp
for (std::map<vector<Uint32>, MacroData>::iterator it = macroMap.begin(); 
     it != macroMap.end(); ++it) { ... }
```

**Fix:**
Use modern range-based for:
```cpp
// Modern C++17 style
for (const auto& [key, data] : macroMap) {
    // Use key, data directly
}

// Or with auto iterator if modification needed
for (auto& [key, data] : macroMap) {
    data.macroContentCode = convert(data.macroContent);
}
```

**Priority:** LOW - Modernization

---

### CR-023: Empty Catch/Else Blocks

**File:** `OpenKeyHelper.cpp:412-418`

```cpp
} else if (res == E_OUTOFMEMORY) {
    // Empty - no handling
} else if (res == INET_E_DOWNLOAD_FAILURE) {
    // Empty - no handling
} else {
    // Empty - no handling
}
```

**Problem:**
Empty error handling blocks are code smells - either log the error or remove the conditions.

**Fix:**
```cpp
} else if (res == E_OUTOFMEMORY) {
    OutputDebugStringA("URL download failed: out of memory\n");
} else if (res == INET_E_DOWNLOAD_FAILURE) {
    OutputDebugStringA("URL download failed: network error\n");
} else {
    char buf[64];
    sprintf_s(buf, "URL download failed: 0x%08X\n", (unsigned)res);
    OutputDebugStringA(buf);
}
// OR simply:
if (res != S_OK) return L"";  // Simpler if we don't care about specific errors
```

**Priority:** LOW

---

### CR-024: Shadow Variable `hKey`

**File:** `OpenKeyHelper.cpp:423`

```cpp
bool OpenKeyHelper::isWindowsDarkMode() {
    HKEY hKey;  // ⚠️ Shadows static HKEY hKey at line 29!
```

**Problem:**
Local `hKey` shadows static global `hKey`, making code confusing.

**Fix:**
```cpp
bool OpenKeyHelper::isWindowsDarkMode() {
    HKEY hThemeKey;  // Use distinct name
```

**Priority:** LOW - Naming clarity

---

## 📋 Prioritized Action Plan

### Phase 1: High Priority (When Touching Files)
1. [ ] CR-001: Remove `using namespace Gdiplus;`
2. [ ] CR-002: Remove `using namespace std;`
3. [ ] CR-004: Replace static loop variables with local
4. [ ] CR-006: Fix potential switch fallthrough

### Phase 2: Technical Debt Sprint
5. [ ] CR-007+CR-008: Create `StringUtils.h` with shared functions
6. [ ] CR-009: Create `DwmConstants.h`
7. [ ] CR-005: Centralize GDI+ initialization
8. [ ] CR-012: Add COM RAII wrapper

### Phase 3: Refactoring (Long-term)
9. [ ] CR-003: Rename cryptic variables
10. [ ] CR-013: Split long functions
11. [ ] CR-014: Encapsulate global state
12. [ ] CR-015: Standardize naming convention

---

## ✅ Completed Fixes

| ID | Description | Date | PR/Commit |
|----|-------------|------|-----------|
| - | - | - | - |

---

## 📝 Notes

- Follow **Boy Scout Rule**: Leave code cleaner than you found it
- Apply fixes incrementally when modifying related code
- Run build verification after each fix
- Update this document when fixes are completed
