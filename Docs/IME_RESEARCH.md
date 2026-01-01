# Nghiên cứu các bộ gõ tiếng Việt khác

Tài liệu này tổng hợp các tính năng và ý tưởng từ hai dự án bộ gõ tiếng Việt nguồn mở để có thể học hỏi và tối ưu cho OpenKey.

## Nguồn tham khảo

| Dự án | Nền tảng | Công nghệ | Link |
|-------|----------|-----------|------|
| **Gõ Nhanh** | macOS, Linux, Windows (beta) | Swift, Rust (engine) | [GitHub](https://github.com/khaphanspace/gonhanh.org) |
| **XKey** | macOS | Swift Native, SwiftUI | [GitHub](https://github.com/xmannv/xkey) |

---

## ✅ So sánh tính năng

### Các tính năng OpenKey ĐÃ CÓ

| Tính năng | OpenKey | Gõ Nhanh | XKey |
|-----------|---------|----------|------|
| Quick Telex (cc=ch, nn=ng...) | ✅ `vQuickTelex` | ❌ | ✅ |
| Smart Switch Key (nhớ app) | ✅ | ✅ | ✅ |
| Kiểm tra chính tả | ✅ | ❌ | ✅ |
| Phục hồi từ sai | ✅ `vRestoreIfWrongSpelling` | ✅ | ✅ |
| Macro/Gõ tắt | ✅ | ✅ | ✅ |
| Tự động cập nhật | ✅ | ✅ | ✅ |
| Loại trừ ứng dụng | ✅ | ❌ | ❌ |

---

## 🔥 Tính năng CHƯA CÓ - Có thể học hỏi

### 1. Auto-restore tiếng Anh (Gõ Nhanh) ⭐⭐⭐

**Mô tả:** Khi gõ tiếng Anh bằng Telex, một số chữ cái bị nhận nhầm thành modifier tiếng Việt. Gõ Nhanh tự động khôi phục khi nhấn **Space** nếu phát hiện pattern tiếng Anh.

> [!NOTE]
> Khác với `vRestoreIfWrongSpelling` của OpenKey (phục hồi từ SAI → gốc), tính năng này nhận diện **từ tiếng Anh** và phục hồi thành chính từ đó.

**Ví dụ:**
| Gõ vào | macOS Telex | Gõ Nhanh |
|--------|-------------|----------|
| `text` | `têt` | `text` |
| `expect` | `ễpct` | `expect` |
| `window` | `ưindow` | `window` |

**Cách thức:**
- Có bảng từ điển các từ tiếng Anh phổ biến: `text`, `next`, `test`, `expect`, `window`, `user`, `file`...
- Khi nhấn Space, nếu pattern khớp → tự động khôi phục nguyên văn

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

### 3. Tự động theo Input Source (Gõ Nhanh)

**Mô tả:** Khi dùng tiếng Nhật, Hàn, Trung... → Gõ Nhanh tự tắt. Chuyển về tiếng Anh/Việt → tự bật lại.

**Áp dụng cho OpenKey:**
> [!NOTE]
> OpenKey có thể monitor sự thay đổi input method của Windows (Japanese IME, Korean IME...) và tự động disable/enable tương ứng.

---

### 4. Hiệu chỉnh Engine theo ứng dụng (XKey) ⭐⭐⭐

**Mô tả:** Phát hiện ngữ cảnh đặc biệt dựa trên tiêu đề cửa sổ, áp dụng xử lý phù hợp cho từng context.

**Cấu hình chi tiết:**
- **Bundle ID / Process Name:** `*` cho tất cả apps hoặc app cụ thể
- **Title Pattern:** Từ khóa nhận diện trong tiêu đề cửa sổ (hỗ trợ Regex)
- **Override Options:**
  - Ghi đè Injection Method: Fast, Slow, Selection
  - Tùy chỉnh Injection Delays (µs) cho Backspace, Wait, Text
  - Phương thức gửi text: Chunked hoặc One-by-One

**Ví dụ:** Chrome address bar, JetBrains IDEs, Google Docs cần delay khác với Notepad.

> [!IMPORTANT]
> Đây là tính năng rất mạnh mẽ. OpenKey có thể implement:
> - Per-app delay configuration
> - Per-window-title configuration (nhận diện web apps trong browser)

---

### 5. Tạm tắt thông minh bằng phím modifier (XKey)

**Mô tả:**
- **Giữ Ctrl** → Tạm tắt bộ gõ (cho đến khi thả ra)
- **Giữ Alt** → Tạm tắt kiểm tra chính tả

**Áp dụng cho OpenKey:**
> [!TIP]
> Option "Giữ Ctrl để tạm tắt bộ gõ" - rất hữu ích khi gõ shortcut hoặc gõ một từ tiếng Anh nhanh.

---

### ~~6. Tự động viết hoa đầu câu~~ ✅ ĐÃ CÓ

> OpenKey đã có tính năng này: `vUpperCaseFirstChar` (Viết hoa chữ cái đầu câu)

---

### 7. Debug Window (XKey)

**Mô tả:** Cửa sổ debug real-time hiển thị:
- Key events đang xảy ra
- Trạng thái engine
- Quyết định xử lý (tại sao chọn dấu này, tại sao không xử lý...)

**Áp dụng cho OpenKey:**
> [!TIP]
> Dành cho developer hoặc khi debug issues. Có thể bật từ Settings → Advanced → Show Debug Window.

---

### 8. Export/Import Settings (XKey)

**Mô tả:** XKey sử dụng Dual Storage System với khả năng backup/restore settings.

**Áp dụng cho OpenKey:**
> [!NOTE]
> OpenKey dùng Registry. Có thể thêm:
> - Export settings ra file JSON/INI
> - Import settings từ file
> - Backup trước khi update

---

## 📊 Đánh giá theo tiêu chí: Hữu ích ↔ Hiệu suất/Ổn định

> [!IMPORTANT]
> Tiêu chí đánh giá: **Chỉ implement tính năng HỮU ÍCH mà KHÔNG làm nặng hệ thống**

### ✅ NÊN IMPLEMENT (Lightweight, High Value)

| Tính năng | Hữu ích | Hiệu suất | Đánh giá |
|-----------|---------|-----------|----------|
| **ESC khôi phục text gốc** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ (0 overhead) | ✅ **NÊN LÀM** |
| | Rất cần cho dev/user gõ nhầm | Chỉ cần lưu buffer sẵn có, 0 CPU thêm | Single keystroke check |

| **Export/Import Settings** | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ (0 runtime) | ✅ **NÊN LÀM** |
| | Backup, migrate PC | Chỉ chạy khi user bấm, 0 runtime cost | One-time action |

### ⚠️ CÂN NHẮC (Trade-off)

| Tính năng | Hữu ích | Hiệu suất | Đánh giá |
|-----------|---------|-----------|----------|
| **Auto-restore tiếng Anh** | ⭐⭐⭐⭐ | ⭐⭐⭐ (vừa) | ⚠️ **CÂN NHẮC** |
| | Cực kỳ hữu ích cho dev | Cần từ điển ~5-10KB, lookup mỗi từ | Có thể làm option riêng |
| **Giữ Ctrl tạm tắt** | ⭐⭐⭐ | ⭐⭐⭐⭐ (nhẹ) | ⚠️ **CÂN NHẮC** |
| | Tiện khi gõ shortcut | Thêm logic check modifier state | Dễ conflict với app khác |

### ❌ KHÔNG NÊN (Heavy, Complex, Low Value)

| Tính năng | Lý do KHÔNG NÊN |
|-----------|-----------------|
| **Per-app engine customization** | ❌ Phức tạp, cần UI riêng, monitor window title liên tục = nặng |
| **Debug Window** | ❌ Chỉ dành cho dev, cần render liên tục = nặng, không cần cho user |
| **Tự động tắt theo IME khác** | ❌ Poll/Hook input source liên tục = tốn CPU, edge case ít gặp |

---

## 💡 Nguyên tắc thiết kế học được

### Từ Gõ Nhanh:
1. **Engine dựa trên ngữ âm học:** Sử dụng cấu trúc âm tiết tiếng Việt thay vì bảng tra cứu
   ```
   Âm tiết = [Phụ âm đầu] + [Âm đệm] + Nguyên âm chính + [Âm cuối] + Thanh điệu
   ```
2. **Cam kết "Ba Không":** Không thu phí, không quảng cáo, không theo dõi
3. **Hiệu suất cao:** <1ms latency, ~5MB RAM

### Từ XKey:
1. **Per-app customization:** Cho phép user tinh chỉnh behavior cho từng app
2. **Dual storage:** Backup settings để không bao giờ mất cấu hình
3. **Debug-friendly:** Có công cụ theo dõi real-time cho developer

---

## 📝 Kế hoạch hành động (Đã lọc theo tiêu chí)

### ✅ Nên làm (0 overhead, high value)
- [ ] **ESC khôi phục text gốc** - Chỉ cần lưu buffer & check ESC key
- [ ] **Export/Import Settings** - Button trong Settings, chỉ chạy khi bấm

### ⚠️ Cân nhắc (Làm sau, có trade-off)
- [ ] Auto-restore tiếng Anh - Cần từ điển, nhưng rất hữu ích cho dev

### ❌ Không làm
- Per-app engine customization (quá phức tạp)
- Debug Window (chỉ cho dev, nặng)
- Auto-tắt theo IME khác (poll liên tục)

---

*Tài liệu được tổng hợp vào ngày 2025-12-31*
*Đã đối chiếu với tính năng hiện có của OpenKey*
