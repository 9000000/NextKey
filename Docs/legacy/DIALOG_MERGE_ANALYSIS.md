# Phân tích gộp SpecialApps & ClipboardApps Dialog

> **Mục đích**: Tài liệu phân tích chi tiết để review với senior trước khi implement.  
> **Ngày**: 2026-01-19

---

## 1. Hiện trạng (Current State)

### 1.1 SpecialAppsDialogSciter

**Vị trí files**:
- `Resources/Sciter/specialapps/` (HTML, CSS, JS)
- `Sources/OpenKey/win32/OpenKey/OpenKey/SpecialAppsDialogSciter.cpp` (~823 lines)

**Mục đích**: Cấu hình typing behavior cho các ứng dụng đặc biệt.

**Data model**:
```cpp
enum class SpecialAppType {
    QtElectron = 0,    // Apps có lazy Input Context init (skip empty char)
    SkipImeCheck = 1   // MS Office apps (false IME detection)
};

struct SpecialAppEntry {
    std::string exeName;
    SpecialAppType type;
    bool isDefault;  // Built-in vs user-added
};
```

**Config storage** (`config.toml`):
```toml
[specialApps]
qtElectronApps = ["custom1.exe", "custom2.exe"]
skipImeCheckApps = ["custom3.exe"]
deletedDefaults = ["code.exe"]  # User đã xóa default này
```

**Default apps (hardcoded)**:
- Qt/Electron: `notepadnext.exe`, `code.exe`, `sublime_text.exe`, `atom.exe`, `discord.exe`, `slack.exe`
- Skip IME: `powerpnt.exe`, `winword.exe`, `excel.exe`

---

### 1.2 ClipboardAppsDialogSciter

**Vị trí files**:
- `Resources/Sciter/clipboardapps/` (HTML, CSS, JS)
- `Sources/OpenKey/win32/OpenKey/OpenKey/ClipboardAppsDialogSciter.cpp` (~730 lines)

**Mục đích**: Override phương thức paste clipboard cho từng app.

**Data model**:
```cpp
enum class ClipboardMethod {
    ShiftInsert = 0,
    CtrlV = 1,
    SendInputKey = 2  // For games
};

struct ClipboardAppEntry {
    std::string exeName;
    ClipboardMethod method;
    int delayMs;  // 0-500ms
};
```

**Config storage** (`config.toml`):
```toml
[clipboardApps]
apps = [
    { name = "firefox.exe", method = 1, delay = 0 },
    { name = "game.exe", method = 2, delay = 50 }
]
```

**Default apps**: Không có (chỉ user-added).

---

### 1.3 RuntimeProfile (Backend auto-detection)

**Vị trí**: `RuntimeProfile.h`, `RuntimeProfile.cpp`

**Mục đích**: Cache runtime behavior per-HWND, auto-detect framework và optimize hot path.

**Data model đã có**:
```cpp
struct RuntimeProfile {
    FrameworkType framework;   // Native, Qt, Electron (auto-detected via window class)
    ProfileType type;          // NativeRichTSF, QtElectronLike, BrowserLike, LegacyFallback
    ProfileFlags flags;        // SkipImeCheck, SkipEmptyChar, PreferClipboard
    
    int8_t injectionMethod;    // -1=default, 0=ShiftInsert, 1=CtrlV, 2=SendInputKey
    uint16_t delayMs;          // Clipboard delay
    uint8_t failureCount;      // Auto-downgrade after 3 clipboard failures
    
    // Latency probe (Milestone 2)
    uint16_t minLatencyMs;
    uint8_t probeCount;
    bool isProbeComplete;
};
```

> **Quan sát quan trọng**: RuntimeProfile **ĐÃ CÓ** tất cả fields cần thiết cho cả hai dialog!

---

## 2. Vấn đề (Problems)

### 2.1 Code Duplication (~80%)

| Component | SpecialApps | ClipboardApps |
|-----------|-------------|---------------|
| Window boilerplate | ✅ Giống | ✅ Giống |
| Acrylic effect | ✅ Giống | ✅ Giống |
| Window picker | ✅ Giống | ✅ Giống |
| Running apps dropdown | ✅ Giống | ✅ Giống |
| SubclassProc | ✅ Giống | ✅ Giống |
| Event handling pattern | ✅ Giống | ✅ Giống |
| **Unique logic** | ~100 lines | ~100 lines |

