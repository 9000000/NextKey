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
#include "stdafx.h"
#include "PerformanceLogger.h"
#include "OpenKeyHelper.h"
#include <ctime>
#include <sstream>
#include <iomanip>

// Static member initialization
HANDLE PerformanceLogger::logFile = INVALID_HANDLE_VALUE;
bool PerformanceLogger::enabled = false;
std::wstring PerformanceLogger::logPath;
std::wstring PerformanceLogger::logDir;
CRITICAL_SECTION PerformanceLogger::cs;
bool PerformanceLogger::initialized = false;
int PerformanceLogger::currentLogDay = 0;

void PerformanceLogger::init() {
    if (initialized) return;
    
    InitializeCriticalSection(&cs);
    
    // Get exe directory path
    TCHAR exePath[MAX_PATH];
    GetModuleFileName(NULL, exePath, MAX_PATH);
    
    // Remove exe filename to get directory
    std::wstring path(exePath);
    size_t lastSlash = path.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos) {
        path = path.substr(0, lastSlash + 1);
    }
    
    // Create debug folder
    logDir = path + L"debug";
    CreateDirectoryW(logDir.c_str(), NULL);  // Ignore error if already exists
    
    // Log path will be set dynamically based on current date
    updateLogPath();
    initialized = true;
    
    // Don't open file yet - only open when enabled
}

void PerformanceLogger::updateLogPath() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    
    wchar_t filename[64];
    swprintf_s(filename, L"openkey_perf_%04d-%02d-%02d.log", st.wYear, st.wMonth, st.wDay);
    logPath = logDir + L"\\" + filename;
    currentLogDay = st.wDay;
}

void PerformanceLogger::shutdown() {
    if (!initialized) return;
    
    EnterCriticalSection(&cs);
    
    if (logFile != INVALID_HANDLE_VALUE) {
        CloseHandle(logFile);
        logFile = INVALID_HANDLE_VALUE;
    }
    
    LeaveCriticalSection(&cs);
    DeleteCriticalSection(&cs);
    initialized = false;
}

void PerformanceLogger::log(const char* tag, double elapsedMs) {
    if (!enabled || !initialized) return;
    
    EnterCriticalSection(&cs);
    
    // Check if day has changed - need new log file
    SYSTEMTIME st;
    GetLocalTime(&st);
    if (st.wDay != currentLogDay) {
        // Close current file and open new one
        if (logFile != INVALID_HANDLE_VALUE) {
            CloseHandle(logFile);
            logFile = INVALID_HANDLE_VALUE;
        }
        updateLogPath();
    }
    
    // Open file if not already open
    if (logFile == INVALID_HANDLE_VALUE) {
        logFile = CreateFile(
            logPath.c_str(),
            GENERIC_WRITE,
            FILE_SHARE_READ,  // Allow reading while we write
            NULL,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
        
        if (logFile != INVALID_HANDLE_VALUE) {
            // Move to end of file for append
            SetFilePointer(logFile, 0, NULL, FILE_END);
            
            // Write header if file is new/empty
            DWORD fileSize = GetFileSize(logFile, NULL);
            if (fileSize == 0) {
                char header[128];
                int hlen = snprintf(header, sizeof(header), 
                    "=== OpenKey Performance Log - %04d-%02d-%02d ===\r\n",
                    st.wYear, st.wMonth, st.wDay);
                DWORD written;
                WriteFile(logFile, header, hlen, &written, NULL);
            }
        }
    }
    
    if (logFile == INVALID_HANDLE_VALUE) {
        LeaveCriticalSection(&cs);
        return;
    }
    
    // st is already declared at the beginning of the function
    
    // Format log entry
    char buffer[256];
    int len = snprintf(buffer, sizeof(buffer),
        "[%04d-%02d-%02d %02d:%02d:%02d.%03d] %s: %.3fms\r\n",
        st.wYear, st.wMonth, st.wDay,
        st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
        tag, elapsedMs);
    
    // Write to file
    DWORD written;
    WriteFile(logFile, buffer, len, &written, NULL);
    
    // Flush immediately to ensure data is written
    FlushFileBuffers(logFile);
    
    LeaveCriticalSection(&cs);
}

bool PerformanceLogger::isEnabled() {
    return enabled && initialized;
}

void PerformanceLogger::setEnabled(bool value) {
    if (!initialized) {
        init();
    }
    
    EnterCriticalSection(&cs);
    
    if (value && !enabled) {
        // Turning on - log start message
        enabled = true;
        
        // Force open file
        if (logFile == INVALID_HANDLE_VALUE) {
            logFile = CreateFile(
                logPath.c_str(),
                GENERIC_WRITE,
                FILE_SHARE_READ,
                NULL,
                OPEN_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL
            );
            if (logFile != INVALID_HANDLE_VALUE) {
                SetFilePointer(logFile, 0, NULL, FILE_END);
            }
        }
        
        if (logFile != INVALID_HANDLE_VALUE) {
            SYSTEMTIME st;
            GetLocalTime(&st);
            char buffer[128];
            int len = snprintf(buffer, sizeof(buffer),
                "\r\n=== Logging started at %04d-%02d-%02d %02d:%02d:%02d ===\r\n",
                st.wYear, st.wMonth, st.wDay,
                st.wHour, st.wMinute, st.wSecond);
            DWORD written;
            WriteFile(logFile, buffer, len, &written, NULL);
            FlushFileBuffers(logFile);
        }
    } else if (!value && enabled) {
        // Turning off - log stop message and close file
        if (logFile != INVALID_HANDLE_VALUE) {
            SYSTEMTIME st;
            GetLocalTime(&st);
            char buffer[128];
            int len = snprintf(buffer, sizeof(buffer),
                "=== Logging stopped at %04d-%02d-%02d %02d:%02d:%02d ===\r\n",
                st.wYear, st.wMonth, st.wDay,
                st.wHour, st.wMinute, st.wSecond);
            DWORD written;
            WriteFile(logFile, buffer, len, &written, NULL);
            FlushFileBuffers(logFile);
            
            CloseHandle(logFile);
            logFile = INVALID_HANDLE_VALUE;
        }
        enabled = false;
    }
    
    LeaveCriticalSection(&cs);
}

std::wstring PerformanceLogger::getLogPath() {
    if (!initialized) {
        init();
    }
    return logPath;
}

void PerformanceLogger::openLogFolder() {
    if (!initialized) {
        init();
    }
    
    // Get directory from log path
    std::wstring dir = logPath;
    size_t lastSlash = dir.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos) {
        dir = dir.substr(0, lastSlash);
    }
    
    // Open in Explorer
    ShellExecute(NULL, L"open", L"explorer.exe", dir.c_str(), NULL, SW_SHOWNORMAL);
}
