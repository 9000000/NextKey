/*----------------------------------------------------------
NextKey - The Modern Vietnamese Input Method Engine.
Based on OpenKey architecture.

ConfigIntent.h - IPC message types for Central Writer Architecture

Dialogs should never write config. They only express intent.
The main process owns persistence.

Copyright (C) 2026 NextKey Project
Author: Mai Tan Phat
License: GPL (Inherited from OpenKey)
-----------------------------------------------------------*/
#pragma once

#include <cstdint>
#include <vector>
#include <string>

// ============================================================
// Central Writer Invariant
// ============================================================
// 
// Config modification flow:
// 1. Subprocess (dialog) collects user input
// 2. Subprocess serializes intent and sends via WM_COPYDATA
// 3. Main process receives, merges into ConfigManager
// 4. Main process debounces and saves to disk
//
// NEVER call ConfigManager::save() from subprocess!
// 
// ============================================================

// IPC Intent Types
enum class ConfigIntentType : uint32_t {
    UPDATE_SETTINGS = 1,
    UPDATE_MACROS = 2,
    UPDATE_EXCLUDED_APPS = 3,
    UPDATE_SPECIAL_APPS = 4,
    UPDATE_CLIPBOARD_APPS = 5,
    UPDATE_CONVERT_TOOL = 6,
};

// Version header for future compatibility
// Payload layout: [IntentHeader][ActualPayload...]
struct IntentHeader {
    uint16_t version;   // Start with 1
    uint16_t reserved;  // For alignment and future use
};

#define CONFIG_INTENT_VERSION 1

// ============================================================
// Settings Payload (UPDATE_SETTINGS)
// ============================================================
// All fields are fixed-size for reliable IPC
#pragma pack(push, 1)
struct SettingsPayload {
    // General Tab
    int32_t language;
    int32_t inputType;
    int32_t codeTable;
    int32_t switchKey;
    uint8_t smartSwitch;
    
    // Typing Tab
    uint8_t checkSpelling;
    uint8_t restoreWrongSpelling;
    uint8_t modernOrthography;
    uint8_t fixRecommendBrowser;
    uint8_t upperCaseFirstChar;
    uint8_t allowZwfj;
    uint8_t tempOffSpelling;
    uint8_t tempOffOpenKey;
    uint8_t rememberCode;
    
    // Macro Tab (settings only, macros sent separately)
    uint8_t macroEnabled;
    uint8_t macroInEnglish;
    uint8_t autoCapsMacro;
    uint8_t quickTelex;
    uint8_t quickStartConsonant;
    uint8_t quickEndConsonant;
    uint8_t tempOffMacro;  // ESC key to skip macro expansion
    
    // System Tab
    uint8_t runWithWindows;
    uint8_t runAsAdmin;
    uint8_t checkNewVersion;
    uint8_t createDesktopShortcut;
    uint8_t supportMetroApp;
    uint8_t fixChromiumBrowser;
    uint8_t useClipboard;
    int32_t iconStyle;
    int32_t customColorV;
    int32_t customColorE;
    uint8_t showOnStartup;
    uint8_t showAdvancedSettings;
    int32_t backgroundOpacity;
    int32_t blurMode;
    
    // Excluded Apps
    uint8_t excludeAppsEnabled;
    
    // Debug
    uint8_t enablePerfLog;
    uint8_t reduceMemory;  // EmptyWorkingSet toggle for Settings dialog
};
#pragma pack(pop)

// ============================================================
// Macros Payload (UPDATE_MACROS)
// ============================================================
// Format: [IntentHeader][count:uint16][entries...]
// Each entry: [keyLen:uint8][keyData...][valLen:uint8][valData...]
// Encoding: UTF-8
// Max key/value length: 255 bytes

// ============================================================
// Convert Tool Payload (UPDATE_CONVERT_TOOL)
// ============================================================
#pragma pack(push, 1)
struct ConvertToolPayload {
    int32_t hotkey;
    int32_t fromCode;
    int32_t toCode;
    uint8_t toAllCaps;
    uint8_t toAllNonCaps;
    uint8_t removeMark;
    uint8_t toCapsEachWord;
    uint8_t toCapsFirstLetter;
    uint8_t dontAlertCompleted;
    uint8_t autoPasteReselect;  // Quick Convert auto-paste + reselect toggle
    uint8_t sequentialMode;     // Convert tuần tự - cycle through options one by one
};
#pragma pack(pop)