**Kết quả**: ~1553 lines code, trong đó ~1200 lines duplicate.

### 2.2 Conceptual Overlap

Một app có thể cần **CẢ HAI** loại config:

| App | Behavior Need | Clipboard Need |
|-----|---------------|----------------|
| `discord.exe` | Qt/Electron (skip empty char) | Có thể cần Ctrl+V |
| `powerpnt.exe` | Skip IME Check | Có thể cần delay |
| `game.exe` | Normal | SendInputKey + delay |

**Hiện tại**: User phải mở 2 dialog riêng để config 1 app.

### 2.3 Inconsistent with RuntimeProfile Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        RuntimeProfile                            │
│  ┌──────────────────────┐    ┌──────────────────────┐           │
│  │  Behavior Fields     │    │  Clipboard Fields    │           │
│  │  - framework         │    │  - injectionMethod   │           │
│  │  - type             │    │  - delayMs           │           │
│  │  - flags            │    │  - failureCount      │           │
│  └──────────────────────┘    └──────────────────────┘           │
└─────────────────────────────────────────────────────────────────┘
        ↑                              ↑
        │                              │
┌───────────────────┐        ┌───────────────────┐
│ SpecialAppsDialog │        │ ClipboardAppsDialog│
│ (Chỉ set behavior)│        │ (Chỉ set clipboard) │
└───────────────────┘        └───────────────────┘
```

**Vấn đề**: 2 UI riêng lẻ đang set các fields khác nhau của **cùng 1 struct backend**.

### 2.4 Delay Field - Giá trị thực tế thấp

**Phân tích field `delay`**:

| Aspect | Observation |
|--------|-------------|
| **Latency Probe đã có** | RuntimeProfile track `minLatencyMs` per-HWND |
| **Auto-calculate khả thi** | Nếu probe latency > 80ms → tự thêm delay 20-50ms |
| **User config burden** | User không biết nên set bao nhiêu delay |
| **Value range** | 0-500ms, nhưng thực tế chỉ 0-100ms có ý nghĩa |

**Kết luận**: Delay nên **auto-detect** dựa trên latency probe, không cần user config.

---

## 3. Phân tích các phương án

### Option A: Giữ nguyên 2 dialog riêng

**Pros**:
- Không cần refactor
- Simple mental model cho mỗi dialog

**Cons**:
- Code duplication không giảm
- User phải mở 2 dialog cho 1 app
- Inconsistent với kiến trúc RuntimeProfile

**Đánh giá**: ❌ Không khuyến khích

---

### Option B: Gộp thành 1 "App Profiles" dialog

**Proposed UI**:
```
┌─────────────────────────────────────────────────────────────────┐
│  Cấu hình ứng dụng                                         [X]  │
├─────────────────────────────────────────────────────────────────┤
│  App: [input + autocomplete]  [🎯 Pick Window]  [↻ Refresh]     │
│                                                                  │
│  ┌─ Behavior ────────────────────────────────────────────────┐  │
│  │  ○ Auto-detect (recommended)                              │  │
│  │  ○ Qt/Electron - skip empty char for lazy init apps      │  │
│  │  ○ Native Rich - skip IME check for MS Office            │  │
│  └───────────────────────────────────────────────────────────┘  │
│                                                                  │
│  ┌─ Clipboard ───────────────────────────────────────────────┐  │
│  │  Method: [Auto-detect ▼]  (Shift+Insert / Ctrl+V / Game)  │  │
│  └───────────────────────────────────────────────────────────┘  │
│                                                                  │
│  [+ Thêm]                                                        │
├─────────────────────────────────────────────────────────────────┤
│  App           │ Behavior        │ Clipboard      │             │
├────────────────┼─────────────────┼────────────────┼─────────────┤
│  discord.exe   │ Qt/Electron     │ Auto           │ [🗑]        │
│  powerpnt.exe  │ Native Rich ⚙️  │ Auto           │ [🗑]        │
│  firefox.exe   │ Auto            │ Ctrl+V         │ [🗑]        │
│  game.exe      │ Auto            │ Game Mode      │ [🗑]        │
└─────────────────────────────────────────────────────────────────┘
  Legend: ⚙️ = default (built-in)
