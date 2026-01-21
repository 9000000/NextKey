# NextKey FAQ - Câu hỏi thường gặp

## Mục lục

1. [Vấn đề với Notepad Windows 11](#vấn-đề-với-notepad-windows-11)
2. [Cấu hình tương thích cho ứng dụng](#cấu-hình-tương-thích-cho-ứng-dụng)

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

## Cấu hình tương thích cho ứng dụng

### ❓ Tôi gặp lỗi gõ trên một số ứng dụng (Game, Remote Desktop, Excel...)

**Cơ chế tự động:**

NextKey hiện đã có cơ chế **tự động phát hiện và tối ưu** phương thức gõ cho từng ứng dụng (Native, Qt/Electron, Browser...). Đa số trường hợp bạn không cần cấu hình gì thêm.

**Cấu hình thủ công:**

Nếu vẫn gặp lỗi (mất ký tự, lag, nhân đôi ký tự), bạn có thể cấu hình thủ công cho ứng dụng đó bằng cách mở **"Cấu hình ứng dụng"** (nút có biểu tượng danh sách/bánh răng):

1. **Thêm ứng dụng:** Nhập tên file `.exe` (vd: `game.exe`) hoặc dùng nút **"Chọn cửa sổ"** để chọn ứng dụng đang mở.
2. **Tuỳ chọn cấu hình:**
    - **Xử lý đặc biệt:**
        - `Qt/Electron`: Dành cho các app chat như Discord, VSCode, Slack nếu thấy lag.
        - `Skip IME Check`: Dành cho các app Office (Word, Excel) hoặc Remote Desktop nếu bị lỗi kết hợp phím.
    - **Ép clipboard:** Bắt buộc dùng Clipboard để gửi ký tự. Hữu ích cho các ứng dụng không nhận tín hiệu phím ảo (SendInput) hoặc bị nhân đôi ký tự.

> [!TIP]
> Sử dụng nút **"Chọn cửa sổ"** là cách nhanh nhất để thêm cấu hình chuẩn xác cho ứng dụng bạn đang dùng.

---

## Các câu hỏi khác

### ❓ Làm sao để backup cài đặt?

NextKey lưu cài đặt trong file `config.toml`:

- **Vị trí mặc định:** Cùng thư mục với file `NextKey.exe`
- **Vị trí dự phòng:** `%LocalAppData%\NextKey\config.toml` (khi không có quyền ghi vào thư mục cài đặt, ví dụ khi nằm ở ổ C)

Bạn có thể sao lưu file này để backup cài đặt, cũng như chia sẻ cài đặt với người khác.

### ❓ NextKey có hỗ trợ kiểu gõ nào?

- VNI
- Telex
- Simple Telex 1/2

### ❓ Tôi muốn tắt NextKey cho một ứng dụng cụ thể?

Thêm ứng dụng vào danh sách **Excluded Apps** trong Settings.

---

*Cập nhật lần cuối: 2026-01-13*
