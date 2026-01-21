# Settings Toggle Implementation Checklist

This checklist ensures all necessary files are updated when implementing a new settings toggle.

## Required Files (9 total)

### 1. Variable Declaration & Definition
| File | Action | Example |
|------|--------|---------|
| `Engine.h` | Add `extern int vNewSetting;` | `extern int vTempOffMacro;` |
| `AppDelegate.cpp` | Initialize `int vNewSetting = 0;` | `int vTempOffMacro = 0;` |
| `AppDelegate.h` | Add `extern int vNewSetting;` | `extern int vTempOffMacro;` |

### 2. Config Loading (3 locations)
| File | Function | Action |
|------|----------|--------|
| `OpenKey.cpp` | `OpenKeyInit()` | Load from config at app startup |
| `SettingsDialog.cpp` | Constructor | Load from config when dialog opens |
| `SystemTrayHelper.cpp` | `WM_USER+101` handler | Load from config on settings reload |

### 3. Config Saving
| File | Function | Action |
|------|----------|--------|
| `SettingsDialog.cpp` | `syncSettingsToConfig()` | `config.setBool("section", "key", vNewSetting);` |

### 4. IPC Sync (Main ↔ Subprocess)
| File | Action |
|------|--------|
| `ConfigIntent.h` | Add field to `SettingsPayload` struct |
| `SettingsDialog.cpp` | Add to `buildSettingsPayload()` |
| `SystemTrayHelper.cpp` | **WM_COPYDATA handler - 2 steps:** |
|  | 1. `config.setBool(section, key, settings.field)` → save to config |
|  | 2. `vNewSetting = settings.field` → update global variable |

### 5. UI Toggle
| File | Action |
|------|--------|
| `settings.html` | Add toggle HTML element with `id` and hidden input `val-*` |
| `SettingsDialog.cpp` | `setToggleState("#toggle-id", vNewSetting)` in `DOCUMENT_READY` |
| `SettingsDialog.cpp` | Add `VALUE_CHANGED` handler for `val-*` input |

## Implementation Order

```
1. AppDelegate.cpp → Define variable with initial value
2. AppDelegate.h   → Extern declaration (win32 scope)
3. Engine.h        → Extern declaration (cross-platform)
4. OpenKey.cpp     → Config load in OpenKeyInit()
5. SettingsDialog.cpp → Config load in constructor
6. SettingsDialog.cpp → syncSettingsToConfig()
7. SettingsDialog.cpp → setToggleState() in DOCUMENT_READY
8. SettingsDialog.cpp → VALUE_CHANGED handler
9. SettingsDialog.cpp → buildSettingsPayload()
10. ConfigIntent.h    → Add field to SettingsPayload
11. SystemTrayHelper.cpp → Config load in WM_USER+101
12. SystemTrayHelper.cpp → Handle in WM_COPYDATA
13. settings.html     → Add toggle UI element
```

## Config Key Naming Convention

| Section | Key Format | Example |
|---------|------------|---------|
| general | `camelCase` | `inputType`, `switchKey` |
| typing | `camelCase` | `checkSpelling`, `tempOffSpellingCtrl` |
| macro | `camelCase` | `enabled`, `tempOffMacroEsc` |
| system | `camelCase` | `runWithWindows`, `iconStyle` |

## Data Flow

```
┌─────────────────┐   VALUE_CHANGED   ┌────────────────────┐
│  settings.html  │ ───────────────▶ │  SettingsDialog    │
│  (UI Toggle)    │                   │  (set vNewSetting) │
└─────────────────┘                   └────────┬───────────┘
                                               │
                                               │ notifyMainProcess()
                                               │ → buildSettingsPayload()
                                               │ → sendSettingsIntent()
                                               ▼
┌─────────────────────────────────────────────────────────────────┐
│                  WM_COPYDATA (IPC)                              │
│  SettingsPayload { ... newSetting: uint8_t ... }               │
└─────────────────────────────────────────────────────────────────┘
                                               │
                                               ▼
┌─────────────────┐                   ┌────────────────────┐
│  ConfigManager  │ ◀───────────────│  SystemTrayHelper  │
│  (RAM cache)    │    config.setBool │  WM_COPYDATA handler│
└────────┬────────┘                   │  vNewSetting = val │
         │                            └────────────────────┘
         │ Debounced save()
         ▼
┌─────────────────┐
│  config.toml    │
│  (Disk)         │
└─────────────────┘
```

## Registry Migration (Optional)

Only needed if migrating from old OpenKey Registry settings:

| File | Function | Action |
|------|----------|--------|
| `ConfigManager.cpp` | `migrateFromRegistry()` | Add `cfg.setBool(section, key, readRegInt(...))` |

> **Note**: For NEW settings not in old Registry, migration is NOT needed.

## Quick Verification

After implementation, search for your config key to verify all locations:

```bash
grep -r "tempOffMacroEsc" Sources/OpenKey/win32/OpenKey/OpenKey/
```

### Expected matches per file:

| File | Matches | Purpose |
|------|---------|---------|
| `OpenKey.cpp` | 1 | Startup config load |
| `SettingsDialog.cpp` | 5 | Load, save, setToggleState, VALUE_CHANGED, buildPayload |
| `SystemTrayHelper.cpp` | 2 | WM_USER+101 reload, WM_COPYDATA handler |

### Variable search:

```bash
grep -r "vTempOffMacro" Sources/OpenKey/
```

| File | Matches | Purpose |
|------|---------|---------|
| `Engine.h` | 1 | Extern declaration (cross-platform) |
| `AppDelegate.h` | 1 | Extern declaration (win32) |
| `AppDelegate.cpp` | 1 | Variable definition |
| `OpenKey.cpp` | 2 | Config load + usage |
| `SettingsDialog.cpp` | 5 | All handlers |
| `SystemTrayHelper.cpp` | 2 | Config load + IPC |
| `settings.html` | 2 | Toggle div + hidden input |
| `ConfigIntent.h` | 1 | SettingsPayload field |

**Total: 15 matches across 8 files**
