/*----------------------------------------------------------
NextKey - The Modern Vietnamese Input Method Engine.
Based on OpenKey architecture.

Copyright (C) 2026 NextKey Project
Author: Mai Tan Phat
License: GPL (Inherited from OpenKey)
-----------------------------------------------------------*/
#pragma once
#include <windows.h>

// Extract sciter.dll from embedded resources (Release) or use local file (Debug)
// Returns path to usable DLL
// Uses Named Mutex for multi-instance synchronization
bool EnsureSciterDll(LPCWSTR& outPath);
