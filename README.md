# NextKey - Modern Vietnamese IME for Windows

[![Build Status](https://img.shields.io/github/actions/workflow/status/phatMT97/NextKey/msbuild.yml?label=Build&logo=github)](https://github.com/phatMT97/NextKey/actions)
[![License](https://img.shields.io/github/license/phatMT97/NextKey)](LICENSE)
[![Release](https://img.shields.io/github/v/release/phatMT97/NextKey)](https://github.com/phatMT97/NextKey/releases)

<p align="center">
  <img src="Docs/images/nextkey-compact.png" alt="NextKey Compact View" width="250">
  <img src="Docs/images/nextkey-expanded.png" alt="NextKey Expanded View" width="550">
</p>
<p align="center"><em>NextKey Interface: Compact Mode (Left) and Expanded Settings (Right)</em></p>

<p align="center">
  <img src="Docs/images/nextkey-full-UI.png" alt="NextKey Full UI" width="800">
</p>
<p align="center"><em>NextKey Full UI</em></p>

---

## EN - About NextKey
**NextKey** is an open-source Vietnamese Input Method Editor (IME) for Windows, forked and improved from [OpenKey](https://github.com/tuyenvm/OpenKey). It focuses on performance, modern UI, and privacy.

<a name="privacy-policy"></a>
### 🔒 Privacy Policy
We take your privacy seriously. **NextKey is purely an Input Method Editor:**
* **No Keylogging:** We do not collect, store, or transmit your keystrokes.
* **No Data Collection:** No personal data is sent to any server.
* **Offline First:** The software operates entirely locally on your machine.
* **Open Source:** You can verify this behavior by reviewing our source code.

---

## VI - Về NextKey (Tiếng Việt)

**NextKey** là bộ gõ tiếng Việt mã nguồn mở, được phát triển tiếp nối từ dự án OpenKey. Sau thời gian dài tối ưu, NextKey mang đến trải nghiệm gõ phím mượt mà với giao diện hiện đại:

- 🎨 **Giao diện Glassmorphism** - Thiết kế trong suốt, hiện đại kiểu Windows 11.
- ⚡ **Hiệu năng cao** - Tối ưu sâu, nhẹ hơn, không gây lag máy.
- 🛡️ **Tính năng mạnh mẽ** - Loại trừ ứng dụng (Game mode), tự động chuyển Anh/Việt.
- 🐛 **Sửa lỗi tồn đọng** - Fix lỗi gõ trong PowerPoint, Lock Screen, ứng dụng Qt/Electron.

> **Ghi nhận (Credits):** NextKey được xây dựng trên nền tảng OpenKey. Xin cảm ơn tác giả Mai Vũ Tuyên và cộng đồng.

---

## ✨ Điểm nổi bật (Highlights)

### 🎨 Giao diện Hiện đại (Modern UI)
- **Glassmorphism Design**: Hiệu ứng kính mờ tinh tế.
- **Toggle Switches**: Công tắc gạt mượt mà kiểu iOS.
- **Phân loại rõ ràng**: Tab Bộ gõ, Gõ tắt, Hệ thống, Thông tin riêng biệt.
- **Sciter Engine**: Sử dụng HTML/CSS rendering engine giúp giao diện đẹp nhưng cực nhẹ.

### 🛡️ Chế độ Game/Code (English Only Mode)
Tính năng "sống còn" cho Dev và Gamer:
- Lập danh sách ứng dụng "loại trừ" (VSCode, Terminal, CS:GO...).
- Tự động chuyển sang chế độ tiếng Anh khi vào các app này.
- Tránh việc gõ nhầm tiếng Việt khi đang code hoặc combat.

### ⚡ Tối ưu hiệu năng & Sửa lỗi
| Vấn đề | Trạng thái | Chi tiết |
|--------|-----------|----------|
| **Lock Screen** | ✅ Fixed | Sửa lỗi mất phím tắt sau khi khóa máy [Xem chi tiết](Docs/LOCK_SCREEN_FIX.md) |
| **Electron Apps** | ✅ Fixed | Gõ mượt trên Discord, VSCode, Notion... |
| **CPU Usage** | ⚡ Optimized | Giảm tải CPU khi ở chế độ chờ |

### 🎯 Hỗ trợ màn hình độ phân giải cao (High DPI)
- Icon sắc nét từ 16px đến 256px.
- Tự động scale giao diện theo độ phân giải màn hình Windows.

---

## ⌨️ Tính năng gõ tiếng Việt

- **Kiểu gõ**: Telex, VNI, Simple Telex.
- **Bảng mã**: Unicode, TCVN3, VNI Windows...
- **Gõ tắt (Macro)**: Cho phép định nghĩa từ viết tắt không giới hạn.
- **Kiểm tra chính tả**: Tự động phát hiện lỗi sai cơ bản.
- **Smart Switch**: Ghi nhớ trạng thái gõ (Anh/Việt) cho từng ứng dụng riêng biệt.

---

## ❓ FAQ & Troubleshooting

Gặp vấn đề khi sử dụng? Kiểm tra ngay **[FAQ - Câu hỏi thường gặp](Docs/FAQ.md)** để tìm giải pháp cho:
- Lỗi gõ trong Notepad, Game.
- Cấu hình tương thích ứng dụng.
- Và nhiều vấn đề khác...

---

## 📥 Cài đặt (Installation)

1. Tải về phiên bản mới nhất tại mục **[Releases](https://github.com/phatMT97/NextKey/releases)**.
2. Giải nén và chạy file `NextKey.exe`.
3. *(Khuyến nghị)* Tắt các bộ gõ khác (Unikey, EVKey) để tránh xung đột.

---

## 📜 License (Giấy phép)

Dự án này được phân phối dưới giấy phép **GNU General Public License v3.0 (GPL-3.0)**.
Xem file [LICENSE](LICENSE) để biết thêm chi tiết.

This project is licensed under the **GNU General Public License v3.0**.
Based on OpenKey by Mai Vu Tuyen.
