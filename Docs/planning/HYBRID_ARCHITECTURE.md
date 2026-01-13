# NextKey Hybrid Architecture Migration

## Tổng Quan

Migrate từ **Registry-centric** sang **TOML + RAM** architecture.

| Layer | Hiện tại | Đề xuất |
|-------|----------|---------|
| Settings | Registry (`APP_GET_DATA`) | `config.toml` → RAM |
| Runtime State | Registry (mỗi toggle) | RAM only |
| IPC | `PostMessage(HWND_BROADCAST)` | SharedMemory + Events |
| Auto-start | Registry Run key | Giữ nguyên |

---

## Danh Sách Registry Keys

### Nhóm 1: Config Tĩnh (Lưu vào config.toml)
*User thay đổi qua Settings → lưu liền*

| Key | Mô tả | Default |
|-----|-------|---------|
| `vInputType` | Kiểu gõ (0=Telex, 1=VNI...) | 0 |
| `vCodeTable` | Bảng mã (0=Unicode...) | 0 |
| `vSwitchKeyStatus` | Phím tắt chuyển V/E | 0x7A000206 |
| `vCheckSpelling` | Kiểm tra chính tả | 1 |
| `vRestoreIfWrongSpelling` | Khôi phục từ sai | 1 |
| `vUseModernOrthography` | Đặt dấu oà, uý | 0 |
| `vFixRecommendBrowser` | Sửa lỗi gợi ý | 1 |
| `vUpperCaseFirstChar` | Viết hoa đầu câu | 0 |
| `vAllowConsonantZFWJ` | Cho phép z,w,j,f | 0 |
| `vQuickTelex` | Quick Telex | 0 |
| `vQuickStartConsonant` | Quick phụ âm đầu | 0 |
| `vQuickEndConsonant` | Quick phụ âm cuối | 0 |
| `vUseMacro` | Bật gõ tắt | 1 |
| `vUseMacroInEnglishMode` | Macro trong Eng | 0 |
| `vAutoCapsMacro` | Tự viết hoa macro | 0 |
| `vUseSmartSwitchKey` | Smart Switch | 1 |
| `vRememberCode` | Nhớ bảng mã/app | 1 |
| `vSendKeyStepByStep` | Gửi phím clipboard | 1 |
| `vFixChromiumBrowser` | Fix Chromium | 0 |
| `vSupportMetroApp` | Hỗ trợ Metro | 0 |
| `vExcludeApps` | Bật loại trừ app | 0 |
| `vUseGrayIcon` | Icon xám | 0 |
| `vShowOnStartUp` | Mở Settings startup | 0 |
| `vRunWithWindows` | Chạy với Windows | 0 |
| `vRunAsAdmin` | Chạy admin | 0 |
| `vCheckNewVersion` | Tự kiểm tra update | 0 |
| `vTempOffSpelling` | Tạm tắt chính tả bằng Ctrl | 0 |
| `vTempOffOpenKey` | Tạm tắt OpenKey bằng Alt | 0 |
| `vEnablePerfLog` | Debug performance log | 0 |
| **Convert Tool:** | | |
| `convertToolHotKey` | Hotkey convert | 0 |
| `convertToolFromCode` | Mã nguồn | 0 |
| `convertToolToCode` | Mã đích | 0 |
| `convertToolToAllCaps` | Tất cả hoa | 0 |
| `convertToolToAllNonCaps` | Tất cả thường | 0 |
| `convertToolRemoveMark` | Bỏ dấu | 0 |
| `convertToolToCapsEachWord` | Hoa đầu từ | 0 |
| `convertToolToCapsFirstLetter` | Hoa đầu câu | 0 |
| `convertToolDontAlertWhenCompleted` | Không thông báo | 0 |
| **Binary/Array:** | | |
| `macroData` | Dữ liệu macro | binary |
| `vQtElectronApps` | App Qt/Electron | CSV |
| `vSkipImeCheckApps` | App skip IME | CSV |
| `englishOnlyApps` | App chỉ English | binary |
| `vDeletedDefaultApps` | Default đã xóa | CSV |

### Nhóm 2: Runtime State (Chỉ RAM, lưu khi shutdown)
*Thay đổi runtime qua hotkey/toggle, KHÔNG ghi disk liền*

| Key | Mô tả | Lưu khi shutdown? |
|-----|-------|-------------------|
| `vLanguage` | Trạng thái V/E hiện tại | ❌ Không (user toggle lại) |
| `smartSwitchKey` | V/E per app | ✅ Có |

### Nhóm 3: Lazy Save (RAM, lưu khi dialog đóng)
*User chỉnh nhiều lần (slider/color picker) → chỉ save 1 lần khi xong*

| Key | Mô tả | Trigger save |
|-----|-------|--------------|
| `vBackgroundOpacity` | Độ trong suốt UI (slider) | Dialog close/OK |
| `vTrayIconColorV` | Màu icon Vietnamese | Dialog close/OK |
| `vTrayIconColorE` | Màu icon English | Dialog close/OK |

> **Pattern**: Khi user kéo slider hoặc chọn màu → cập nhật RAM + preview UI. Save vào config.toml chỉ khi bấm OK hoặc đóng dialog.

## Flow Hoạt Động

