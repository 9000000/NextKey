# TASK 13: Migrate Convert Tool Dialog to Sciter

## Overview
Dialog "Công cụ chuyển mã" (Convert Tool) vẫn đang sử dụng Win32 UI cũ. Cần migrate sang Sciter để đồng bộ với các dialog khác (Settings, Macro, Excluded Apps, Special Apps, About).

## Current State
- **File**: `ConvertToolDialog.cpp` + `OpenKey.rc` (IDD_DIALOG_CONVERT_TOOL)
- **Style**: Win32 native controls (checkboxes, comboboxes, edit, buttons)
- **Screenshot**: Docs/planning/convert_tool_dialog.png

## Features to Migrate
1. **Tùy chọn chung** (General options)
   - Chuyển sang chữ HOA
   - Chuyển sang chữ thường  
   - Loại bỏ dấu câu
   - Đặt chữ Hoa đầu câu
   - Đặt chữ Hoa Sau Mỗi Từ
   - Thông báo khi chuyển xong

2. **Lựa chọn**
   - Chuyển mã trong Clipboard

3. **Phím chuyển mã nhanh**
   - Ctrl/Alt/Win/Shift checkboxes + key input

4. **Bảng mã nguồn/đích**
   - Dropdown: Unicode, TCVN3, VNI Windows, etc.
   - Đảo button (swap)

5. **Actions**
   - Chuyển mã (Convert)
   - Đóng (Close)

## Priority
Low - Dialog này ít được sử dụng, có thể defer sau các task khác.

## Dependencies
- Sciter SDK integration (already done)
- Common CSS styles from settings.css
