/*----------------------------------------------------------
NextKey - The Modern Vietnamese Input Method Engine.
Based on OpenKey architecture.

Copyright (C) 2026 NextKey Project
Author: Mai Tan Phat
License: GPL (Inherited from OpenKey)
-----------------------------------------------------------*/
//
//  DebugSnapshot.cpp
//  OpenKey - Tripwire debugging implementation
//

#include "DebugSnapshot.h"
#include "Engine.h"
#include <cstring>
#include <cstdio>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#else
#include <ctime>
#endif

// Static snapshot storage - only one snapshot per session
static DebugSnapshot _debugSnapshot;
static bool _snapshotCaptured = false;

// Extern declarations for engine state
extern vKeyHookState HookState;
extern Uint32 TypingWord[MAX_BUFF];
extern Byte _index;
extern int vSendKeyStepByStep;

void initDebugSnapshot() {
    memset(&_debugSnapshot, 0, sizeof(DebugSnapshot));
    _snapshotCaptured = false;
}

// Auto-save snapshot to debug folder next to exe
static void autoSaveSnapshot() {
#ifdef _WIN32
    // Get exe directory path
    TCHAR exePath[MAX_PATH];
    GetModuleFileName(NULL, exePath, MAX_PATH);
    
    // Remove exe filename to get directory
    std::wstring path(exePath);
    size_t lastSlash = path.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos) {
        path = path.substr(0, lastSlash + 1);
    }
    
    // Create debug folder if not exists
    std::wstring debugDir = path + L"debug";
    CreateDirectoryW(debugDir.c_str(), NULL);
    
    // Build filepath
    std::wstring filepath = debugDir + L"\\debug_snapshot.json";
    
    std::ofstream file(filepath);
    if (!file.is_open()) return;
    
    // Write JSON format for easy analysis
    file << "{\n";
    file << "  \"reason\": \"" << _debugSnapshot.reason << "\",\n";
    file << "  \"app\": \"" << _debugSnapshot.app << "\",\n";
    file << "  \"timestamp\": " << _debugSnapshot.timestamp << ",\n";
    file << "  \"index\": " << _debugSnapshot.index << ",\n";
    file << "  \"wordLen\": " << _debugSnapshot.wordLen << ",\n";
    file << "  \"bsCount\": " << _debugSnapshot.bsCount << ",\n";
    file << "  \"newCharCount\": " << _debugSnapshot.newCharCount << ",\n";
    file << "  \"code\": " << _debugSnapshot.code << ",\n";
    file << "  \"stepByStep\": " << (_debugSnapshot.stepByStep ? "true" : "false") << ",\n";
    file << "  \"injectionMethod\": " << _debugSnapshot.injectionMethod << ",\n";
    file << "  \"imeCachedState\": " << (_debugSnapshot.imeCachedState ? "true" : "false") << ",\n";
    file << "  \"elapsedMs\": " << _debugSnapshot.elapsedMs << ",\n";
    
    // Write typingWord as hex array
    file << "  \"typingWord\": [";
    for (int i = 0; i < _debugSnapshot.wordLen && i < MAX_BUFF; i++) {
        if (i > 0) file << ", ";
        file << "\"0x" << std::hex << _debugSnapshot.typingWord[i] << std::dec << "\"";
    }
    file << "]\n";
    file << "}\n";
    
    file.close();
#endif
}

