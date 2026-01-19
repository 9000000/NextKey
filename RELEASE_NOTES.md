# 🚀 NeXTKey v1.0.8 - Auto-Detect & Performance Boost

Phiên bản này mang đến tính năng **Tự động phát hiện ứng dụng** thông minh, cải thiện hiệu suất gõ và sửa các lỗi quan trọng.

## ✨ Tính năng mới (Feature)
- **Tự động phát hiện ứng dụng (Smart App Detection):** NextKey giờ đây tự động nhận diện loại ứng dụng (Electron, Qt, Game...) để áp dụng phương pháp gõ tối ưu nhất. Bạn không cần phải cấu hình thủ công cho từng app nữa!

## ⚡ Cải thiện (Enhancement)
- **Gộp Dialog Cấu hình:** Hợp nhất "Special Apps" và "Clipboard Apps" thành **"Cấu hình ứng dụng"** (Hệ thống -> Cấu hình ứng dụng). Do đã có cơ chế tự động, bạn chỉ cần dùng menu này khi muốn ép buộc setting thủ công cho các trường hợp đặc biệt.
- **Tối ưu Engine:** Cải thiện thuật toán xử lý phím, tăng tốc độ phản hồi tối đa giúp trải nghiệm gõ mượt mà hơn.
- **Build Optimization:** Điều chỉnh quy trình build pipeline giúp giảm thiểu báo động giả (false positive) từ Windows Defender.

## 🐛 Sửa lỗi (Bug Fixes)
- **Fix Reload Settings:** Sửa lỗi một số cài đặt không áp dụng ngay lập tức (real-time) mà phải khởi động lại app.
- **Fix Auto-Update:** Sửa lỗi cập nhật thất bại dù thông báo thành công. Thêm trạng thái hiển thị rõ ràng khi bấm "Kiểm tra ngay".

> [!CAUTION]
> **Lưu ý quan trọng:** Cấu hình thủ công cũ (Special Apps/Clipboard Apps) sẽ **không tự động chuyển đổi** sang hệ thống mới. Nếu bạn đang có cấu hình thủ công quan trọng, hãy ghi lại trước khi cập nhật. Tuy nhiên, với cơ chế Auto-Detect mới, hầu hết các cấu hình này có thể không còn cần thiết.

---

*Cảm ơn bạn đã lựa chọn NeXTKey!*
