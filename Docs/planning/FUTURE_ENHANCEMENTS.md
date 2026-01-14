# Future Enhancements

## Clipboard Content Restoration

**Status**: Deferred (Planned for future release)

**Date Logged**: 2026-01-13

### The Issue

When OpenKey uses clipboard mode (Shift+Insert or Ctrl+V), it overwrites whatever the user had previously copied. This can be frustrating if the user was in the middle of copy-paste workflow.

### Proposed Solution

1. **Before setting clipboard**: Save current clipboard content to buffer
2. **After paste operation**: Restore original clipboard content

### Implementation Sketch

```cpp
static std::wstring savedClipboardText;

static void SendNewCharString(...) {
    // Save current clipboard
    savedClipboardText = OpenKeyHelper::getClipboardText();
    
    // Set our text and paste
    OpenKeyHelper::setClipboardText(...);
    SendCombineKey(...);
    
    // Restore after short delay (async)
    std::thread([](std::wstring text) {
        Sleep(50); // Wait for paste to complete
        OpenKeyHelper::setClipboardText(text.c_str(), text.size(), CF_UNICODETEXT);
    }, savedClipboardText).detach();
}
```

### Challenges

1. **Thread safety** - Clipboard operations must be on main thread
2. **Timing** - Too fast restore = paste gets old content; too slow = user notices
3. **Format preservation** - User may have images, rich text, not just text
4. **Performance** - Extra clipboard operations per keystroke

### Acceptance Criteria

- [ ] User's previous clipboard content is preserved
- [ ] Works with text, images, and rich content
- [ ] No noticeable delay in typing
- [ ] Optional toggle in settings
