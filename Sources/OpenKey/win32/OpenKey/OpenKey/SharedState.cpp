/*----------------------------------------------------------
NextKey - Vietnamese Keyboard Input Method

SharedState implementation using Windows Shared Memory.
Uses Local\ namespace (session-scoped, no admin required).

Copyright (C) 2024 Phat Mai
-----------------------------------------------------------*/
#include "stdafx.h"
#include "SharedState.h"
#include <windows.h>

// Shared memory layout - all fields use volatile LONG for Interlocked ops
// alignas(4) ensures proper alignment for Interlocked operations (defensive)
struct SharedStateData {
    alignas(4) volatile LONG version;      // Incremented on any change
    alignas(4) volatile LONG language;     // 0=English, 1=Vietnamese
    alignas(4) volatile LONG inputType;    // 0=Telex, 1=VNI, etc.
    alignas(4) volatile LONG codeTable;    // 0=Unicode, 1=TCVN3, etc.
    DWORD mainProcessId;                   // For subprocess orphan detection
    DWORD reserved[4];                     // Future expansion
};

// Object names - Local\ namespace = session-scoped, no admin required
static const wchar_t* SHARED_MEM_NAME = L"Local\\NextKeySharedState";
static const wchar_t* CONFIG_EVENT_NAME = L"Local\\NextKeyConfigReload";

SharedState& SharedState::instance() {
    static SharedState s_instance;
    return s_instance;
}

SharedState::~SharedState() {
    shutdown();
}

bool SharedState::init(bool isMainProcess) {
    m_isMain = isMainProcess;
    
    if (m_isMain) {
        // Main process: CREATE shared memory
        m_hMapFile = CreateFileMappingW(
            INVALID_HANDLE_VALUE,    // Use paging file
            NULL,                    // Default security
            PAGE_READWRITE,          // Read/Write access
            0,                       // High DWORD of size
            sizeof(SharedStateData), // Low DWORD of size
            SHARED_MEM_NAME);
        
        if (!m_hMapFile) {
            LOG(L"[SharedState] CreateFileMapping failed: %d\n", GetLastError());
            return false;
        }
        
        bool alreadyExists = (GetLastError() == ERROR_ALREADY_EXISTS);
        
        // Map view of file
        m_pData = (SharedStateData*)MapViewOfFile(
            m_hMapFile,
            FILE_MAP_ALL_ACCESS,
            0, 0,
            sizeof(SharedStateData));
        
        if (!m_pData) {
            LOG(L"[SharedState] MapViewOfFile failed: %d\n", GetLastError());
            CloseHandle(m_hMapFile);
            m_hMapFile = nullptr;
            return false;
        }
        
        // Initialize data if this is first creation
        if (!alreadyExists) {
            memset((void*)m_pData, 0, sizeof(SharedStateData));
            m_pData->mainProcessId = GetCurrentProcessId();
            LOG(L"[SharedState] Created new shared memory, PID=%d\n", m_pData->mainProcessId);
        } else {
            LOG(L"[SharedState] Attached to existing shared memory\n");
        }
        
        // Create config change event (auto-reset)
        m_hConfigEvent = CreateEventW(NULL, FALSE, FALSE, CONFIG_EVENT_NAME);
        if (!m_hConfigEvent) {
            LOG(L"[SharedState] CreateEvent failed: %d\n", GetLastError());
        }
        
    } else {
        // Subprocess: OPEN existing shared memory
        m_hMapFile = OpenFileMappingW(
            FILE_MAP_ALL_ACCESS,
            FALSE,
            SHARED_MEM_NAME);
        
        if (!m_hMapFile) {
            LOG(L"[SharedState] OpenFileMapping failed: %d (main not running?)\n", GetLastError());
            return false;
        }
        
        m_pData = (SharedStateData*)MapViewOfFile(
            m_hMapFile,
            FILE_MAP_ALL_ACCESS,
            0, 0,
            sizeof(SharedStateData));
        
        if (!m_pData) {
            LOG(L"[SharedState] MapViewOfFile failed: %d\n", GetLastError());
            CloseHandle(m_hMapFile);
            m_hMapFile = nullptr;
            return false;
        }
        
        // Open config change event
        m_hConfigEvent = OpenEventW(SYNCHRONIZE, FALSE, CONFIG_EVENT_NAME);
        // Note: May fail if event not created yet, that's OK
        
        LOG(L"[SharedState] Subprocess attached, mainPID=%d\n", m_pData->mainProcessId);
    }
    
    return true;
}

void SharedState::shutdown() {
    if (m_pData) {
        UnmapViewOfFile(m_pData);
        m_pData = nullptr;
    }
    if (m_hMapFile) {
        CloseHandle(m_hMapFile);
        m_hMapFile = nullptr;
    }
    if (m_hConfigEvent) {
        CloseHandle(m_hConfigEvent);
        m_hConfigEvent = nullptr;
    }
    LOG(L"[SharedState] Shutdown complete\n");
}

// === Lock-free Getters/Setters using Interlocked ===

int SharedState::getLanguage() const {
    if (!m_pData) return 1;  // Default: Vietnamese
    return InterlockedCompareExchange(&m_pData->language, 0, 0);
}

void SharedState::setLanguage(int lang) {
    if (!m_pData) return;
    InterlockedExchange(&m_pData->language, lang);
    InterlockedIncrement(&m_pData->version);
}

int SharedState::getInputType() const {
    if (!m_pData) return 0;  // Default: Telex
    return InterlockedCompareExchange(&m_pData->inputType, 0, 0);
}

void SharedState::setInputType(int type) {
    if (!m_pData) return;
    InterlockedExchange(&m_pData->inputType, type);
    InterlockedIncrement(&m_pData->version);
}

int SharedState::getCodeTable() const {
    if (!m_pData) return 0;  // Default: Unicode
    return InterlockedCompareExchange(&m_pData->codeTable, 0, 0);
}

void SharedState::setCodeTable(int code) {
    if (!m_pData) return;
    InterlockedExchange(&m_pData->codeTable, code);
    InterlockedIncrement(&m_pData->version);
}

int SharedState::getVersion() const {
    if (!m_pData) return 0;
    return InterlockedCompareExchange(&m_pData->version, 0, 0);
}

// === Event Signaling ===

void SharedState::signalConfigChanged() {
    if (m_hConfigEvent) {
        SetEvent(m_hConfigEvent);
        LOG(L"[SharedState] Config change signaled\n");
    }
}

bool SharedState::waitForConfigChange(unsigned long ms) {
    if (!m_hConfigEvent) return false;
    return WaitForSingleObject(m_hConfigEvent, ms) == WAIT_OBJECT_0;
}
