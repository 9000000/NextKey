/*----------------------------------------------------------
NextKey - The Modern Vietnamese Input Method Engine.
Based on OpenKey architecture.

Copyright (C) 2026 NextKey Project
Author: Mai Tan Phat
License: GPL (Inherited from OpenKey)
-----------------------------------------------------------*/
#pragma once
#include "stdafx.h"
#include <fstream>
#include <string>

// Performance logging threshold in milliseconds
// Only log entries that exceed this threshold to avoid bloating the log file
#define PERF_LOG_THRESHOLD_MS 15.0

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
    
    // Log a debug message (for diagnostics, no timing threshold)
    // @param tag: identifier for the log entry (e.g., "QC_START", "QC_DETECT")
    // @param message: descriptive message
    static void logDebug(const char* tag, const char* message);
    
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

// Debug logging macros for QuickConvert diagnostics (always logs, no threshold)
#define DEBUG_LOG(tag, msg) \
    if(PerformanceLogger::isEnabled()) { \
        PerformanceLogger::logDebug(tag, msg); \
    }

#define DEBUG_LOG_FMT(tag, fmt, ...) \
    if(PerformanceLogger::isEnabled()) { \
        char _dbg_buf[512]; \
        snprintf(_dbg_buf, sizeof(_dbg_buf), fmt, __VA_ARGS__); \
        PerformanceLogger::logDebug(tag, _dbg_buf); \
    }

// Helper to convert wide string to narrow for logging (truncates to maxLen chars)
inline std::string wideToNarrowForLog(const std::wstring& ws, size_t maxLen = 50) {
    if (ws.empty()) return "(empty)";
    std::wstring truncated = ws.substr(0, (std::min)(ws.length(), maxLen));
    if (ws.length() > maxLen) truncated += L"...";
    
    // Convert to UTF-8
    int size = WideCharToMultiByte(CP_UTF8, 0, truncated.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size <= 0) return "(conversion_error)";
    
    std::string result(size - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, truncated.c_str(), -1, &result[0], size, nullptr, nullptr);
    return result;
}
