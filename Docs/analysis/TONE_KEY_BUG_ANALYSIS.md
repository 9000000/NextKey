# Tone Key Bug Analysis & Fix Proposal

**Author:** Investigation Team  
**Date:** 2026-01-20  
**Status:** Maintained for Future Reference (Architecture Decision Record)

---

## 1. Problem Statement

### Symptom
When typing `cungx` in Notepad++, occasionally the output is `cungx` (raw keystrokes) instead of `cũng` (Vietnamese word with ngã tone).

### Characteristics
- **Frequency:** Rare, intermittent
- **Environment:** Notepad++ with STEP_BY_STEP injection mode
- **Timing:** Occurs when keystroke processing is very fast (<1ms)

---

## 2. Investigation Methodology

### 2.1 Tripwire-Based Debugging
Instead of spam logging (which would overwhelm I/O), we implemented **invariant-based assertions** that capture state snapshots only when violations occur.

### 2.2 Tripwires Implemented

| ID | Location | Condition | Purpose |
|----|----------|-----------|---------|
| C | OpenKey.cpp STEP_BY_STEP | `elapsedMs < 1.0 && chars > 2` | Detect race condition |
| D | OpenKey.cpp STEP_BY_STEP | `_index == 0 && bs > 0` | Detect empty word BS |
| E | OpenKey.cpp keyboardHookProcess | `appSwitch && _index > 0` | Detect stale buffer |
| F | Engine.cpp insertMark | `bs != nc` | Detect mismatch at source |

### 2.3 Stress Test Script
Created `Tools/stress_test.ps1` with "dirty state" techniques:
- 40% no space at word end (incomplete word boundary)
- 20% backspace mid-word (partial replace scenarios)
- 5% cursor movement (position corruption)
- Random timing jitter 0-8ms

---

## 3. Test Results

### 3.1 Tripwire Fired
After ~646 iterations, **TRIPWIRE_C** captured:

```json
{
  "reason": "TRIPWIRE_C: STEP_TOO_FAST",
  "app": "notepad++.exe",
  "timestamp": 515641750,
  "index": 4,
  "wordLen": 4,
  "bsCount": 4,
  "newCharCount": 3,
  "code": 1,
  "stepByStep": true,
  "injectionMethod": -1,
  "imeCachedState": false,
  "elapsedMs": 0.8397,
  "typingWord": ["0x43", "0x80055", "0x4e", "0x47"]
}
```

### 3.2 Decoded Data
- `typingWord[0] = 0x43` → 'C'
- `typingWord[1] = 0x80055` → 'U' + 0x80000 (MARK_MASK bit 19 = ngã tone)
- `typingWord[2] = 0x4E` → 'N'
- `typingWord[3] = 0x47` → 'G'
- Word: **"CŨNG"** (correct Vietnamese word)

### 3.3 Critical Finding
```
bsCount = 4      (delete 4 characters)
newCharCount = 3 (insert only 3 characters)
```

**→ 1 character is LOST during replacement!**

---

## 4. Root Cause Analysis

