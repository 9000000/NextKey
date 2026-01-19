# 🚀 NextKey v1.0.9

Phiên bản v1.0.9 tập trung vào việc hoàn thiện trải nghiệm gõ, sửa các lỗi tồn đọng và cải tiến thao tác người dùng.

## 🐛 Sửa lỗi (Bug Fixes)
- **Settings Hotfix:** Khắc phục lỗi nghiêm trọng khiến các thay đổi trong phần Cài đặt không được tải lại và áp dụng ngay lập tức.
- **Custom Icon Persistence:** Sửa lỗi màu icon trên khay hệ thống (Custom Color) bị reset về màu mặc định trong một số trường hợp.
- **Logic gõ tiếng Việt ("Uơ"):**
  - Khắc phục lỗi không gõ được chữ "uơ".
  - Chuẩn hóa luồng gõ: `uo` + `w` -> `uơ`; `uơ` + `w` -> `ươ`.
- **Macro với ký tự Shift:** Sửa lỗi tính năng gõ tắt không hoạt động với các từ khóa chứa ký tự đặc biệt cần giữ Shift (ví dụ: `{`, `:`...).

## ✨ Cải thiện (Enhancements)
- **Tạm tắt bộ gõ:** Thay đổi thao tác kích hoạt từ **"Giữ Alt"** sang **"Nhấn đúp Alt"** (Double-tap Alt). Thay đổi này giúp thao tác nhanh hơn và tránh xung đột với các phím tắt giữ Alt khác.

## 🌟 Tính năng mới (New Feature)
- **Hủy Macro nhanh bằng Esc:** Thêm khả năng hủy bung từ gõ tắt tức thì.
  - **Cách dùng:** Nhấn phím `Esc` trước khi gõ từ khóa macro để ngăn hệ thống tự động thay thế.
  - **Ví dụ:** Thiết lập `btw` -> `by the way`.
    - Gõ `btw` -> ra `by the way`.
    - Gõ `Esc` sau đó gõ `btw` -> ra `btw` (nguyên bản).
