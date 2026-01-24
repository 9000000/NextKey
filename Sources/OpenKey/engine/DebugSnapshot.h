/*----------------------------------------------------------
NextKey - The Modern Vietnamese Input Method Engine.
Based on OpenKey architecture.

Copyright (C) 2026 NextKey Project
Author: Mai Tan Phat
License: GPL (Inherited from OpenKey)
-----------------------------------------------------------*/
//
//  DebugSnapshot.h
//  OpenKey - Tripwire debugging for rare timing bugs
//
//  Created for bug investigation: cungx → cungx instead of cũng
//

#ifndef DebugSnapshot_h
#define DebugSnapshot_h

#include "DataType.h"
#include <cstdint>

// Snapshot structure for capturing engine state when invariant is violated
struct DebugSnapshot {
    char app[32];           // Current app name
    char reason[64];        // Tripwire that fired
    
    Uint32 typingWord[MAX_BUFF];  // Current typing buffer
    int wordLen;            // Length of word in buffer
    int index;              // Current _index value
    
    int bsCount;            // Backspace count from HookState
    int newCharCount;       // New char count from HookState
    int code;               // HookState.code (vDoNothing, vWillProcess, etc.)
    
    bool stepByStep;        // vSendKeyStepByStep setting
    int injectionMethod;    // -1=SendInput, 0=ShiftIns, 1=CtrlV, 2=SendInputDelay
    bool imeCachedState;    // Cached IME state
    
    double elapsedMs;       // Elapsed time for operation (for timing tripwires)
    uint64_t timestamp;     // GetTickCount64() when snapshot captured
};

// Tripwire functions
void initDebugSnapshot();
void captureDebugSnapshot(const char* reason, const char* app, double elapsedMs, int injectionMethod);
bool hasDebugSnapshot();
void resetDebugSnapshot();
void saveSnapshotToFile(const char* basePath);

// Engine state access (implemented in Engine.cpp)
int getEngineIndex();
bool getEngineCachedImeState();

#endif /* DebugSnapshot_h */