### 4.1 Code Location
[Engine.cpp:checkRestoreIfWrongSpelling()](file:////wsl.localhost/Ubuntu-24.04/home/phatmt/code/OpenKey/Sources/OpenKey/engine/Engine.cpp#L1267)

### 4.2 Original Code
```cpp
bool checkRestoreIfWrongSpelling(const int& handleCode) {
    for (ii = 0; ii < _index; ii++) {
        if (!IS_CONSONANT(CHR(ii)) &&
            (TypingWord[ii] & MARK_MASK || TypingWord[ii] & TONE_MASK || TypingWord[ii] & TONEW_MASK)) {
            
            hCode = handleCode;
            hBackspaceCount = _index;        // ← Uses _index for backspace count
            hNewCharCount = _stateIndex;     // ← Uses _stateIndex for new char count
            for (i = 0; i < _stateIndex; i++) {
                TypingWord[i] = KeyStates[i];
                hData[_stateIndex - 1 - i] = TypingWord[i];
            }
            _index = _stateIndex;
            return true;
        }
    }
    return false;
}
```

### 4.3 Problem Explanation

**Two separate indices are used:**
- `_index`: Number of **composed characters** in `TypingWord[]` (e.g., 4 for "CŨNG")
- `_stateIndex`: Number of **raw keystrokes** in `KeyStates[]` (e.g., 3 if desync occurs)

**Normal case:** `_stateIndex == _index` → OK

**Bug case:** `_stateIndex < _index`
- `hBackspaceCount = 4` (delete 4 composed chars)
- `hNewCharCount = 3` (insert only 3 raw keystrokes)
- **Result:** 1 character lost → raw keystroke 'x' leaks to app

### 4.4 Desync Scenarios
Possible causes for `_stateIndex < _index`:
1. Race condition in STEP_BY_STEP timing (~0.84ms)
2. App switch resetting `_stateIndex` but not `_index`
3. Edge case in keystroke handling where `insertState()` not called

---

## 5. Proposed Fix

### 5.1 Engine.cpp - Core Fix
```cpp
// Senior Review RFC: Centralize invariant checking
// Detects when Logical Length (_index) differs from Physical Length (_stateIndex)
// specifically when restoration results in DATA LOSS (contraction).
//
// ARCHITECTURAL NOTE: 
// Vietnamese Restore (Undo Composition) is strictly Expansion (e.g. â -> aa) or Neutral.
// Contraction (nc < bs) is ILLEGAL in this specific context.
// Future features (English auto-correct) may allow contraction, but must use a different assertion.
inline void assertRestorationInvariant(int bs, int nc) {
    if (nc < bs) {
        captureDebugSnapshot("INVARIANT_BROKEN: VN_RESTORE_CONTRACTION", ...);
    }
}
```

### 5.2 Architectural Decision: Composed Preference
We explicitly chose to fallback to `TypingWord[]` (composed data) when `_stateIndex` is desynchronized (too short).

**Rationale:**
1.  **UX Priority:** User prefers a potentially "too composed" word (â) over a truncated word (a) and lost characters.
2.  **Semantic Lie:** We acknowledge that `hNewCharCount = safeStateIndex` is a "white lie" to the engine to prevent data loss.
3.  **Future Mordenization:** This logic should eventually be replaced by a `RestorePlan` object with explicit `Raw` vs `Composed` source types.

### 5.2 OpenKey.cpp - Defensive Logging
```cpp
if (pData->backspaceCount > 0 && pData->newCharCount != pData->backspaceCount) {
    if (PerformanceLogger::isEnabled()) {
        char warnBuf[128];
        sprintf_s(warnBuf, "WARNING: bs/nc mismatch bs=%d nc=%d",
            pData->backspaceCount, pData->newCharCount);
        PerformanceLogger::log(warnBuf, 0);
    }
}
```

---

## 6. Alternative Approaches Considered

### 6.1 Force `_stateIndex = _index` Sync
- **Pros:** Simpler
- **Cons:** May hide other bugs, loses raw keystroke data

### 6.2 Reject Operation When Mismatch
- **Pros:** Safe, no data corruption
- **Cons:** User experience degraded (word not processed)

### 6.3 Always Use `_index` (chosen approach variation)
- **Pros:** Defensive, guarantees no char loss
- **Cons:** May restore composed chars instead of raw keystrokes

---

## 7. Questions for Senior Review

1. **Design Intent:** Is the `_stateIndex != _index` scenario intentional in any code path? Should we add explicit invariant assertions?

2. **Root Cause vs Symptom:** The fix addresses the symptom (char loss). Should we investigate deeper into WHY `_stateIndex` becomes less than `_index`?

3. **Performance:** The `max()` comparison adds minimal overhead. Is this acceptable for the hot path?

4. **Testing:** Should we add unit tests for `checkRestoreIfWrongSpelling()` with various `_index` vs `_stateIndex` combinations?

5. **Fallback Logic:** When `_stateIndex < _index`, we fall back to existing `TypingWord[i]`. Is this the correct behavior, or should we abort the operation?

---

## 8. Appendix: Files Changed

| File | Change |
|------|--------|
| `engine/Engine.cpp` | Core fix in `checkRestoreIfWrongSpelling()` |
| `engine/DebugSnapshot.h` | New: Tripwire snapshot structure |
| `engine/DebugSnapshot.cpp` | New: Snapshot capture & auto-save |
| `win32/OpenKey/OpenKey.cpp` | Tripwires B,C,D,E + defensive logging |
| `Tools/stress_test.ps1` | New: Bug hunter script |
