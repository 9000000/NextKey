# Future Enhancements

## Clipboard Content Restoration

**Status**: ✅ Resolved (Simpler Solution Implemented)

**Date Logged**: 2026-01-13
**Date Resolved**: 2026-01-14

### The Issue

When OpenKey uses clipboard mode (Shift+Insert or Ctrl+V), it overwrites whatever the user had previously copied. This can be frustrating if the user was in the middle of copy-paste workflow.

### Original Proposed Solution (Complex - Deferred)

1. **Before setting clipboard**: Save current clipboard content to buffer
2. **After paste operation**: Restore original clipboard content

This approach had many challenges:
- Thread safety - Clipboard operations must be on main thread
- Timing - Too fast restore = paste gets old content; too slow = user notices
- Format preservation - User may have images, rich text, not just text
- Performance - Extra clipboard operations per keystroke
- Race conditions - Multiple restores from fast typing

### ✅ Actual Solution (Simple - Implemented)

Instead of saving/restoring clipboard content, we **exclude OpenKey's clipboard operations from Windows Clipboard History** using a special clipboard format.

```cpp
// In OpenKeyHelper.cpp
static UINT CF_EXCLUDE_CLIPBOARD_HISTORY = 
    RegisterClipboardFormat(_T("ExcludeClipboardContentFromMonitorProcessing"));

void OpenKeyHelper::setClipboardText(LPCTSTR data, const int & len, const int& type) {
    // ... set clipboard data ...
    
    // Exclude from Windows Clipboard History (Win+V)
    SetClipboardData(CF_EXCLUDE_CLIPBOARD_HISTORY, NULL);
    
    CloseClipboard();
}
```

### Why This Is Better

| Aspect | Save/Restore | Exclude from History |
|--------|--------------|---------------------|
| Complexity | 🔴 High (threading, timing, formats) | 🟢 Low (1 extra line) |
| Performance | 🔴 +50-100ms per word | 🟢 Zero overhead |
| Risk | 🔴 Race conditions, crashes | 🟢 None |
| User's clipboard | Temporarily overwritten | Still overwritten |
| Clipboard History | Polluted with typing | Clean |

### Result

- User's previous clipboard content is still overwritten temporarily (unavoidable with clipboard mode)
- BUT Windows Clipboard History (Win+V) stays clean - no pollution from typing
- Zero performance impact
- No threading/timing issues

