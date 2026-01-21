# Nghiên cứu các bộ gõ tiếng Việt khác

Tài liệu này tổng hợp các tính năng và ý tưởng từ các dự án bộ gõ tiếng Việt nguồn mở (bao gồm các fork của OpenKey) để có thể học hỏi và tối ưu cho OpenKey.

## Nguồn tham khảo

| Dự án | Nền tảng | Công nghệ | Link |
|-------|----------|-----------|------|
| **Gõ Nhanh** | macOS, Linux, Windows (beta) | Swift, Rust (engine) | [GitHub](https://github.com/khaphanspace/gonhanh.org) |
| **XKey** | macOS | Swift Native, SwiftUI | [GitHub](https://github.com/xmannv/xkey) |
| **PHTV** | Windows (OpenKey Fork) | C++, Sciter | [GitHub](https://github.com/PhamHungTien/PHTV) |

---

## ✅ So sánh tính năng

### Các tính năng OpenKey ĐÃ CÓ

| Tính năng | OpenKey | Gõ Nhanh | XKey | PHTV |
|-----------|---------|----------|------|------|
| Quick Telex (cc=ch, nn=ng...) | ✅ `vQuickTelex` | ❌ | ✅ | ✅ |
| Smart Switch Key (nhớ app) | ✅ | ✅ | ✅ | ✅ |
| Kiểm tra chính tả | ✅ | ❌ | ✅ | ✅ |
| Phục hồi từ sai | ✅ `vRestoreIfWrongSpelling` | ✅ | ✅ | ✅ |
| Macro/Gõ tắt | ✅ | ✅ | ✅ | ✅ |
| Tự động cập nhật | ✅ | ✅ | ✅ | ✅ |
| Loại trừ ứng dụng | ✅ | ❌ | ❌ | ✅ |

---

## 🔥 Tính năng CHƯA CÓ - Có thể học hỏi

### 1. Auto-restore tiếng Anh (Gõ Nhanh / PHTV) ⭐⭐⭐

**Mô tả:** Khi gõ tiếng Anh bằng Telex, một số chữ cái bị nhận nhầm thành modifier tiếng Việt. Hệ thống tự động khôi phục khi nhấn **Space** nếu phát hiện pattern tiếng Anh.

> [!NOTE]
> Khác với `vRestoreIfWrongSpelling` của OpenKey (phục hồi từ SAI → gốc), tính năng này nhận diện **từ tiếng Anh** (có trong từ điển) và phục hồi thành chính từ đó.

**Ví dụ:**
| Gõ vào | macOS Telex | Gõ Nhanh/PHTV |
|--------|-------------|---------------|
| `text` | `têt` | `text` |
| `expect` | `ễpct` | `expect` |
| `window` | `ưindow` | `window` |

**Cách thức:**
- Có bảng từ điển các từ tiếng Anh phổ biến (developer words): `terminal`, `browser`, `text`, `expect`, `file`...
- PHTV cũng implement tính năng này, nhận diện các lỗi như `gôgle` -> `google`, `ủe` -> `user`.

**Áp dụng cho OpenKey:**
> [!TIP]
> Thêm option "Auto-restore English words" với bảng từ điển tiếng Anh phổ biến. Đây là tính năng RẤT HỮU ÍCH cho developer.

---

### 2. ESC để khôi phục ngay lập tức (Gõ Nhanh) ⭐⭐

**Mô tả:** Gõ `user` → `úẻ` → nhấn **ESC** → `user` được khôi phục **ngay lập tức**.

> [!IMPORTANT]
> OpenKey có `vRestoreIfWrongSpelling` nhưng chỉ trigger khi nhấn space/break-key. Tính năng ESC cho phép user chủ động khôi phục **bất cứ lúc nào**.

**Áp dụng cho OpenKey:**
- Lưu buffer gốc (trước khi xử lý dấu) trong session
- Khi nhấn ESC: khôi phục buffer gốc thay vì buffer đã có dấu

---

### 3. Tắt bộ gõ khi giữ phím Alt (Logic PHTV) ⭐⭐

**Mô tả:** 
- **PHTV Pattern:** GIỮ phím Alt để tạm tắt bộ gõ, THẢ ra để bật lại (Hold-to-disable).
- **OpenKey Pattern (Hiện tại):** NHẤN-THẢ Alt để toggle chế độ "Temp Off", reset khi gặp word break.

**So sánh:**
- **Hold (PHTV):** Trực quan hơn cho hành động "muốn gõ shortcut một chút".
- **Toggle (OpenKey):** Tiện nếu muốn gõ một chuỗi phím tắt dài mà không muốn giữ phím.

**Áp dụng cho OpenKey:**
> [!NOTE]
> Có thể xem xét option cho phép user chọn behavior: "Hold to disable" hoặc "Toggle temporary".

---

### 4. Macro Categories (PHTV) ⭐

**Mô tả:** Tổ chức macro theo nhóm (Folder/Category) thay vì một list dài.
**Độ khó:** ⭐⭐ | **Hữu ích:** ⭐

**Áp dụng cho OpenKey:**
- Giúp quản lý macro tốt hơn nếu user có >50 macros. Tuy nhiên UI sẽ phức tạp hơn.

---

### 5. Non-Latin keyboard detection (PHTV) ⭐

**Mô tả:** Tự động tắt bộ gõ tiếng Việt khi phát hiện người dùng chuyển sang bàn phím Nhật/Trung/Hàn.

**Áp dụng cho OpenKey:**
- OpenKey hiện có logic check layout keyboard, cần verify xem đã cover trường hợp CJK IME chưa.

---

### 6. Hiệu chỉnh Engine theo ứng dụng (XKey) ⭐⭐⭐

**Mô tả:** Phát hiện ngữ cảnh đặc biệt dựa trên tiêu đề cửa sổ, áp dụng xử lý phù hợp (injection method, delay...).

---

### 7. Export/Import Settings (XKey / PHTV) ⭐⭐⭐

**Mô tả:** Khả năng backup toàn bộ cấu hình ra file để mang sang máy khác hoặc backup trước khi cài lại Win.

**Áp dụng cho OpenKey:**
> [!IMPORTANT]
> Đây là tính năng user request nhiều. Cần implement function serialize settings Registry -> JSON/TOML và ngược lại.

---

## 📊 Đánh giá theo tiêu chí: Hữu ích ↔ Hiệu suất/Ổn định

> [!IMPORTANT]
> Tiêu chí đánh giá: **Chỉ implement tính năng HỮU ÍCH mà KHÔNG làm nặng hệ thống**

### ✅ NÊN IMPLEMENT (Lightweight, High Value)

| Tính năng | Hữu ích | Hiệu suất | Đánh giá |
|-----------|---------|-----------|----------|
| **ESC khôi phục text gốc** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ (0 overhead) | ✅ **NÊN LÀM** |
| | Rất cần cho dev/user gõ nhầm | Chỉ cần lưu buffer sẵn có | Single keystroke check |
| **Export/Import Settings** | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ (0 runtime) | ✅ **NÊN LÀM** |
| | Backup, migrate PC | Chỉ chạy khi user bấm | One-time action |
| **Auto-restore tiếng Anh** | ⭐⭐⭐⭐ | ⭐⭐⭐ (vừa) | ✅ **NÊN LÀM** (có option tắt) |
| | Dev rất cần | Lookup từ điển nhỏ | Check khi word complete |

### ⚠️ CÂN NHẮC (Trade-off)

| Tính năng | Hữu ích | Hiệu suất | Đánh giá |
|-----------|---------|-----------|----------|
| **Hold Alt to Disable** | ⭐⭐⭐ | ⭐⭐⭐⭐ (nhẹ) | ⚠️ **CÂN NHẮC** (Thay đổi habits) |
| | UX mượt mà | Logic check hold state | Cần option để user chọn |
| **Macro Categories** | ⭐⭐ | ⭐⭐⭐ (UI complex) | ⚠️ **LOW PRIORITY** |

### ❌ KHÔNG NÊN (Heavy, Complex, Low Value)

| Tính năng | Lý do KHÔNG NÊN |
|-----------|-----------------|
| **Per-app engine customizations** | ❌ Phức tạp, UI rối, ít user cần deep config như vậy |
| **Debug Window** | ❌ Chỉ dành cho dev, làm nặng app release |

---

## 📝 Kế hoạch hành động (Updated)

### ✅ Nên làm ngay
- [ ] **ESC khôi phục text gốc** - Chỉ cần lưu buffer & check ESC key
- [ ] **Export/Import Settings** - Button trong Settings, chỉ chạy khi bấm
- [ ] **Auto-restore tiếng Anh** - Xây dựng từ điển các từ dev/IT common (dictionary-based approach)

### ⚠️ Backlog (Làm sau)
- [ ] **Hold Alt to Disable** - Nghiên cứu UX switch
- [ ] **Review Non-Latin detection** - Kiểm tra logic hiện tại với CJK IMEs

### ❌ Không làm
- Per-app engine customization (quá phức tạp)
- Debug Window (chỉ cho dev, nặng)
- Macro Categories (chưa cần thiết lúc này)

---

*Tài liệu được tổng hợp vào ngày 2026-01-21*
*OpenKey Research Team*