```

**Pros**:
- 1 dialog cho toàn bộ per-app config
- Align với RuntimeProfile architecture
- Loại bỏ ~600 lines duplicate code
- "Auto-detect" làm default → ít config hơn

**Cons**:
- Cần refactor 2 dialogs thành 1
- Data migration (nhưng có thể làm transparent)

**Đánh giá**: ✅ Khuyến khích

---

### Option C: Gộp + Thêm "Advanced" section (expandable)

Mở rộng Option B với section ẩn cho power users:

```
┌─ Advanced (click to expand) ──────────────────────────────────┐
│  Delay override: [   ] ms  (0 = auto-detect from latency)     │
│  Force clipboard: ☐ Always use clipboard (bypass SendInput)   │
└───────────────────────────────────────────────────────────────┘
```

**Pros**:
- Giữ UI đơn giản cho 99% users
- Power users vẫn có control

**Cons**:
- Phức tạp hơn Option B
- Delay override có thể không cần thiết

**Đánh giá**: ⚠️ Có thể implement sau nếu cần

---

## 4. Data Model đề xuất

### 4.1 Unified AppProfileConfig

```cpp
// Thay thế cả SpecialAppEntry và ClipboardAppEntry
struct AppProfileConfig {
    std::string exeName;
    
    // Behavior: -1 = auto-detect, 0 = QtElectron, 1 = NativeRich
    int8_t behaviorType = -1;
    
    // Clipboard: -1 = auto-detect, 0 = ShiftInsert, 1 = CtrlV, 2 = SendInputKey
    int8_t clipboardMethod = -1;
    
