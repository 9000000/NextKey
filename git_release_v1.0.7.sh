# Git Commands for v1.0.7 Release

## 1. Stage changes
```bash
cd /mnt/c/Users/Admin/code/OpenKey  # Or your OpenKey workspace path
git add Sources/OpenKey/win32/OpenKey/OpenKey/OpenKey.rc
git add RELEASE_NOTES.md
git add Sources/OpenKey/win32/OpenKey/OpenKey/ConfigIntent.h
git add Sources/OpenKey/win32/OpenKey/OpenKey/SystemTrayHelper.cpp
git add Sources/OpenKey/win32/OpenKey/OpenKey/MacroDialogSciter.cpp
git add Sources/OpenKey/win32/OpenKey/OpenKey/SettingsDialog.cpp
git add Sources/OpenKey/win32/OpenKey/OpenKey/ConvertToolDialogSciter.cpp
git add Sources/OpenKey/win32/OpenKey/OpenKey/ExcludedAppsDialogSciter.cpp
git add Sources/OpenKey/win32/OpenKey/OpenKey/ConfigManager.h
git add Sources/OpenKey/win32/OpenKey/OpenKey/ConfigManager.cpp
```

## 2. Commit
```bash
git commit -m "chore: release v1.0.7 - Central Writer hotfix

Fix critical bug: settings lost when multiple dialogs modify config

Changes:
- Implement Central Writer Architecture (single writer pattern)
- Add IPC via WM_COPYDATA for config intents
- Add debounce timer (1.5s) to optimize disk I/O
- Refactor dialogs: Macro, Settings, ConvertTool, ExcludedApps
- Add force save on exit (WM_DESTROY, WM_ENDSESSION)
- Update version 1.0.6 -> 1.0.7

Technical:
- New file: ConfigIntent.h (IPC message types)
- Modified: SystemTrayHelper.cpp (WM_COPYDATA handler)
- Modified: 4 dialogs (send intents instead of direct save)
- Runtime warning in DEBUG build for save() calls
"
```

## 3. Tag
```bash
git tag -a v1.0.7 -m "v1.0.7 - Hotfix Config Save

Fix bug: settings overwrite khi dùng nhiều dialog
Implement Central Writer Architecture
Optimize disk I/O với debounce 1.5s
"
```

## 4. Push
```bash
git push origin main
git push origin v1.0.7
```

---

## One-liner (copy-paste toàn bộ)

```bash
cd /mnt/c/Users/Admin/code/OpenKey && \
git add Sources/OpenKey/win32/OpenKey/OpenKey/OpenKey.rc RELEASE_NOTES.md Sources/OpenKey/win32/OpenKey/OpenKey/ConfigIntent.h Sources/OpenKey/win32/OpenKey/OpenKey/SystemTrayHelper.cpp Sources/OpenKey/win32/OpenKey/OpenKey/MacroDialogSciter.cpp Sources/OpenKey/win32/OpenKey/OpenKey/SettingsDialog.cpp Sources/OpenKey/win32/OpenKey/OpenKey/ConvertToolDialogSciter.cpp Sources/OpenKey/win32/OpenKey/OpenKey/ExcludedAppsDialogSciter.cpp Sources/OpenKey/win32/OpenKey/OpenKey/ConfigManager.h Sources/OpenKey/win32/OpenKey/OpenKey/ConfigManager.cpp && \
git commit -m "chore: release v1.0.7 - Central Writer hotfix" && \
git tag -a v1.0.7 -m "v1.0.7 - Hotfix Config Save" && \
git push origin main && \
git push origin v1.0.7
```

**Note:** Bạn cần edit lại path `/mnt/c/Users/Admin/code/OpenKey` cho đúng workspace của mình nếu khác.
