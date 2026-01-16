# Hot Path Optimization Plan - Tầng 4

## Context

Current IPC handler làm **quá nhiều việc**:
```cpp
case ConfigIntentType::UPDATE_MACROS:
    deserializeMacros(data, size, macros);       // ✅ OK
    config.setMacros(macros);                    // ✅ OK
    initMacrosFromList(macros);                  // ⚠️ Engine work!
```

**Vấn đề:**
- Engine update trong message handler → block UI thread
- Buffer serialization không reserve → multiple realloc

**Mức độ:** ⚠️ **Non-critical** - UX hiện tại OK, nhưng là future pressure point

---

## Problem 1: Engine Update in IPC Handler

### Current State

**WM_COPYDATA handler (SystemTrayHelper.cpp):**
```cpp
case UPDATE_MACROS:
    deserializeMacros(data, size, macros);
    config.setMacros(macros);
    initMacrosFromList(macros);           // ⚠️ Blocking call
    
case UPDATE_EXCLUDED_APPS:
    deserializeExcludedApps(...);
    config.setStringArray(...);
    initEnglishOnlyAppsFromList(apps);    // ⚠️ Blocking call
    initSmartSwitchKeyFromMap(data);      // ⚠️ Blocking call
```

### Why This Matters

**Current volume:** Low, UX unaffected
**Risk scenarios:**
- Import 100 macros từ file → 100 intents → 100 engine rebuilds
- Plugin automation → burst intents
- Future background sync

### Proposed Solution A: Defer Engine Update (Preferred)

**Principle:** IPC handler chỉ update **config memory**, engine update sau khi **save()**

```cpp
// WM_COPYDATA handler - FAST
case UPDATE_MACROS:
    deserializeMacros(data, size, macros);
    config.setMacros(macros);
    // NO engine update here!

// Timer fires → save() - SLOW OK
case WM_TIMER if TIMER_CONFIG_SAVE:
    if (s_configDirty) {
        config.save();
        s_configDirty = false;
        
        // NOW update engine (data already persisted)
        reloadEngineFromConfig();  // Helper function
    }
```

**Ưu điểm:**
- IPC handler mỏng, nhanh
- Engine update chỉ 1 lần (sau gộp nhiều intents)
- Data đã safe trên disk trước khi update engine

**Trade-off:**
- Engine state lag 0-1.5s so với config
- Cần thêm `reloadEngineFromConfig()` helper

### Proposed Solution B: Async Engine Update

```cpp
case UPDATE_MACROS:
    config.setMacros(macros);
    PostMessage(hWnd, WM_USER + 201, 0, 0);  // Async engine update

case WM_USER + 201:
    reloadEngineFromConfig();
```

**Ưu điểm:** IPC return ngay
**Trade-off:** Phức tạp hơn, thêm message type

---

## Problem 2: Buffer Serialization without Reserve

### Current State

**ConfigIntent.h:**
```cpp
inline std::vector<uint8_t> serializeMacros(...) {
    std::vector<uint8_t> buffer;
    // No reserve! ❌
    
    buffer.insert(buffer.end(), ...);  // Realloc 1
    buffer.insert(buffer.end(), ...);  // Realloc 2
    buffer.push_back(...);             // Realloc 3
}
```

### Performance Impact

**Macro list nhỏ (< 50 items):** Negligible
**Future import 1000 macros:** Noticeable

### Proposed Solution: Reserve with Estimation

```cpp
inline std::vector<uint8_t> serializeMacros(
    const std::vector<std::pair<std::string, std::string>>& macros) {
    
    std::vector<uint8_t> buffer;
    
    // Estimate: header + count + (avg key 10 + avg val 30) * count
    size_t estimated = sizeof(IntentHeader) + sizeof(uint16_t) + 
                       macros.size() * 50;
    buffer.reserve(estimated);
    
    // ... rest unchanged
}
```

**Similar for:**
- `serializeSettings()` - trivial, fixed size
- `serializeExcludedApps()` - estimate app name length
- `serializeMacros()` - estimate key/value length

---

## Implementation Priority

| Task | Impact | Effort | Priority |
|------|--------|--------|----------|
| Add buffer.reserve() | Low | 10 min | **P2** (easy win) |
| Defer engine update | Medium | 1-2 hours | **P1** (architecture) |
| Async engine update | Medium | 2-3 hours | **P3** (optional) |

### Recommended Order

**Phase 1 (Now - Quick Win):**
1. Add `buffer.reserve()` in all serialization functions
2. Measure improvement (probably negligible but clean)

**Phase 2 (Future - When Needed):**
3. Move engine update to post-save callback
4. Add `reloadEngineFromConfig()` helper
5. Test with burst intent scenarios

---

## Files to Modify

### Phase 1: Buffer Reserve

**[ConfigIntent.h](file:///wsl.localhost/Ubuntu-24.04/home/phatmt/code/OpenKey/Sources/OpenKey/win32/OpenKey/OpenKey/ConfigIntent.h)**
- `serializeMacros()` - add reserve
- `serializeExcludedApps()` - add reserve  
- `serializeSettings()` - not needed (fixed size)

### Phase 2: Defer Engine Update

**[SystemTrayHelper.cpp](file:///wsl.localhost/Ubuntu-24.04/home/phatmt/code/OpenKey/Sources/OpenKey/win32/OpenKey/OpenKey/SystemTrayHelper.cpp)**
- Remove `initMacrosFromList()` from WM_COPYDATA
- Remove `initEnglishOnlyAppsFromList()` from WM_COPYDATA
- Add `reloadEngineFromConfig()` helper
- Call helper in TIMER_CONFIG_SAVE after save()

---

## Testing Strategy

**Phase 1:**
```
Measure: Time to serialize 100 macros
Before: ~X μs
After: ~X μs (expect 10-20% improvement)
```

**Phase 2:**
```
Test: Send 10 intents in 100ms (simulated burst)
Verify: Engine updated once after debounce
Verify: IPC handler returns <1ms
```

---

## Verdict

**Do Phase 1?** ✅ Yes - "easy win", good discipline
**Do Phase 2?** ⏸️ Not urgent - defer until:
- Import macro feature added
- Background sync added
- User reports lag

**Current state:** ✅ **Safe to ship as-is**