// ============================================================
// Excluded Apps Payload (UPDATE_EXCLUDED_APPS)
// ============================================================
// Format: [IntentHeader][appCount:uint16][apps...][switchCount:uint16][switchData...]
// Each app: [len:uint8][name...]
// Each switch entry: [len:uint8][appName...][lang:int8]

// ============================================================
// Serialization Helpers
// ============================================================

// Serialize excluded apps + smartSwitch data
inline std::vector<uint8_t> serializeExcludedApps(
    const std::vector<std::string>& apps,
    const std::map<std::string, int>& smartSwitchData) {
    
    std::vector<uint8_t> buffer;
    
    // Header
    IntentHeader header = { CONFIG_INTENT_VERSION, 0 };
    buffer.insert(buffer.end(), (uint8_t*)&header, (uint8_t*)&header + sizeof(header));
    
    // Apps count
    uint16_t appCount = static_cast<uint16_t>(apps.size());
    buffer.insert(buffer.end(), (uint8_t*)&appCount, (uint8_t*)&appCount + sizeof(appCount));
    
    // Apps
    for (const auto& app : apps) {
        uint8_t len = static_cast<uint8_t>(app.size() > 255 ? 255 : app.size());
        buffer.push_back(len);
        buffer.insert(buffer.end(), app.begin(), app.begin() + len);
    }
    
    // SmartSwitch count
    uint16_t switchCount = static_cast<uint16_t>(smartSwitchData.size());
    buffer.insert(buffer.end(), (uint8_t*)&switchCount, (uint8_t*)&switchCount + sizeof(switchCount));
    
    // SmartSwitch data
    for (const auto& [appName, lang] : smartSwitchData) {
        uint8_t len = static_cast<uint8_t>(appName.size() > 255 ? 255 : appName.size());
        buffer.push_back(len);
        buffer.insert(buffer.end(), appName.begin(), appName.begin() + len);
        int8_t langVal = static_cast<int8_t>(lang);
        buffer.push_back(static_cast<uint8_t>(langVal));
    }
    
    return buffer;
}

// Deserialize excluded apps + smartSwitch
inline bool deserializeExcludedApps(
    const uint8_t* data, size_t size,
    std::vector<std::string>& outApps,
    std::map<std::string, int>& outSmartSwitch) {
    
    outApps.clear();
    outSmartSwitch.clear();
    
    if (size < sizeof(IntentHeader) + sizeof(uint16_t)) {
        return false;
    }
    
    size_t cursor = 0;
    
    // Header
    IntentHeader header;
    memcpy(&header, data + cursor, sizeof(header));
    cursor += sizeof(header);
    
    if (header.version != CONFIG_INTENT_VERSION) {
        return false;
    }
    
    // Apps count
    uint16_t appCount;
    memcpy(&appCount, data + cursor, sizeof(appCount));
    cursor += sizeof(appCount);
    
    // Apps
    for (uint16_t i = 0; i < appCount; i++) {
        if (cursor >= size) return false;
        uint8_t len = data[cursor++];
        if (cursor + len > size) return false;
        std::string app((char*)(data + cursor), len);
        cursor += len;
        outApps.push_back(app);
    }
    
    // SmartSwitch count
    if (cursor + sizeof(uint16_t) > size) return false;
    uint16_t switchCount;
    memcpy(&switchCount, data + cursor, sizeof(switchCount));
    cursor += sizeof(switchCount);
    
    // SmartSwitch data
    for (uint16_t i = 0; i < switchCount; i++) {
        if (cursor >= size) return false;
        uint8_t len = data[cursor++];
        if (cursor + len + 1 > size) return false;
        std::string appName((char*)(data + cursor), len);
        cursor += len;
        int8_t langVal = static_cast<int8_t>(data[cursor++]);
        outSmartSwitch[appName] = static_cast<int>(langVal);
    }
    
    return true;
}

// Serialize macros list to binary format for WM_COPYDATA
inline std::vector<uint8_t> serializeMacros(const std::vector<std::pair<std::string, std::string>>& macros) {
    std::vector<uint8_t> buffer;
    
    // Header
    IntentHeader header = { CONFIG_INTENT_VERSION, 0 };
    buffer.insert(buffer.end(), (uint8_t*)&header, (uint8_t*)&header + sizeof(header));
    
    // Count
    uint16_t count = static_cast<uint16_t>(macros.size());
    buffer.insert(buffer.end(), (uint8_t*)&count, (uint8_t*)&count + sizeof(count));
    
    // Entries
    for (const auto& [key, val] : macros) {
        // Key length (max 255)
        uint8_t keyLen = static_cast<uint8_t>(key.size() > 255 ? 255 : key.size());
        buffer.push_back(keyLen);
        buffer.insert(buffer.end(), key.begin(), key.begin() + keyLen);
        
        // Value length (max 255)
        uint8_t valLen = static_cast<uint8_t>(val.size() > 255 ? 255 : val.size());
        buffer.push_back(valLen);
        buffer.insert(buffer.end(), val.begin(), val.begin() + valLen);
    }
    
    return buffer;
}

