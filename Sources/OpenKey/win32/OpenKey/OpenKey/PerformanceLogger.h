/*----------------------------------------------------------
OpenKey - The Cross platform Open source Vietnamese Keyboard application.

Copyright (C) 2019 Mai Vu Tuyen
Contact: maivutuyen.91@gmail.com
Github: https://github.com/tuyenvm/OpenKey
Fanpage: https://www.facebook.com/OpenKeyVN

This file is belong to the OpenKey project, Win32 version
which is released under GPL license.
You can fork, modify, improve this program. If you
redistribute your new version, it MUST be open source.
-----------------------------------------------------------*/
#pragma once
#include "stdafx.h"
#include <fstream>
#include <string>

// Performance logging threshold in milliseconds
// Only log entries that exceed this threshold to avoid bloating the log file
#define PERF_LOG_THRESHOLD_MS 5.0

class PerformanceLogger {
public:
    // Initialize the logger - opens log file in exe directory
    static void init();
    
    // Shutdown the logger - closes log file
    static void shutdown();
    
    // Log a performance entry
    // @param tag: identifier for what was measured (e.g., "HOOK_TOTAL", "IME_CHECK")
    // @param elapsedMs: time elapsed in milliseconds
    static void log(const char* tag, double elapsedMs);
    
    // Check if logging is enabled
    static bool isEnabled();
    
    // Enable or disable logging at runtime
    static void setEnabled(bool enabled);
    
    // Get the log file path
    static std::wstring getLogPath();
    
    // Open the log folder in Explorer
    static void openLogFolder();

private:
    static HANDLE logFile;
    static bool enabled;
    static std::wstring logPath;
    static std::wstring logDir;       // debug folder path
    static CRITICAL_SECTION cs;       // Thread safety for file writes
    static bool initialized;
    static int currentLogDay;         // Track current log file day
    
    static void updateLogPath();      // Update logPath based on current date
};

// Convenience macros for timing code blocks
// Usage:
//   PERF_START();
//   ... code to measure ...
//   PERF_END("TAG_NAME");

#define PERF_START() \
    LARGE_INTEGER _perfStart, _perfEnd, _perfFreq; \
    if(PerformanceLogger::isEnabled()) { \
        QueryPerformanceCounter(&_perfStart); \
    }

#define PERF_END(tag) \
    if(PerformanceLogger::isEnabled()) { \
        QueryPerformanceCounter(&_perfEnd); \
        QueryPerformanceFrequency(&_perfFreq); \
        double _ms = (double)(_perfEnd.QuadPart - _perfStart.QuadPart) * 1000.0 / _perfFreq.QuadPart; \
        if(_ms > PERF_LOG_THRESHOLD_MS) { \
            PerformanceLogger::log(tag, _ms); \
        } \
    }

// Variant for measuring a specific section without affecting outer timing
#define PERF_START_SECTION(name) \
    LARGE_INTEGER _perfStart_##name, _perfEnd_##name, _perfFreq_##name; \
    if(PerformanceLogger::isEnabled()) { \
        QueryPerformanceCounter(&_perfStart_##name); \
    }

#define PERF_END_SECTION(name, tag) \
    if(PerformanceLogger::isEnabled()) { \
        QueryPerformanceCounter(&_perfEnd_##name); \
        QueryPerformanceFrequency(&_perfFreq_##name); \
        double _ms_##name = (double)(_perfEnd_##name.QuadPart - _perfStart_##name.QuadPart) * 1000.0 / _perfFreq_##name.QuadPart; \
        if(_ms_##name > PERF_LOG_THRESHOLD_MS) { \
            PerformanceLogger::log(tag, _ms_##name); \
        } \
    }
