# Task 10: Auto-Update Feature

## Status
Done

## Goal
Implement automatic update checking and downloading from GitHub Releases.

## Current Bug ⚠️
`OpenKeyManager::checkUpdate()` (line 61) fetches from **wrong repo**:
```cpp
// WRONG - points to original author's repo, not your fork
L"https://raw.githubusercontent.com/tuyenvm/OpenKey/master/version.json"
```

Need to change to your fork: `phatMT97/OpenKey`

## GitHub Release Structure
URL: `https://github.com/phatMT97/OpenKey/releases`
API: `https://api.github.com/repos/phatMT97/OpenKey/releases/latest`

Assets:
- `OpenKey-x64.zip` (64-bit)
- `OpenKey-x86.zip` (32-bit)

## Flow

### 1. Check for Updates
```
User click "Check Update" or on startup (if vCheckNewVersion enabled)
    ↓
Fetch: https://api.github.com/repos/phatMT97/OpenKey/releases/latest
    ↓
Parse JSON → get tag_name (e.g., "v1.0.3")
    ↓
Compare with current version
    ↓
If new version available → Show dialog
```

### 2. Download & Install
```
User clicks "Update Now"
    ↓
Detect architecture: #ifdef _WIN64 → x64, else → x86
    ↓
Download: assets[].browser_download_url matching "OpenKey-x64.zip" or "OpenKey-x86.zip"
    ↓
Save to %TEMP%\OpenKey-update.zip
    ↓
Extract to %TEMP%\OpenKey-update\
    ↓
Launch updater (OpenKeyUpdate.exe) with args: --update <extracted_path>
    ↓
OpenKeyUpdate waits for OpenKey to exit, then replaces files
```

## Implementation

### Files to Modify/Create

#### [MODIFY] OpenKeyManager.cpp
- Update `checkUpdate()` to use GitHub API instead of version.json
- Add `downloadUpdate(const std::string& downloadUrl)`
- Add `getArchitectureAssetName()` → "OpenKey-x64.zip" or "OpenKey-x86.zip"

#### [MODIFY] AppDelegate.cpp
- Add menu item "Kiểm tra cập nhật" 
- Add update notification dialog with "Cập nhật ngay" button

#### [EXISTING] OpenKeyUpdate.exe
- Already exists for update installation
- May need modifications for new flow

### GitHub API Response (releases/latest)
```json
{
  "tag_name": "v1.0.3-rc",
  "assets": [
    {
      "name": "OpenKey-x64.zip",
      "browser_download_url": "https://github.com/.../OpenKey-x64.zip"
    },
    {
      "name": "OpenKey-x86.zip", 
      "browser_download_url": "https://github.com/.../OpenKey-x86.zip"
    }
  ]
}
```

### Version Comparison
Current: `OpenKeyHelper::getVersionString()` → "1.0.3 RC"
Remote: parse `tag_name` → "v1.0.3-rc" → "1.0.3"

Need to handle:
- Semantic versioning (1.0.3 vs 1.0.4)
- Pre-release tags (rc, beta, alpha)

## UI

### Update Available Dialog
```
┌─────────────────────────────────────┐
│  Có phiên bản mới!              [X] │
├─────────────────────────────────────┤
│  Phiên bản hiện tại: 1.0.2          │
│  Phiên bản mới: 1.0.3               │
│                                     │
│  [Cập nhật ngay]  [Để sau]          │
└─────────────────────────────────────┘
```

### Progress Dialog (Optional)
Show download progress bar.

## Effort
~3-4 hours

## Dependencies
- WinHTTP or URLDownloadToFile for downloading
- JSON parser (nlohmann/json or manual parsing)
- Zip extraction (existing or add library)

## Testing Strategies

### 1. Mock Lower Version (Recommended for E2E test)
Temporarily change version in `OpenKey.rc`:
```rc
// Change from 1,0,3,0 to 1,0,2,0
FILEVERSION 1,0,2,0
PRODUCTVERSION 1,0,2,0
```
Build → Run → Should detect 1.0.3 as new version

### 2. Debug Flag for Fake Response
```cpp
#ifdef _DEBUG
// Force "new version available" for testing
newVersion = "1.0.99";
return true;
#endif
```

### 3. Test Each Component Separately

| Component | How to Test |
|-----------|-------------|
| Fetch API | `curl https://api.github.com/repos/phatMT97/OpenKey/releases/latest` |
| Parse JSON | Unit test with sample response |
| Download | Test download zip manually |
| Extract | Test unzip to temp folder |
| Updater | Run `OpenKeyUpdate.exe --update <path>` manually |

### 4. Pre-release Tag Testing
- Current release: `v1.0.3-rc`
- Create test release: `v1.0.4-test`
- Local build: 1.0.3 → Should detect 1.0.4

### Edge Cases to Test
- No internet connection
- GitHub API rate limit (60 req/hour unauthenticated)
- Invalid JSON response
- Download interrupted
- Wrong architecture selection
- User cancels update

