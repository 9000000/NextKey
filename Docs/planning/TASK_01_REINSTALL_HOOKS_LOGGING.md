# Task 1: Debug Logging for ReinstallHooks()

## Status
**Done** ✅

## Goal
Add timing and state logs to help diagnose issues when hooks fail to reinstall.

## Background
`ReinstallHooks()` in `OpenKey.cpp` uses `Sleep(100)` which is arbitrary. In rare cases, hooks might not be fully released before reinstalling. Debug logging will help identify if this is ever an issue.

## Changes

### File: OpenKey.cpp

Location: `ReinstallHooks()` function (line ~112)

```cpp
void ReinstallHooks() {
    PERF_START_SECTION(reinstall);  // ADD
    
    static std::mutex reinstallMutex;
    std::lock_guard<std::mutex> lock(reinstallMutex);
    
    OutputDebugString(_T("OpenKey: ReinstallHooks - Starting...\n"));
    
    // ... existing unhook code ...
    
    Sleep(100);
    
    // ADD: Log state after resync
    if (PerformanceLogger::isEnabled()) {
        char stateLog[256];
        sprintf_s(stateLog, "ReinstallHooks: _flag=0x%X after resync", _flag);
        PerformanceLogger::log("REINSTALL_HOOKS", 0);
    }
    
    // ... existing rehook code ...
    
    PERF_END_SECTION(reinstall, "REINSTALL_HOOKS_TOTAL");  // ADD
}
```

## Effort
~15 minutes