    // Metadata
    bool isDefault = false;   // Built-in vs user-added
    // Note: delay removed (auto-detected from latency probe)
};
```

### 4.2 Config.toml format

**Mới**:
```toml
[appProfiles]
profiles = [
    { name = "discord.exe", behavior = 0, clipboard = -1 },  # Qt/Electron, auto clipboard
    { name = "powerpnt.exe", behavior = 1, clipboard = -1, default = true },  # Skip IME, default
    { name = "firefox.exe", behavior = -1, clipboard = 1 },  # Auto behavior, Ctrl+V
]
deletedDefaults = ["code.exe"]
```

### 4.3 Migration từ old format

```cpp
void migrateOldConfig() {
    auto& config = ConfigManager::instance();
    
    // Check if already migrated
    if (config.hasKey("appProfiles", "profiles")) return;
    
    // Migrate specialApps
    auto qtApps = config.getStringArray("specialApps", "qtElectronApps");
    auto imeApps = config.getStringArray("specialApps", "skipImeCheckApps");
    
    // Migrate clipboardApps
    auto clipApps = config.getClipboardApps();
    
    // Merge into appProfiles (by exeName)
    std::map<std::string, AppProfileConfig> merged;
    
    for (const auto& app : qtApps) {
        merged[toLower(app)] = {app, 0, -1, false};  // Qt/Electron
    }
    for (const auto& app : imeApps) {
        merged[toLower(app)] = {app, 1, -1, false};  // NativeRich
    }
    for (const auto& app : clipApps) {
        auto key = toLower(app.exeName);
        if (merged.find(key) != merged.end()) {
            merged[key].clipboardMethod = app.method;
        } else {
            merged[key] = {app.exeName, -1, app.method, false};
        }
    }
    
    // Save new format
    config.setAppProfiles(merged);
    
    // Remove old sections
    config.removeSection("specialApps");
    config.removeSection("clipboardApps");
    
    config.save();
}
```

---

## 5. Vị trí dialog

### So sánh các vị trí

| Vị trí | Pros | Cons |
|--------|------|------|
| **Tab Hệ thống** | Gần SmartSwitch (cùng per-app config), user quen tìm settings ở đây | Tab có thể đông settings |
| **Tab Debug** | Tách biệt, cho "advanced" users | User thường dùng khó tìm, không phù hợp semantics |
| **Context menu** (right-click tray icon) | Không chiếm space | Khó discover |
| **Nút riêng trong Settings** | Explicit | Thêm 1 button vào UI |

### Đề xuất: Tab Hệ thống

**Lý do**:
1. **Conceptual fit**: "Cấu hình ứng dụng" là system-level per-app setting, cùng category với SmartSwitch
2. **Discoverability**: Users đã quen tìm settings ở tab Hệ thống
3. **Không phải Debug**: Đây là feature chính, không phải debug tool

**Vị trí trong tab**:
```
Tab Hệ thống:
├── SmartSwitch (ngôn ngữ per-app)
├── [Cấu hình ứng dụng] ← NEW (behavior + clipboard per-app)
├── ...other settings...
```

---

## 6. Những điểm cần lưu ý khi implement

### 6.1 Backward Compatibility

- [ ] Auto-migrate old config format on startup
- [ ] Keep reading old sections if new section empty (transition period)
- [ ] Log migration for debugging

### 6.2 Default Apps Handling

- [ ] Hardcoded defaults (Qt/Electron, Skip IME) với `isDefault = true`
- [ ] User có thể override behavior/clipboard của default apps
- [ ] User có thể "xóa" default (thêm vào `deletedDefaults`)
- [ ] UI indicator: ⚙️ cho defaults, empty cho user-added

### 6.3 Auto-detect Integration

- [ ] `-1` value = let RuntimeProfile auto-detect
- [ ] UI shows "Auto" for `-1` values
- [ ] Tooltip explain: "Engine tự động phát hiện dựa trên window class và latency"

### 6.4 RuntimeProfile Sync

Khi user thay đổi config trong dialog:
```cpp
void onConfigChanged(const std::string& exeName, const AppProfileConfig& config) {
    // 1. Save to config.toml
    saveData();
    
    // 2. Notify main process
    PostMessage(mainWnd, WM_USER + 101, 0, 0);
    
    // 3. Clear cached RuntimeProfiles for this EXE
    // (Next focus change will re-apply with new config)
    invalidateProfilesForExe(exeName);
}
```

### 6.5 Files to Create/Modify

| Action | File |
|--------|------|
| CREATE | `Resources/Sciter/appprofiles/appprofiles.html` |
| CREATE | `Resources/Sciter/appprofiles/appprofiles.css` |
| CREATE | `Resources/Sciter/appprofiles/appprofiles.js` |
| CREATE | `Sources/.../AppProfilesDialogSciter.cpp` |
| CREATE | `Sources/.../AppProfilesDialogSciter.h` |
| MODIFY | `ConfigManager.h/cpp` - add `AppProfileConfig`, `getAppProfiles()`, `setAppProfiles()` |
| MODIFY | `OpenKey.cpp` - wire up new dialog, migration logic |
| DELETE | `SpecialAppsDialogSciter.*` (sau khi verify) |
| DELETE | `ClipboardAppsDialogSciter.*` (sau khi verify) |
| DELETE | `Resources/Sciter/specialapps/` |
| DELETE | `Resources/Sciter/clipboardapps/` |

---

## 7. Câu hỏi cho Senior

1. **Delay field**: Đồng ý bỏ và dùng auto-detect từ latency probe? Hay cần giữ cho edge cases?

2. **Vị trí**: Tab Hệ thống phù hợp? Hay có suggestion khác?

3. **Default apps**: Có nên cho user override type của default apps? (Hiện tại chỉ cho xóa)

4. **Migration**: Làm transparent (auto-migrate) hay hiện dialog confirm?

5. **Naming**: "Cấu hình ứng dụng" vs "App Profiles" vs "Ứng dụng đặc biệt"?

---

## 8. Tổng kết

| Aspect | Recommendation |
|--------|----------------|
| **Approach** | Gộp 2 dialogs thành 1 "App Profiles" |
| **Delay field** | Bỏ, dùng auto-detect |
| **Location** | Tab Hệ thống |
| **Default UI value** | "Auto-detect" cho cả behavior và clipboard |
| **Code reduction** | ~600 lines duplicate removed |
| **User experience** | 1 dialog thay vì 2, simpler config |

---

*Document created for senior review before implementation.*
