# 📘 TECH SPEC

## Per-App Override Configuration for Input Engine

**Project**: OpenKey / NextKey
**Audience**: Core engine dev
**Status**: Approved – aligned with current engine behavior
**Principle**: *Default works → Config exists only to override failure cases*

---

## 1. Triết lý thiết kế (Design Philosophy)

### 1.1 Nguyên tắc cốt lõi

> **Engine phải tự chạy tốt trước.
> Config chỉ dùng khi engine không tự cứu được.**

* Nếu default đã xử lý đúng → **UI không được cho user chọn lại**
* Nếu user phải mở config → **đó là vì có lỗi thật**

### 1.2 Những gì KHÔNG làm

* ❌ Không expose "Auto / Default" trong UI
* ❌ Không cho user chọn SendInputKey (vốn là mặc định)
* ❌ Không migrate config cũ (dev tool, user tự chịu)
* ❌ Không biến config thành "control panel toàn năng"

---

## 2. Hiện trạng engine (Baseline – đã có sẵn)

Engine hiện tại **đã có logic mặc định**:

### 2.1 Default injection pipeline

```
SendInputKey
   ↓ fail
Clipboard fallback (runtime)
   ↓ fail
Downgrade / log
```

### 2.2 Default auto-handling

| Case                      | Engine đã xử lý                       |
| ------------------------- | ------------------------------------- |
| Qt/Electron lag ký tự đầu | Window class detect → skip empty char |
| Clipboard API fail        | failureCount → downgrade              |
| App native                | SendInputKey                          |
| Không config              | Engine tự quyết                       |

👉 **Do đó: "Tự động" đã tồn tại trong code, không phải feature UI**

---

## 3. Mục đích của config per-app

Config **chỉ tồn tại** cho các trường hợp:

> "App này gõ không ra chữ, engine không tự detect được, user cần ép hành vi"

Không hơn, không kém.

---

## 4. Nhóm config chính (Final Grouping)

### 🔹 Nhóm A – Lỗi IME / xử lý tiếng Việt

**Khi nào dùng**

* App không gõ được tiếng Việt
* WinAPI IME check sai
* TSF ổn nhưng engine hiểu nhầm

**Config**

```
☐ Bỏ qua kiểm tra IME cho ứng dụng này
```

**Mapping**

```cpp
profile.flags |= SkipImeCheck;
```

**Ví dụ**

* PowerPoint
* Một số app legacy Office / COM

---

### 🔹 Nhóm B – Lỗi lag ký tự đầu (Qt / Electron)

**Khi nào dùng**

* Gõ chữ đầu tiên bị delay 100–200ms
* Lazy input context init
* Engine auto-detect chưa bắt được

**Config**

```
☐ Ứng dụng Qt / Electron (bỏ ký tự rỗng đầu)
```

**Mapping**

```cpp
profile.flags |= SkipEmptyChar;
```

**Ví dụ**

* NotepadNext
* Custom Qt app build lạ

---

### 🔹 Nhóm C – Lỗi chèn ký tự (Clipboard Override)

> ⚠️ **Chỉ dùng khi SendInputKey KHÔNG hoạt động**

#### ❗ Nguyên tắc

* Không chọn gì = SendInputKey (mặc định)
* Không có "Auto"
* Không có "SendInputKey" trong UI

#### UI

**Tiêu đề**

> *Ép cách chèn ký tự (chỉ dùng khi gõ không ra chữ)*

**Options**

```
( ) Clipboard – Ctrl + V
( ) Clipboard – Shift + Insert
```

#### Mapping

```cpp
if (!hasClipboardOverride) {
    useSendInputKey(); // default
} else {
    forceClipboard(method);
}
```

**Ví dụ**

* Terminal → Shift + Insert
* Một số app text → Ctrl + V
* Game → SendInputKey vẫn default (không config)

---

## 5. Cấu trúc config (Không migrate)

### 5.1 Nguyên tắc lưu config

* Chỉ lưu **override**
* Không lưu default
* Không migrate config cũ

### 5.2 Data model đề xuất

```cpp
struct AppOverrideConfig {
    std::string exeName;

    bool skipImeCheck = false;
    bool skipEmptyChar = false;

    // Clipboard override
    // -1 = none (default SendInputKey)
    //  0 = Ctrl+V
    //  1 = Shift+Insert
    int8_t clipboardMethod = -1;
};
```

### 5.3 config.toml

```toml
[appOverrides]
apps = [
  { name = "powerpnt.exe", skipImeCheck = true },
  { name = "notepadnext.exe", skipEmptyChar = true },
  { name = "terminal.exe", clipboard = 1 }
]
```

---

## 6. UI Spec (rõ – ít – không gây hiểu nhầm)

### 6.1 Một dialog duy nhất: **"Cấu hình ứng dụng"**

* Không chia Special / Clipboard
* Một app = một hàng
* Checkbox = override

### 6.2 Không hiển thị nếu không cần

* Không có "Auto"
* Không có tooltip dài dòng
* Chỉ có option khi user gặp lỗi

---

## 7. Runtime Integration

### 7.1 Khi focus app

```cpp
RuntimeProfile profile = autoDetect(hwnd);

if (hasUserOverride(exe)) {
    applyOverrides(profile, userConfig);
}
```

### 7.2 Override luôn thắng auto-detect

> **User override > runtime heuristics**

Không merge logic, không ambiguity.

---

## 8. Memory & RAM Consideration

* Không lưu runtime-detected apps vào config
* RuntimeProfile cache theo HWND → tự cleanup
* Config chỉ lưu apps user đã **đụng tay**

👉 RAM không phình
👉 Config không bẩn

---

## 9. Những gì CỐ Ý KHÔNG LÀM

| Item               | Lý do                      |
| ------------------ | -------------------------- |
| Auto-detect toggle | Đã có trong engine         |
| Migrate config     | Đây là dev tool            |
| Delay tuning UI    | Runtime xử lý              |
| TSF deep hook      | Overkill cho milestone này |

---

## 10. Checklist cho dev

- [ ] UI chỉ chứa override
- [ ] Không expose default
- [ ] Không migrate config
- [ ] RuntimeProfile apply override sau auto-detect
- [ ] Không phá hot path
- [ ] Log rõ khi override active

---

## 11. One-liner summary

> **Engine tự động chạy trước.
> Config chỉ để ép khi engine không cứu được.
> Không expose mặc định, không migrate, không phình.**

---

## 12. Implementation Plan

### Phase 1: Data Layer
1. Add `AppOverrideConfig` struct to `ConfigManager`
2. Add `getAppOverrides()` / `setAppOverrides()` 
3. Integrate with RuntimeProfile in focus change handler

### Phase 2: UI Layer
1. Create single "Cấu hình ứng dụng" dialog
2. Remove old SpecialAppsDialog + ClipboardAppsDialog
3. No defaults shown, only user overrides

### Phase 3: Cleanup
1. Remove old config sections (specialApps, clipboardApps)
2. Remove hardcoded default app lists from dialog (keep in engine for auto-detect)
3. Update documentation

---

*Spec approved after senior review - 2026-01-19*
