/*----------------------------------------------------------
NextKey - Vietnamese Keyboard Input Method

ConfigManager - TOML-based configuration management
Uses PIMPL idiom to isolate toml++ from rest of codebase.

Copyright (C) 2024 Phat Mai
-----------------------------------------------------------*/
#pragma once

// MINIMAL includes only - NO Windows.h, NO polluted headers
#include <string>
#include <vector>
#include <map>

// Forward declaration - hides toml++ implementation
struct ConfigInternal;

class ConfigManager {
public:
    // Singleton access
    static ConfigManager& instance();
    
    // Lifecycle
    ConfigManager();
    ~ConfigManager();
    
    // === Initialization ===
    bool init();                    // Detect path, load config
    bool isPortableMode() const { return m_isPortable; }
    bool needsMigration() const { return m_needsMigration; }  // True if config.toml didn't exist
    std::wstring getConfigPath() const { return m_configPath; }
    
    // === Load/Save ===
    bool load();                    // Load from config.toml
    
    // CENTRAL WRITER INVARIANT:
    // Only the MAIN PROCESS should call save().
    // Subprocesses (dialogs) should send intents via WM_COPYDATA.
    // See ConfigIntent.h for the IPC protocol.
    bool save();                    // Save all to config.toml
    
    bool saveIfDirty();             // Save only if changes pending
    void markDirty() { m_dirty = true; }
    
    // === Int Settings ===
    int getInt(const char* section, const char* key, int defaultVal = 0);
    void setInt(const char* section, const char* key, int val);
    
    // === Bool Settings ===
    bool getBool(const char* section, const char* key, bool defaultVal = false);
    void setBool(const char* section, const char* key, bool val);
    
    // === String Settings ===
    std::string getString(const char* section, const char* key, const char* defaultVal = "");
    void setString(const char* section, const char* key, const std::string& val);
    
    // === String Array (for app lists) ===
    std::vector<std::string> getStringArray(const char* section, const char* key);
    void setStringArray(const char* section, const char* key, const std::vector<std::string>& arr);
    bool hasArrayKey(const char* section, const char* key);  // Check if key exists (even if empty)
    
    // === Macro Data ===
    std::vector<std::pair<std::string, std::string>> getMacros();
    void setMacros(const std::vector<std::pair<std::string, std::string>>& macros);
    
    // === Smart Switch Data ===
    std::map<std::string, int> getSmartSwitchData();
    void setSmartSwitchData(const std::map<std::string, int>& data);
    
    // === Clipboard Apps Configuration ===
    // Struct to store per-app clipboard settings
    struct ClipboardAppConfig {
        std::string exeName;
        int method = 0;      // 0 = ShiftInsert, 1 = CtrlV
        int delayMs = 0;     // Delay after paste (0-500ms)
    };
    std::vector<ClipboardAppConfig> getClipboardApps();
    void setClipboardApps(const std::vector<ClipboardAppConfig>& apps);
    
    // === Migration ===
    static bool migrateFromRegistry();  // One-time migration from old Registry
    
private:
    // Non-copyable
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    
    // State (primitive types only)
    std::wstring m_configPath;
    bool m_isPortable = false;
    bool m_dirty = false;
    bool m_initialized = false;
    bool m_needsMigration = false;  // True when init() found no config.toml
    
    // PIMPL - hides all toml++ details
    ConfigInternal* m_impl = nullptr;
};

// Convenience macros for migration compatibility
#define CONFIG_GET_INT(section, key, def) ConfigManager::instance().getInt(section, key, def)
#define CONFIG_SET_INT(section, key, val) ConfigManager::instance().setInt(section, key, val)
#define CONFIG_GET_BOOL(section, key, def) ConfigManager::instance().getBool(section, key, def)
#define CONFIG_SET_BOOL(section, key, val) ConfigManager::instance().setBool(section, key, val)