void captureDebugSnapshot(const char* reason, const char* app, double elapsedMs, int injectionMethod) {
    // Only capture FIRST invariant violation (forensic approach)
    if (_snapshotCaptured) return;
    _snapshotCaptured = true;
    
    // Copy reason and app name (use safe string functions for MSVC)
#ifdef _WIN32
    strncpy_s(_debugSnapshot.reason, sizeof(_debugSnapshot.reason), reason, _TRUNCATE);
    strncpy_s(_debugSnapshot.app, sizeof(_debugSnapshot.app), app, _TRUNCATE);
#else
    strncpy(_debugSnapshot.reason, reason, 63);
    _debugSnapshot.reason[63] = '\0';
    strncpy(_debugSnapshot.app, app, 31);
    _debugSnapshot.app[31] = '\0';
#endif
    
    // Copy engine state
    memcpy(_debugSnapshot.typingWord, TypingWord, sizeof(Uint32) * MAX_BUFF);
    _debugSnapshot.wordLen = _index;
    _debugSnapshot.index = _index;
    _debugSnapshot.bsCount = HookState.backspaceCount;
    _debugSnapshot.newCharCount = HookState.newCharCount;
    _debugSnapshot.code = HookState.code;
    _debugSnapshot.stepByStep = (vSendKeyStepByStep != 0);
    _debugSnapshot.injectionMethod = injectionMethod;
    _debugSnapshot.imeCachedState = false;  // Passed from OpenKey.cpp context
    _debugSnapshot.elapsedMs = elapsedMs;
    
#ifdef _WIN32
    _debugSnapshot.timestamp = GetTickCount64();
#else
    _debugSnapshot.timestamp = (uint64_t)time(nullptr) * 1000;
#endif
    
    // Immediately log to performance logger if available
    extern void logTripwire(const char* msg);
    char logMsg[256];
    snprintf(logMsg, sizeof(logMsg), 
        "TRIPWIRE[%s] app=%s idx=%d bs=%d nc=%d code=%d elapsed=%.2fms",
        reason, app, _debugSnapshot.index, _debugSnapshot.bsCount,
        _debugSnapshot.newCharCount, _debugSnapshot.code, elapsedMs);
    logTripwire(logMsg);
    
    // Auto-save to debug folder
    autoSaveSnapshot();
}

bool hasDebugSnapshot() {
    return _snapshotCaptured;
}

void resetDebugSnapshot() {
    _snapshotCaptured = false;
    memset(&_debugSnapshot, 0, sizeof(DebugSnapshot));
}

void saveSnapshotToFile(const char* basePath) {
    if (!_snapshotCaptured) return;
    
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s\\debug_snapshot.json", basePath);
    
    std::ofstream file(filepath);
    if (!file.is_open()) return;
    
    // Write JSON format for easy analysis
    file << "{\n";
    file << "  \"reason\": \"" << _debugSnapshot.reason << "\",\n";
    file << "  \"app\": \"" << _debugSnapshot.app << "\",\n";
    file << "  \"timestamp\": " << _debugSnapshot.timestamp << ",\n";
    file << "  \"index\": " << _debugSnapshot.index << ",\n";
    file << "  \"wordLen\": " << _debugSnapshot.wordLen << ",\n";
    file << "  \"bsCount\": " << _debugSnapshot.bsCount << ",\n";
    file << "  \"newCharCount\": " << _debugSnapshot.newCharCount << ",\n";
    file << "  \"code\": " << _debugSnapshot.code << ",\n";
    file << "  \"stepByStep\": " << (_debugSnapshot.stepByStep ? "true" : "false") << ",\n";
    file << "  \"injectionMethod\": " << _debugSnapshot.injectionMethod << ",\n";
    file << "  \"imeCachedState\": " << (_debugSnapshot.imeCachedState ? "true" : "false") << ",\n";
    file << "  \"elapsedMs\": " << _debugSnapshot.elapsedMs << ",\n";
    
    // Write typingWord as hex array
    file << "  \"typingWord\": [";
    for (int i = 0; i < _debugSnapshot.wordLen && i < MAX_BUFF; i++) {
        if (i > 0) file << ", ";
        file << "\"0x" << std::hex << _debugSnapshot.typingWord[i] << std::dec << "\"";
    }
    file << "]\n";
    file << "}\n";
    
    file.close();
}

// Helper to get engine index (for external access)
int getEngineIndex() {
    return _index;
}