```
KHỞI ĐỘNG
│
├─ Check config.bin cache? ──YES──→ Memory-map (~0.5ms)
│                           │
│                           NO
│                           ↓
├─ Parse TOML essentials (~2ms)
│
├─ Init hooks ngay
│
└─ Background thread: load macros, smart switch data
```

```
RUNTIME
│
├─ Toggle V/E → cập nhật RAM (<0.001ms)
├─ Gõ phím → đọc settings từ RAM
└─ KHÔNG ghi disk
```

```
TẮT APP
│
└─ Save SmartSwitch + runtime state → config.toml
```

---

## So Sánh Hiệu Năng

| Metric | Registry | TOML + RAM |
|--------|----------|------------|
| Toggle V/E | 2-5ms (có thể 50ms+) | <0.001ms |
| Startup (cold) | ~0.8ms | ~3-5ms |
| Startup (warm/cached) | - | ~0.5-1ms |
| Disk writes/ngày | Hàng trăm | <10 |

**Kết luận**: Warm start nhanh hơn hoặc bằng Registry, runtime nhanh hơn 1000x.

---

## Startup Optimization

### Strategy: Lazy Loading + Binary Cache

1. **Essential settings** (language, input type, spelling) → load sync
2. **Macros, SmartSwitch** → load async background thread
3. **Binary cache** (`config.bin`) → skip TOML parsing nếu có

### Implementation
```cpp
void OpenKeyInit() {
    if (ConfigManager::hasBinaryCache()) {
        ConfigManager::loadFromCache();  // ~0.5ms
    } else {
        ConfigManager::loadEssentials(); // ~2ms
    }
    
    initHooks();  // Hooks hoạt động ngay
    
    std::thread([]{ 
        ConfigManager::loadDeferred();   // macros, smart switch
        ConfigManager::updateCacheIfNeeded();
    }).detach();
}
```

---

## Cấu Trúc config.toml

```toml
[general]
language = 1
input_type = 0
code_table = 0
switch_key = 0x7A000206

[options]
check_spelling = true
restore_wrong_spelling = true
fix_recommend_browser = true

[macro]
enabled = true

[[macros]]
key = "addr"
value = "123 Đường ABC"

[excluded_apps]
enabled = false
list = ["game.exe"]

[special_apps]
qt_electron = ["code.exe"]
skip_ime = ["powerpnt.exe"]

[smart_switch]
"chrome.exe" = 1
"notepad.exe" = 0
```

---

## Files Cần Thay Đổi

### Tạo mới
| File | Mục đích |
|------|----------|
| `lib/tomlplusplus/toml.hpp` | TOML parser (header-only) |
| `ConfigManager.h/.cpp` | Quản lý config.toml |
| `SharedState.h/.cpp` | IPC via shared memory |

### Chỉnh sửa
| File | Thay đổi |
|------|----------|
| `OpenKeyHelper.cpp` | Sửa `migrateFromOldRegistry()` → export TOML |
| `OpenKey.cpp` | Thay `APP_GET_DATA` → ConfigManager |
| `stdafx.h` | Thêm `CONFIG_GET/SET` macros |
| `AppDelegate.cpp` | Dùng SharedState thay PostMessage |

---

## Thứ Tự Thực Hiện

- [x] **Phase 1**: Thêm toml++, tạo ConfigManager (Done)
- [x] **Phase 2**: Migrate `OpenKeyInit()` sang ConfigManager (Done)
- [x] **Phase 3**: Implement SharedState cho IPC (Done - check `SharedState.cpp`)
- [x] **Phase 4**: Update subprocess dialogs (Done - all dialogs use ConfigManager)
- [x] **Phase 5**: Cleanup Registry code, test portable mode (Done - `OpenKeyHelper` cleaned)

---

## Rủi Ro & Mitigation

| Rủi ro | Giải pháp |
|--------|-----------|
| Crash mất V/E state | User toggle lại (accepted) |
| File lock conflict | Retry với delay |
| Config corruption | Atomic write (tmp → rename) |
| Multi-instance conflict | Main process owns file |

---

## Dead Code Cleanup

### Biến không dùng
| Biến/Key | Vị trí | Lý do xóa |
|----------|--------|----------|
| `vTrayIconFontName` | AppDelegate.cpp, OpenKey.cpp, stdafx.h | Load từ Registry nhưng không dùng, UI không có chỗ chỉnh |

### Legacy Dialog Files (đã chuyển sang Sciter)
*Các file Win32 dialog cũ không còn được include, có thể xóa khỏi project*

| File | Sciter Replacement | Action |
|------|-------------------|--------|
| `MainControlDialog.h/.cpp` | SettingsDialog (Sciter) | Xóa khỏi vcxproj |
| `MacroDialog.h/.cpp` | MacroDialogSciter | Xóa khỏi vcxproj |
| `ExcludedAppsDialog.h/.cpp` | ExcludedAppsDialogSciter | Xóa khỏi vcxproj |
| `ConvertToolDialog.h/.cpp` | ConvertToolDialogSciter | Xóa khỏi vcxproj |
| `OpenKeySettingsController.h/.cpp` | SettingsDialog (Sciter) | Xóa khỏi vcxproj |
| `OpenKeyManager.h/.cpp` | Không dùng | Review xem còn dùng không |

> **Lưu ý**: Backup trước khi xóa. Test build sau mỗi lần xóa file.
