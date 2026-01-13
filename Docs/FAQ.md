# NextKey FAQ - Câu hỏi thường gặp

## Mục lục

1. [Vấn đề với Notepad Windows 11](#vấn-đề-với-notepad-windows-11)
2. [Vấn đề với PowerPoint](#vấn-đề-với-powerpoint)
3. [Lag khi chuyển ứng dụng](#lag-khi-chuyển-ứng-dụng)

---

## Vấn đề với Notepad Windows 11

### ❓ Gõ tiếng Việt bị mất dấu hoặc mất ký tự trong Notepad mới

**Triệu chứng:**
- `dùng` → `dung` (mất dấu)
- `dùng, gõ` → `dùng, go` (mất dấu ở từ sau)
- `dùng, ủa` → `dùng,` (mất hoàn toàn ký tự)

**Nguyên nhân:**

Notepad mới của Windows 11 (phiên bản 11.2508+) có tính năng **Spell Check** và **Auto-complete** tích hợp. Những tính năng này conflict với cách Vietnamese IME gửi ký tự qua `SendInput`, dẫn đến:
- Input events bị "block" hoặc "drop"
- Ký tự có dấu bị mất dấu hoặc mất hoàn toàn

**Giải pháp:**

1. Mở Notepad → Settings (⚙️)
2. Trong phần **Text Formatting**:
   - Tắt **Formatting** → OFF
3. Nếu vẫn lỗi, thử tắt thêm các tùy chọn khác trong Settings

| Setting | Trạng thái | Lý do |
|---------|-----------|-------|
| Formatting | ❌ OFF | Chứa spell check, conflict với IME |
| Word wrap | ✅ Tùy ý | Không ảnh hưởng |

> [!NOTE]
> Vấn đề này xảy ra với tất cả Vietnamese IME (NextKey, Unikey, EVKey...), không riêng NextKey.

---

## Vấn đề với PowerPoint

### ❓ Không gõ được tiếng Việt trong PowerPoint

**Giải pháp:**

Thêm `powerpnt.exe` vào danh sách **Special Apps** với cấu hình:
- Type: Skip IME check

Xem chi tiết: [POWERPOINT_FIX.md](./POWERPOINT_FIX.md)

---

## Lag khi chuyển ứng dụng

### ❓ Lag ký tự đầu tiên khi chuyển sang VSCode, Discord, NotepadNext

**Nguyên nhân:**

Các ứng dụng Qt/Electron (VSCode, Discord, Slack, NotepadNext...) có cơ chế xử lý event khác, gây delay.

**Giải pháp:**

NextKey đã tối ưu cho các ứng dụng này từ phiên bản 1.0.3. Nếu vẫn gặp lag:
1. Đảm bảo đang dùng phiên bản NextKey mới nhất
2. Thêm ứng dụng vào **Special Apps** với cấu hình:
   - Type: Qt/Electron

Xem chi tiết: [OPTIMIZATION_DETAILS.md](./OPTIMIZATION_DETAILS.md)

---

## Các câu hỏi khác

### ❓ Làm sao để backup cài đặt?

Cài đặt NextKey được lưu trong Registry tại:
```
HKEY_CURRENT_USER\Software\NextKey
```

### ❓ NextKey có hỗ trợ kiểu gõ nào?

- VNI
- Telex
- Simple Telex 1/2

### ❓ Tôi muốn tắt NextKey cho một ứng dụng cụ thể?

Thêm ứng dụng vào danh sách **Excluded Apps** trong Settings.

---

*Cập nhật lần cuối: 2026-01-08*
