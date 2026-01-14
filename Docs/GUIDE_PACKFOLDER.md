# Hướng dẫn sử dụng `packfolder.exe` trong môi trường WSL

File `packfolder.exe` là công cụ của Sciter dùng để đóng gói toàn bộ tài nguyên UI (HTML, CSS, script, ảnh) thành một file mã nguồn C++ (`resources.cpp`). File này được dùng để nhúng trực tiếp giao diện vào file thực thi (exe) khi build ở chế độ Release.

## Vấn đề thường gặp

Khi mã nguồn nằm trong Windows Subsystem for Linux (WSL), đường dẫn sẽ có dạng UNC path (ví dụ: `\\wsl.localhost\Ubuntu-24.04\...`).

Command Prompt (CMD) của Windows **không hỗ trợ** việc đặt đường dẫn mạng (UNC path) làm thư mục làm việc hiện tại (current directory) khi khởi động hoặc dùng lệnh `cd`. Nó sẽ tự động quay về `C:\Windows` và báo lỗi:
> 'packfolder.exe' is not recognized as an internal or external command...

## Cách thực hiện đúng

Để khắc phục, hãy sử dụng lệnh `pushd`. Lệnh này sẽ tạo một ổ đĩa ảo tạm thời trỏ đến đường dẫn mạng và chuyển thư mục làm việc đến đó.

### Bước 1: Mở CMD và chuyển đến thư mục chứa packfolder

Chạy lệnh sau trong CMD:

```cmd
pushd \\path-to-your-project\OpenKey\Resources\Sciter
```

### Bước 2: Chạy lệnh đóng gói

Sau khi đã vào đúng thư mục, chạy lệnh sau để đóng gói tài nguyên:

```cmd
packfolder.exe . resources.cpp -v "resources"
```

**Giải thích tham số:**
*   `.`: Đóng gói toàn bộ thư mục hiện tại.
*   `resources.cpp`: Tên file đầu ra sẽ được tạo.
*   `-v "resources"`: Tên biến mảng byte trong C++ sẽ là `resources` (để khớp với code trong `SciterArchive.cpp`).

### Bước 3: Hoàn tất

Sau khi chạy xong, file `resources.cpp` sẽ được cập nhật/tạo mới. Bạn có thể đóng cửa sổ CMD hoặc gõ `popd` để quay lại thư mục trước đó.
