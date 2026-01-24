/*----------------------------------------------------------
NextKey - The Modern Vietnamese Input Method Engine.
Based on OpenKey architecture.

SharedState - Cross-process state sharing via Windows Shared Memory
Replaces HWND_BROADCAST for IPC between main process and UI dialogs.

Uses Local\ namespace (session-scoped, no admin required).

Copyright (C) 2026 NextKey Project
Author: Mai Tan Phat
License: GPL (Inherited from OpenKey)
-----------------------------------------------------------*/
#pragma once

// PIMPL - hide Windows headers from includers  
struct SharedStateData;

class SharedState {
public:
    static SharedState& instance();

    // Lifecycle
    // isMainProcess: true = Core (creates), false = UI subprocess (opens)
    bool init(bool isMainProcess);
    void shutdown();

    // === State Accessors (Lock-free via Interlocked) ===
    int getLanguage() const;        // 0=English, 1=Vietnamese
    void setLanguage(int lang);
    
    int getInputType() const;       // 0=Telex, 1=VNI, etc.
    void setInputType(int type);
    
    int getCodeTable() const;       // 0=Unicode, 1=TCVN3, etc.
    void setCodeTable(int code);
    
    // Version counter - incremented on any state change
    // UI can poll this to detect updates
    int getVersion() const;
    
    // === Change Signaling ===
    void signalConfigChanged();             // Main: signal config.toml was modified
    bool waitForConfigChange(unsigned long ms); // UI: wait for config change event
    
    // Debug helper
    bool isValid() const { return m_pData != nullptr; }

private:
    SharedState() = default;
    ~SharedState();
    
    // Non-copyable
    SharedState(const SharedState&) = delete;
    SharedState& operator=(const SharedState&) = delete;

    void* m_hMapFile = nullptr;      // HANDLE to file mapping
    void* m_hConfigEvent = nullptr;  // HANDLE to config change event
    SharedStateData* m_pData = nullptr;
    bool m_isMain = false;
};
