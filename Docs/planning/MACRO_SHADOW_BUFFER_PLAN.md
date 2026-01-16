# Implementation Plan: Shadow Buffer for Macro Tracking

## Problem Confirmed

**Debug findings:**
```
[MACRO] MAIN_SPACE_CHECK: [82(R) 76(L) ] size=2 handled=0
```
When typing `u-r-r-l` in Vietnamese mode:
- Buffer only contains 2 chars (R, L) instead of 3 (U, R, L)
- U was lost during Vietnamese processing
- Macro matching fails

**Root cause:** hMacroKey is corrupted by Vietnamese engine (vWillProcess/vRestore), exactly as senior developers predicted.

---

## Design Decision Required

### Question: Should macros support Vietnamese characters?

**Case 1: English macro in VN mode**
- User types: `u-r-r-l` (with reversion)
- Expects: `url` → `URL`
- Solution: Track **raw keycodes** before Vietnamese processing

**Case 2: Vietnamese macro**
- User types: `u-w` (Telex) → displays `ủ`
- User wants macro: `ủ` → `ủy ban`
- Problem: Raw buffer has `[u, w]`, not `[ủ]`
- **How to match?**

### Option A: Raw Keycode Only (Simpler)
**Pros:**
- Solves English macro problem (url, btw, etc.)
- Simple implementation
- No ambiguity

**Cons:**
- Cannot support Vietnamese macros
- User must define macro as raw keys: `uw` → `ủy ban` (confusing!)

### Option B: Dual Buffer System (Complex)
**Pros:**
- Support both English and Vietnamese macros
- User can define: `ủ` → `ủy ban` OR `url` → `URL`

**Cons:**
- More complex: maintain 2 buffers
- Need to check both when matching
- Performance overhead

### Option C: Hybrid (Recommended)
**Strategy:**
1. Track raw keycodes in shadow buffer
2. When space pressed, check **both**:
   - Raw buffer: `[u, r, l]` → matches `url` macro
   - Processed buffer (current TypingWord): `[ủ]` → matches `ủ` macro
3. Priority: Try raw first, then processed

**Pros:**
- Solves both use cases
- Flexible for users
- Backwards compatible

**Cons:**
- Slightly more complex than Option A
- Need clear priority rules

---

## Proposed Solution: Option C (Hybrid)

### Architecture

```
┌─────────────┐
│  Raw Input  │
└──────┬──────┘
       │
       ├────────────────────────────┬─────────────────────┐
       │                            │                     │
       v                            v                     v
┌──────────────┐         ┌──────────────────┐   ┌────────────────┐
│ Shadow Buffer│         │ Vietnamese Engine│   │ hMacroKey (old)│
│  (Raw Keys)  │         │   (TypingWord)   │   │   [REMOVE]     │
└──────┬───────┘         └────────┬─────────┘   └────────────────┘
       │                          │
       │         Space            │
       └──────────┬───────────────┘
                  v
       ┌────────────────────┐
       │   Macro Matching   │
       │   1. Try Shadow    │
       │   2. Try Processed │
       └────────────────────┘
```

### Implementation Steps

#### Phase 1: Shadow Buffer Setup
```cpp
// In Engine.cpp - global scope
static vector<Uint32> g_rawMacroBuffer;  // Shadow buffer

// At the START of vKeyHandleEvent (before Vietnamese processing)
void vKeyHandleEvent(...) {
    // 1. Track raw input FIRST
    if (isAlphaNumeric(data)) {
        g_rawMacroBuffer.push_back(data | (_isCaps ? CAPS_MASK : 0));
    } else if (isWordBreak(data)) {
        g_rawMacroBuffer.clear();
    }
    
    // 2. THEN do Vietnamese processing (existing code)
    // ...
}
```

#### Phase 2: Macro Matching Logic
```cpp
// When space pressed (in main handler)
if (vUseMacro && !_hasHandledMacro) {
    // Try 1: Raw buffer (for English words like "url")
    if (findMacro(g_rawMacroBuffer, hMacroData)) {
        hCode = vReplaceMaro;
        hBPC = g_rawMacroBuffer.size();  // Delete raw chars
        _hasHandledMacro = true;
    }
    // Try 2: Processed buffer (for Vietnamese words like "ủ")
    else if (_index > 0) {
        vector<Uint32> processedBuffer;
        for (int i = 0; i < _index; i++) {
            processedBuffer.push_back(TypingWord[i]);
        }
        if (findMacro(processedBuffer, hMacroData)) {
            hCode = vReplaceMaro;
            hBPC = processedBuffer.size();
            _hasHandledMacro = true;
        }
    }
    
    g_rawMacroBuffer.clear();
}
```