// Deserialize macros from binary format
inline bool deserializeMacros(const uint8_t* data, size_t size, 
                               std::vector<std::pair<std::string, std::string>>& outMacros) {
    outMacros.clear();
    
    // Minimum: header + count
    if (size < sizeof(IntentHeader) + sizeof(uint16_t)) {
        return false;
    }
    
    size_t cursor = 0;
    
    // Header
    IntentHeader header;
    memcpy(&header, data + cursor, sizeof(header));
    cursor += sizeof(header);
    
    if (header.version != CONFIG_INTENT_VERSION) {
        return false;  // Version mismatch
    }
    
    // Count
    uint16_t count;
    memcpy(&count, data + cursor, sizeof(count));
    cursor += sizeof(count);
    
    // Entries
    for (uint16_t i = 0; i < count; i++) {
        if (cursor >= size) return false;
        
        // Key
        uint8_t keyLen = data[cursor++];
        if (cursor + keyLen > size) return false;
        std::string key((char*)(data + cursor), keyLen);
        cursor += keyLen;
        
        // Value
        if (cursor >= size) return false;
        uint8_t valLen = data[cursor++];
        if (cursor + valLen > size) return false;
        std::string val((char*)(data + cursor), valLen);
        cursor += valLen;
        
        outMacros.push_back({ key, val });
    }
    
    return true;
}

// Serialize settings to binary
inline std::vector<uint8_t> serializeSettings(const SettingsPayload& settings) {
    std::vector<uint8_t> buffer;
    
    // Header
    IntentHeader header = { CONFIG_INTENT_VERSION, 0 };
    buffer.insert(buffer.end(), (uint8_t*)&header, (uint8_t*)&header + sizeof(header));
    
    // Payload
    buffer.insert(buffer.end(), (uint8_t*)&settings, (uint8_t*)&settings + sizeof(settings));
    
    return buffer;
}

// Deserialize settings from binary
inline bool deserializeSettings(const uint8_t* data, size_t size, SettingsPayload& outSettings) {
    if (size < sizeof(IntentHeader) + sizeof(SettingsPayload)) {
        return false;
    }
    
    IntentHeader header;
    memcpy(&header, data, sizeof(header));
    
    if (header.version != CONFIG_INTENT_VERSION) {
        return false;
    }
    
    memcpy(&outSettings, data + sizeof(header), sizeof(outSettings));
    return true;
}

// Serialize ConvertTool settings
inline std::vector<uint8_t> serializeConvertTool(const ConvertToolPayload& payload) {
    std::vector<uint8_t> buffer;
    
    IntentHeader header = { CONFIG_INTENT_VERSION, 0 };
    buffer.insert(buffer.end(), (uint8_t*)&header, (uint8_t*)&header + sizeof(header));
    buffer.insert(buffer.end(), (uint8_t*)&payload, (uint8_t*)&payload + sizeof(payload));
    
    return buffer;
}

// Deserialize ConvertTool settings
inline bool deserializeConvertTool(const uint8_t* data, size_t size, ConvertToolPayload& outPayload) {
    if (size < sizeof(IntentHeader) + sizeof(ConvertToolPayload)) {
        return false;
    }
    
    IntentHeader header;
    memcpy(&header, data, sizeof(header));
    
    if (header.version != CONFIG_INTENT_VERSION) {
        return false;
    }
    
    memcpy(&outPayload, data + sizeof(header), sizeof(outPayload));
    return true;
}

// ============================================================
// IPC Send Helper
// ============================================================
// Usage: sendConfigIntent(mainWnd, ConfigIntentType::UPDATE_MACROS, buffer);

#ifdef _WINDOWS_
inline bool sendConfigIntent(HWND mainWnd, ConfigIntentType type, const std::vector<uint8_t>& payload) {
    if (!mainWnd) return false;
    
    COPYDATASTRUCT cds;
    cds.dwData = static_cast<ULONG_PTR>(type);
    cds.cbData = static_cast<DWORD>(payload.size());
    cds.lpData = (void*)payload.data();
    
    // SendMessage ensures payload stays valid during call
    return SendMessage(mainWnd, WM_COPYDATA, 0, (LPARAM)&cds) != 0;
}
#endif
