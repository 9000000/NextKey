# Task 9: SpecialAppsDialog (User-Configurable App Lists)

## Goal
Create a dialog for users to manage special app lists instead of hardcoding.

## Two Lists to Manage
1. **Qt/Electron Apps** - Skip empty char fix (prevents lag in VSCode, Discord, etc.)
2. **Skip IME Check Apps** - Apps that falsely report IME as ON (PowerPoint, Word, Excel)

## Approach
Clone `ExcludedAppsDialogSciter` and modify.

## Subtasks

### 9.1 Extract RunningAppsHelper (~30 min)
Create reusable helper for getting running apps:
```cpp
// RunningAppsHelper.h
class RunningAppsHelper {
public:
    static std::vector<std::string> getRunningApps();
    static std::string getExeNameFromWindow(HWND hwnd);
};
```

### 9.2 Refactor ExcludedAppsDialogSciter (~15 min)
Replace inline code with helper calls.

### 9.3 Create SpecialAppsDialogSciter (~1 hour)
Files:
- `SpecialAppsDialogSciter.cpp/h` 
- `Resources/Sciter/specialapps/specialapps.html`

Changes from ExcludedApps:
- Title: "Ứng dụng đặc biệt"
- Add radio buttons to choose list type
- Storage: `vQtElectronApps`, `vSkipImeCheckApps` (registry, comma-separated)
- Load: Merge user additions with hardcoded defaults

### 9.4 Settings Debug Tab Button (~15 min)
Add button "Quản lý ứng dụng đặc biệt" in settings.html.
OnClick: spawn subprocess with `--specialapps` arg.

## Storage Format (Registry)
```
vQtElectronApps = "code.exe,discord.exe,slack.exe"
vSkipImeCheckApps = "powerpnt.exe,winword.exe,excel.exe"
```

## UI Mockup
```
┌─────────────────────────────────────┐
│  Ứng dụng đặc biệt              [X] │
├─────────────────────────────────────┤
│  ○ Skip empty char (Qt/Electron)    │
│  ● Skip IME check (Office, etc.)    │
├─────────────────────────────────────┤
│  [▼ Chọn ứng dụng đang chạy]        │
│  [+ Thêm]  [🎯 Chọn cửa sổ]         │
├─────────────────────────────────────┤
│  powerpnt.exe                  [🗑] │
│  winword.exe                   [🗑] │
│  excel.exe                     [🗑] │
└─────────────────────────────────────┘
```

## Effort
~2-3 hours total
