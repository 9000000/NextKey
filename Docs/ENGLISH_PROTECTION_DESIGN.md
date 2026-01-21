# 3-Tier English Protection Design

> **Design approved: 2026-01-21**  
> **Status: Ready for implementation**

## Design Principle

> **IME nên hiểu "dáng chữ" (phonotactic shape), không nên nhớ "từ" (vocabulary)**

IME is a **composition engine**, not a spell checker. It should serve user intent, not enforce correctness.

---

## Problem

OpenKey engine uses Vietnamese phonotactics to decide whether to apply diacritics. This causes:
- English words like `year`, `you` to receive unwanted diacritics (`yẻar`, `yỏu`)
- Words starting with `y + vowel` are in a "gray zone" between Vietnamese and English

---

## Solution: 3-Tier Protection

### TIER 1: Hard Reject (Impossible in Vietnamese)

**Patterns that CANNOT appear in any Vietnamese word:**

```cpp
// Start clusters
static const char* _hardEnglishStartClusters[] = {
    "cl", "cr", "br", "dr", "fr", "gr", "pr",
    "sm", "sn", "sp", "sw", "st", "sc", "sk",
    "bl", "fl", "gl", "pl", "sl", "wr", nullptr
};

// End consonants
static const Uint16 _hardEnglishEndConsonants[] = { KEY_X, KEY_R, KEY_Z, KEY_F, 0 };

// Also: Q without U
```

**Behavior:** `_langBias = HardEnglish` → composition disabled entirely

---

### TIER 2: Soft Bias (Ambiguous patterns)

**Rule-based, NOT word list:**

```cpp
// y + vowel WITHOUT valid Vietnamese continuation = ambiguous
if (CHR(0) == KEY_Y && IS_VOWEL(CHR(1)) && !isValidVietnameseYSequence()) {
    return true;  // Soft English bias
}
```

**Valid Vietnamese Y-sequences:**
- `yê` + `u/n/m/t` (yêu, yên, yêm, yết, yếu...)

**Behavior:** Defer tone on first press, apply on second press of **same key**

---

### TIER 3: User Override (Same-key insistence)

```cpp
if (data == _lastToneKey) {
    _sameToneKeyCount++;
} else {
    _lastToneKey = data;
    _sameToneKeyCount = 1;
}

if (_sameToneKeyCount >= 2) {
    // User insisted → apply tone
}
```

**Why same-key?**
- Avoids false-positive when user presses wrong key
- No timer, no magic number, no setting needed
- Matches Vietnamese typing habit: "press again = I mean it"

---

## State Management

```cpp
enum class LanguageBias { Unknown, HardEnglish, SoftEnglish, Vietnamese };
static LanguageBias _langBias = LanguageBias::Unknown;
static Uint16 _lastToneKey = 0;
static int _sameToneKeyCount = 0;
```

**Reset on:**
- `startNewSession()` (word boundary)
- `handleBackspace()` when `_index < 2`

---

## Test Cases

| Input | Expected | Tier |
|-------|----------|------|
| `clear`, `fix`, `year` | raw | 1 - Hard |
| `yo` + `r` once | `yo` | 2 - Soft (defer) |
| `yo` + `r` twice | `yỏ` | 3 - Override |
| `yêu` (y-e-e-u) | `yêu` | VN valid |
| `thương` | `thương` | VN valid |

---

## Design Decisions

### ❌ No word list
- v1 had `"year", "you", "yes"...` which is vocabulary, not shape
- Cannot chase every English word
- v2 uses phonotactic rules → generalizes to `youtube`, `yolo`, `yacht`...

### ❌ No configuration for TIER 2
- Same-key insistence is already a natural override
- Adding settings increases complexity without clear benefit
- May add hidden config later if real UX feedback requires

### ❌ No changes to Vietnamese.cpp
- Existing `_vowelCombine`, `_consonantTable` stay as-is
- They are reference for Vietnamese validity, not blockers

---

## Files to Modify
- `EnglishProtection.h`: API header
- `EnglishProtection.cpp`: Implementation of 3-tier logic
- `Engine.cpp`: Updated to use EnglishProtection module
- No changes to `Vietnamese.cpp`

---

## References

- Senior review discussion: 2026-01-21
- Related: [TONE_KEY_BUG_ANALYSIS.md](./TONE_KEY_BUG_ANALYSIS.md)
