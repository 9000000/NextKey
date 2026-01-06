# PHTV Feature Analysis - So sánh với OpenKey

## Ngày phân tích: 2026-01-06
## Repo: https://github.com/PhamHungTien/PHTV

---

## Tóm tắt

Repo PHTV là bộ gõ tiếng Việt cho macOS, sử dụng engine C++ từ OpenKey. Phân tích này so sánh các tính năng để tìm những điểm có thể học hỏi.

---

## Tính năng OpenKey đã có

Các tính năng sau đã có trong OpenKey, không cần implement thêm:

- Telex, VNI, Simple Telex
- Unicode, TCVN3, VNI Windows, Unicode Compound
- Quick Telex (cc→ch, gg→gi, kk→kh, ...)
- Phụ âm đầu/cuối nhanh (f→ph, j→gi, w→qu)
- Macro/Text Snippets + Import/Export
- Spell checking
- Excluded Apps
- Smart Switch (nhớ ngôn ngữ theo app)
- ESC khôi phục ký tự gốc
- Tương thích Dvorak/Colemak (macOS)
- Hot reload
- Auto update
- Auto-capitalize first letter
- Debug log

---

## Tính năng OpenKey chưa có (đáng học hỏi)

| # | Tính năng | Mô tả | Độ khó | Usefulness |
|---|-----------|-------|--------|------------|
| 1 | **Auto-restore English words** | Nhận diện từ tiếng Anh khi gõ nhầm (VD: "tẻminal" → "terminal") | ⭐⭐⭐ | ⭐⭐ |
| 2 | **Import/Export Settings** | Sao lưu toàn bộ cài đặt ra file | ⭐⭐ | ⭐⭐⭐ |
| 3 | **Macro Categories** | Tổ chức macro theo nhóm với icon/màu sắc | ⭐⭐ | ⭐ |
| 4 | **Non-Latin keyboard detection** | Tự động chuyển về English khi dùng bàn phím Nhật/Trung/Hàn | ⭐ | ⭐ |

---

## Debug: Tính năng có nhưng cần kiểm tra

### 1. "Tự khôi phục phím với từ sai" (vRestoreIfWrongSpelling)

**Trạng thái:** ✅ Có code, ĐANG HOẠT ĐỘNG

**Logic trong Engine.cpp:**
- Dòng 1244-1261: Hàm `checkRestoreIfWrongSpelling()` - copy `KeyStates[]` về `TypingWord[]`
- Dòng 1397-1404: Xử lý khi gặp word break (space, enter, ...)
- Dòng 1457-1461: Xử lý khi nhấn space

**Cách hoạt động:**
1. Khi user gõ từ SAI chính tả và nhấn Space/Enter
2. Engine kiểm tra nếu `tempDisableKey = true` (từ đã bị disable do sai)
3. Nếu có ký tự có dấu/tone bị disable, restore về trạng thái gốc (`KeyStates[]`)
4. Output sẽ là từ gốc (không có dấu tiếng Việt)

**Ví dụ:** Gõ "asdf" rồi nhấn "s" (để đặt dấu sắc) → không hợp lệ → nhấn Space → restore về "asdfs"

---

### 2. "Tạm tắt OpenKey bằng Alt" (vTempOffOpenKey)

**Trạng thái:** ✅ Có code, ĐANG HOẠT ĐỘNG (nhưng UX khác PHTV)

**Logic trong OpenKey.cpp (Windows):**
- Dòng 910-912: Trigger khi **release Alt key**
```cpp
if (vTempOffOpenKey && !_hasJustUsedHotKey && _lastFlag & MASK_ALT) {
    vTempOffEngine();
}
```

**Logic trong Engine.cpp:**
- Dòng 1273-1275: `vTempOffEngine()` chỉ set flag `_willTempOffEngine = true`
- Dòng 1545-1548: Khi flag = true, skip xử lý Vietnamese
```cpp
if (_willTempOffEngine) {
    hCode = vDoNothing;
    hExt = 3;
    return;
}
```

**Cách hoạt động hiện tại:**
1. Nhấn và thả phím Alt (không kết hợp với phím khác)
2. `_willTempOffEngine = true`
3. Các ký tự tiếp theo sẽ bypass Vietnamese engine
4. Reset khi gặp word break (space, click chuột, ...)

**So sánh với PHTV:**
- **PHTV:** **GIỮ** phím để tạm disable, thả ra để enable lại (real-time)
- **OpenKey:** **Nhấn-thả** Alt để toggle temp disable, reset khi word break

> [!NOTE]
> Đây là design decision, không phải bug. Có thể consider implement "hold" pattern như PHTV trong tương lai.

---

## Recommendations

### Ưu tiên cao
1. **Import/Export Settings** - Người dùng hay hỏi về backup settings

### Ưu tiên trung bình  
2. **Auto-restore English words** - Cần từ điển English, phức tạp hơn

### Xem xét thêm
3. **Hold key pattern** - Thay đổi UX của vTempOffOpenKey từ "toggle" thành "hold"

---

## Files liên quan

- `Sources/OpenKey/engine/Engine.cpp` - Core engine logic
- `Sources/OpenKey/engine/Engine.h` - Declarations
- `Sources/OpenKey/win32/OpenKey/OpenKey/OpenKey.cpp` - Windows hook
- `Sources/OpenKey/win32/OpenKey/OpenKey/SettingsDialog.cpp` - UI settings