#### Phase 3: Edge Cases

**Backspace handling:**
```cpp
if (data == KEY_BACKSPACE || data == KEY_DELETE) {
    if (!g_rawMacroBuffer.empty()) {
        g_rawMacroBuffer.pop_back();
    }
}
```

**Cursor movement / Mouse click:**
```cpp
if (state == MouseDown || isNavigationKey(data)) {
    g_rawMacroBuffer.clear();  // Reset on word boundary
}
```

**Vietnamese revert (u-r-r):**
- Shadow buffer keeps: `[u, r, r]`
- Vietnamese engine shows: `ur`
- When space: check shadow `[u, r, r]`? NO - should be `[u, r]`
- **Solution:** Trust shadow buffer AS-IS, user typed 3 keys, buffer = 3 keys

**Wait, problem:** When user types `u-r-r-l`:
- Shadow: `[u, r, r, l]` (4 keys)
- Visual: `url` (3 chars)
- Macro defined as: `url` (3 chars)
- **Mismatch!**

### Refined Strategy

**The issue:** Raw buffer doesn't sync with Vietnamese reversions.

**Better approach:**
- Shadow buffer tracks **visual intent**, not raw keys
- When Vietnamese revert happens (vRestore):
  - **Rebuild shadow from TypingWord** using base characters
  - Use `CHR()` or character extraction

```cpp
// After Vietnamese processing
if (hCode == vRestore) {
    // Sync shadow with actual display
    g_rawMacroBuffer.clear();
    for (int i = 0; i < _index; i++) {
        Uint32 baseChar = CHR(i);  // Extract base char (u, r, l)
        g_rawMacroBuffer.push_back(baseChar);
    }
}
```

**But wait...** this brings us back to the original problem if `CHR()` doesn't work!

---

## Alternative: Semantic Layer

### The Real Solution (Senior P's insight)

Don't track **characters**. Track **semantic tokens**.

```cpp
struct MacroToken {
    wstring visual;      // What user sees: "url" or "ủ"
    vector<Uint32> raw;  // Raw keys pressed
};

// When Vietnamese processing happens:
// 1. Track raw: [u, w]
// 2. Result visual: "ủ"
// 3. Create token: {visual="ủ", raw=[u,w]}

// Macro matching:
// - Match against token.visual
// - This handles BOTH English and Vietnamese!
```

---

## Recommendation

### For v1.0 (Quick Fix):

**Disable macro in Vietnamese processing window**

```cpp
// Simple flag approach
if (hCode == vWillProcess || hCode == vRestore) {
    _disableMacroThisWord = true;
}

// On space
if (vUseMacro && !_disableMacroThisWord && findMacro(hMacroKey, hMacroData)) {
    // ... expand macro
}

// On word break / new word
_disableMacroThisWord = false;
```

**Result:**
- `btw` in VN mode → macro works ✅
- `u-r-r-l` in VN mode → macro **doesn't work** ❌ (acceptable limitation)
- `url` in English mode → macro works ✅

**Pros:** 
- 5 lines of code
- No risk
- Clear behavior

**Cons:**
- Doesn't solve original problem
- User must switch to English mode for English macros

### For v2.0 (Proper Fix):

Implement **shadow buffer with visual sync** (detailed in Phase 1-3 above).

---

## User Question: `ủ` → `ủy ban` macro?

**Answer depends on strategy:**

**If raw-only:**
- User must define macro as: `uw` → `ủy ban` ❌ (confusing)

**If visual-based (recommended):**
- User defines: `ủ` → `ủy ban` ✅
- Engine matches against final visual output
- Works for both VN and EN macros

**Implementation for visual matching:**
```cpp
// Build visual buffer from TypingWord for matching
wstring getVisualWord() {
    wstring result;
    for (int i = 0; i < _index; i++) {
        wchar_t ch = (wchar_t)(TypingWord[i] & 0xFFFF);
        result += ch;
    }
    return result;
}

// Macro check
wstring visual = getVisualWord();  // "ủ" or "url"
if (findMacroByVisual(visual, hMacroData)) {
    // Expand!
}
```

---

## Next Steps

1. **Decide:** Quick fix (v1.0) or proper fix (v2.0)?
2. **If v1.0:** Implement disable flag (30 minutes)
3. **If v2.0:** Need 1-2 days for:
   - Shadow buffer
   - Visual matching
   - Edge cases
   - Testing

Which approach do you prefer? 🤔
