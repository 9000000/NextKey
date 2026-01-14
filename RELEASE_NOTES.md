# 🚀 NeXTKey v1.0.5 RC1 - Rebrand & Config Overhaul

Phiên bản v1.0.5 RC1 mang đến diện mạo mới đồng bộ hơn, chuyển đổi kiến trúc lưu trữ cấu hình giúp hệ thống linh hoạt và nhanh hơn, cùng nhiều cải tiến về trải nghiệm người dùng.

## ⚠️ Lưu ý quan trọng khi cập nhật
*   Đây là phiên bản **Rebrand** từ OpenKey -> NextKey.
*   **Người dùng cũ (v1.0.4 trờ xuống):** Nếu cập nhật tự động gặp lỗi, vui lòng tải thủ công file `NextKey-x64.zip` hoặc `NextKey-x86.zip` từ trang Releases.

*   Hệ thống sẽ tự động migrate cấu hình cũ từ Registry sang `config.toml`.
## 🎨 Giao diện & Trải nghiệm (UI/UX)

*   **Rebrand & Đồng bộ:** Cập nhật Icon mới, điều chỉnh lại text cho đồng bộ, migrate giao diện các dialog còn thiếu.
*   **Công cụ chuyển mã (Convert Tool):**
    *   Đồng bộ UI với giao diện chính.
    *   **Mới:** Thêm tính năng chuyển mã cho file (`.txt` hoặc `.rtf`).
*   **Ghim cửa sổ (Pin Window):** Thêm nút ghim giúp cửa sổ giao diện luôn nổi trên các ứng dụng khác.
*   **Sửa lỗi UI:** Fix lỗi giao diện bị tràn khi dùng màu icon tùy chỉnh.

## ⚡ Hiệu năng & Kiến trúc

*   **Config File (`config.toml`):** Thay đổi cơ chế lưu setting từ Registry sang file `config.toml` nằm ngay trong thư mục file thực thi.
    *   Giúp dễ dàng Backup/Import setting hơn.
    *   Tăng tốc hệ thống và giúp ứng dụng hoạt động độc lập (Portable) tốt hơn.
*   **Tối ưu hóa:** Tối ưu build giúp giảm kích thước file thực thi.

## ⌨️ Tính năng gõ & Tương thích

*   **Clipboard Injection:** Thêm method dùng clipboard để gõ tiếng Việt.
    *   Cho phép config riêng cho từng app, thuận tiện để sử dụng song song các method gõ.
    *   Mặc định sẽ dùng method gửi từng ký tự (SendInput).
    *   ([Xem thêm trong FAQ](Docs/FAQ.md)).
*   **Debug Log:** Thêm debug log cho các case gõ tiếng Việt bị lỗi để dễ dàng chẩn đoán.

## 🔮 Planning (Dự kiến phát triển)

*Các tính năng sẽ phát triển nếu được cộng đồng ủng hộ:*

*   **Per-App Typing Style:** Setup kiểu gõ riêng cho từng apps (Ví dụ: App A dùng Telex, App B dùng VNI).
*   **Smart Correction:**
    *   Chuyển đổi tính năng "Kiểm tra chính tả" -> **Tự sửa lỗi tiếng Việt**.
    *   Chuyển đổi tính năng "Tự khôi phục với từ sai" -> **Tự sửa lỗi tiếng Anh**.

---
*Cảm ơn bạn đã sử dụng NeXTKey!*
