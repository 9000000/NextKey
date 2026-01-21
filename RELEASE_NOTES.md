# 🚀 NextKey v1.1.0

Phiên bản v1.1.0 tập trung vào việc tối ưu hóa độ ổn định của quá trình cập nhật và tinh chỉnh bộ xử lý tiếng Việt thông minh hơn.

## 🐛 Sửa lỗi (Bug Fixes)
- **Tray Icon Persistence:** Khắc phục lỗi biểu tượng trên khay hệ thống (Tray Icon) không hiển thị lại sau khi ứng dụng tự động khởi động lại sau bản cập nhật.
- **General Fixes:** Sửa một số lỗi nhỏ khác.

## ✨ Cải thiện (Enhancements)
- **Kiểm tra chính tả thông minh (Smart Spell Check):**
  - Cải thiện thuật toán để nhận diện tốt hơn các từ tiếng Anh phổ biến. Từ nay khi gõ `year`, `your`... hệ thống sẽ giữ nguyên thay vì bỏ dấu sai thành `yeả`, `yỏu`.
  - Vẫn hỗ trợ linh hoạt các trường hợp bỏ dấu tự do (Free-style) như `yủn` cho những người dùng có thói quen gõ phá cách.
- **Tối ưu Engine tiếng Việt (Engine Optimization):**
  - Sửa lỗi gõ các từ như `cungx` thỉnh thoảng không ra `cũng` như mong đợi.
- **Tối ưu hóa hiệu năng:**
  - Giải phóng bộ nhớ đệm (Config AST) ngay sau khi khởi động, giúp giảm mức tiêu thụ RAM của ứng dụng.
