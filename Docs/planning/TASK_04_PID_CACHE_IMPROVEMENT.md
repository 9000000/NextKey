# Task 4: PID Cache Improvement

## Status
**Done** ✅

## Goal
Prevent stale cache entries when Windows reuses PIDs.

## Problem
Cache stores process names by PID. Windows can reuse PIDs after process exits → wrong app names for Smart Switch.

## Solution Implemented
Enhanced cache with **TTL (5s) + HWND validation**:

### Safety Guarantees
| Scenario | Behavior | Delay |
|----------|----------|-------|
| Different window (HWND) | Instant cache miss | **0ms** |
| Same window, TTL expired | Re-query from OS | 0ms |
| PID reuse with new window | Caught by HWND mismatch | **0ms** |
| Same window, TTL valid | Return cached value | 0ms |

### Trade-off
Multi-window apps (Chrome): Alt-Tab between 2 Chrome windows = extra query (acceptable).

---

## Changes Made

### File: `OpenKeyHelper.cpp`

#### 1. New Struct (line ~40)
```cpp
// Before:
static std::unordered_map<DWORD, std::string> _processNameCache;

// After:
struct ProcessCacheEntry {
    std::string name;
    DWORD lastCheckTime;  // GetTickCount() when cached
    HWND hwnd;            // Window handle to detect PID reuse
};
static std::unordered_map<DWORD, ProcessCacheEntry> _processNameCache;
static const DWORD CACHE_TTL_MS = 5000;  // 5 second TTL
```

#### 2. Cache Lookup (line ~232)
```cpp
// Before: Simple lookup, no validation
if (cacheIt != _processNameCache.end()) {
    _exeNameUtf8 = cacheIt->second;
    return _exeNameUtf8;
}

// After: Validate HWND + TTL
if (cacheIt != _processNameCache.end()) {
    if (cacheIt->second.hwnd == _tempWnd && 
        (GetTickCount() - cacheIt->second.lastCheckTime) < CACHE_TTL_MS) {
        _exeNameUtf8 = cacheIt->second.name;
        return _exeNameUtf8;
    }
    // Cache invalid - will re-query below
}
```

#### 3. Cache Insert (line ~272)
```cpp
// Before:
_processNameCache[_tempProcessId] = _exeNameUtf8;

// After:
_processNameCache[_tempProcessId] = {_exeNameUtf8, GetTickCount(), _tempWnd};
```

## Effort
~30 minutes (including review discussion)
