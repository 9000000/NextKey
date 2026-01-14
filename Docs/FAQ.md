# NextKey FAQ - Câu hỏi thường gặp

## Mục lục

1. [Vấn đề với Notepad Windows 11](#vấn-đề-với-notepad-windows-11)
2. [Vấn đề với PowerPoint](#vấn-đề-với-powerpoint)
3. [Lag khi chuyển ứng dụng](#lag-khi-chuyển-ứng-dụng)
4. [Tại sao có nhiều phương thức gửi ký tự?](#tại-sao-có-nhiều-phương-thức-gửi-ký-tự)

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

## Tại sao có nhiều phương thức gửi ký tự?

### ❓ Tại sao NextKey cung cấp nhiều phương thức (Shift+Insert, Ctrl+V, SendInputKey)?

**Tâm sự về EVKey và sự đa dạng của Windows:**

EVKey - tượng đài app gõ tiếng Việt hiện tại - có độ tương thích đáng kinh ngạc với hầu hết các ứng dụng Windows. Đây là kết quả của nhiều năm phát triển, cộng đồng người dùng lớn, và rất nhiều bug reports từ thực tế sử dụng.

Mình suy đoán EVKey có thể đã:
- Thu thập data từ hàng triệu người dùng (dựa trên bug reports)
- A/B test nhiều phương thức khác nhau
- Fine-tune timing và delays cho từng loại ứng dụng
- Có cơ sở dữ liệu "fingerprint" của các ứng dụng phổ biến

**Thực tế của NextKey:**

NextKey là project phát triển cá nhân, không có:
- Telemetry để thu thập data người dùng
- Cộng đồng đủ lớn để có đủ mẫu thử nghiệm
- Resources để test trên hàng nghìn ứng dụng khác nhau

**Quyết định trao quyền cho user:**

Thay vì cố gắng "đoán" phương thức tối ưu (và thường xuyên đoán sai), NextKey chọn giải pháp khác: **trao quyền cho người dùng tự cấu hình**.

| Phương thức | Mô tả | Phù hợp với |
|-------------|-------|-------------|
| **Shift+Insert** | Clipboard paste cổ điển | Đa số ứng dụng |
| **Ctrl+V** | Clipboard paste phổ biến | Một số app không nhận Shift+Insert |
| **SendInputKey** | Gửi từng phím + 12ms delay | Games, apps có input queue riêng |

### 🔍 Cách tự chẩn đoán và chọn phương thức phù hợp

> [!IMPORTANT]
> **Mặc định của NextKey**: SendInput key-by-key (TẮT "Use Clipboard")
> 
> Các phương thức dưới đây chỉ áp dụng khi bạn **BẬT "Use Clipboard"** trong Settings.

**Triệu chứng 1: Mất ký tự (character dropping) khi dùng Clipboard**
```
Gõ: "xin chào"
Hiển thị: "xin cho" hoặc "xn chào"
```
→ **Nguyên nhân**: App xử lý paste quá chậm
→ **Giải pháp**: Thử `Ctrl+V` hoặc TẮT "Use Clipboard" (về mặc định SendInput)

**Triệu chứng 2: Ký tự bị duplicate khi dùng Clipboard**
```
Gõ: "xin chào"  
Hiển thị: "xxin cchhào"
```
→ **Nguyên nhân**: App nhận được cả key event VÀ clipboard paste
→ **Giải pháp**: Thử `Ctrl+V` thay vì `Shift+Insert`

### 📋 Khi nào nên BẬT "Use Clipboard"?

| Trường hợp | Nên dùng | Lý do |
|------------|----------|-------|
| **Đa số ứng dụng** | SendInput (mặc định, TẮT clipboard) | Nhanh, ổn định |
| **App bị lag với SendInput** | BẬT Clipboard + Shift+Insert | Paste nhanh hơn gõ từng phím |
| **App không nhận Shift+Insert** | BẬT Clipboard + Ctrl+V | Một số app chỉ listen Ctrl+V |

> [!TIP]  
> **Cách test nhanh:** Gõ một câu dài như "Tôi đang test NextKey với ứng dụng này" 5 lần liên tiếp. Nếu không có lỗi → config hiện tại OK cho app này.

### ⏱️ Cài đặt Delay (ms)

Khi thêm app vào **Cấu hình Clipboard**, bạn có thể set thêm **Delay** (0-500ms):

| Delay | Khi nào dùng |
|-------|--------------|
| **0ms** | Mặc định, đa số app |
| **10-20ms** | App xử lý chậm (games, heavy apps) |
| **50-100ms** | App có animation hoặc transition khi nhận input |

**Cách hoạt động:**
- Delay được chèn SAU khi paste xong
- Giúp app có thời gian xử lý trước khi NextKey gửi tiếp ký tự mới
- Giá trị 12ms ≈ 1 frame ở 60fps (mắt người không thể nhận ra)

**Khi nào nên thử thêm Delay?**

| Triệu chứng | Nguyên nhân | Thử Delay |
|-------------|-------------|-----------|
| Mất ký tự **cuối** từ (vd: "xin chào" → "xin chà") | App chưa xử lý xong đã nhận tiếp ký tự | 10-20ms |
| Mất ký tự **ngẫu nhiên** giữa từ | Race condition trong input buffer | 15-30ms |
| Ký tự bị **đảo thứ tự** (vd: "abc" → "acb") | Input events đến không theo order | 20-50ms |
| Lag **chỉ khi gõ nhanh** | App không kịp process batch lớn | 10-15ms |

> [!NOTE]
> Nếu một ứng dụng bị mất ký tự hoặc duplicate, hãy thử đổi phương thức trong **Cấu hình Clipboard** (Settings → System tab).

**Triết lý:**

*"Không có phương thức nào là tối ưu cho tất cả. Nhưng user biết app của mình - họ có thể tìm ra phương thức phù hợp nhất."*

---

## Các câu hỏi khác

### ❓ Làm sao để backup cài đặt?

NextKey đang lưu cài đặt trong file config.toml nằm cùng thư mục với file thực thi(trong trường hợp nằm ở ổ C hoặc không có quyền truy cập vào thư mục NextKey lưu file config.toml ỏ %APPDATA%). Bạn có thể sao lưu file này để backup cài đặt, cũng như chia sẻ cài đặt với người khác.

### ❓ NextKey có hỗ trợ kiểu gõ nào?

- VNI
- Telex
- Simple Telex 1/2

### ❓ Tôi muốn tắt NextKey cho một ứng dụng cụ thể?

Thêm ứng dụng vào danh sách **Excluded Apps** trong Settings.

---

*Cập nhật lần cuối: 2026-01-13*
