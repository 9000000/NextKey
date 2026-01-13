/*----------------------------------------------------------
NextKey - Vietnamese Keyboard Input Method

ConfigManager - TOML-based configuration management implementation
Uses PIMPL idiom to isolate toml++ from rest of codebase.

CRITICAL: toml++ is included FIRST before any other headers
to avoid namespace conflicts with "using namespace std" in legacy code.

Copyright (C) 2024 Phat Mai
-----------------------------------------------------------*/

// ============================================================
// STEP 1: Fix C++17 conflicts BEFORE any includes
// ============================================================
#define _HAS_STD_BYTE 0
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

// ============================================================
// STEP 2: Include toml++ FIRST - before any polluted headers
// ============================================================
#define TOML_EXCEPTIONS 0  // Use error codes instead of exceptions
#include "../../../../../lib/tomlplusplus/toml.hpp"

// ============================================================
// STEP 3: Now safe to include Windows headers
// ============================================================
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Shlobj.h>
#include <Shlwapi.h>

#pragma comment(lib, "Shlwapi.lib")

// ============================================================
// STEP 4: Include our header LAST
// ============================================================
#include "ConfigManager.h"

#include <fstream>
#include <mutex>
#include <sstream>

// ============================================================
// PIMPL Implementation - Hidden from header
// ============================================================
struct ConfigInternal {
    toml::table root;
    std::mutex mtx;
    
    // Cached data for fast access
    std::map<std::string, std::map<std::string, int>> intCache;
    std::map<std::string, std::map<std::string, bool>> boolCache;
    std::map<std::string, std::map<std::string, std::string>> stringCache;
    std::map<std::string, std::map<std::string, std::vector<std::string>>> arrayCache;
    std::vector<std::pair<std::string, std::string>> macros;
    std::map<std::string, int> smartSwitch;
};

// ============================================================
// Singleton
// ============================================================
ConfigManager& ConfigManager::instance() {
    static ConfigManager inst;
    return inst;
}

ConfigManager::ConfigManager() {
    m_impl = new ConfigInternal();
}

ConfigManager::~ConfigManager() {
    // Auto-save on destruction if dirty
    if (m_dirty && m_impl) {
        save();
    }
    delete m_impl;
    m_impl = nullptr;
}

// ============================================================
// Path Detection
// ============================================================
static std::wstring detectConfigPath(bool& outIsPortable) {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    PathRemoveFileSpecW(exePath);
    
    // Check if we can write to exe directory (portable mode)
    std::wstring portablePath = std::wstring(exePath) + L"\\config.toml";
    std::wstring testFile = std::wstring(exePath) + L"\\__nextkey_write_test.tmp";
    
    HANDLE hFile = CreateFileW(testFile.c_str(), GENERIC_WRITE, 0, NULL, 
                               CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
        DeleteFileW(testFile.c_str());
        outIsPortable = true;
        return portablePath;
    }
    
    // Fallback: %LocalAppData%\NextKey\config.toml
    wchar_t appData[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, appData))) {
        std::wstring appDataPath = std::wstring(appData) + L"\\NextKey";
        CreateDirectoryW(appDataPath.c_str(), NULL);
        outIsPortable = false;
        return appDataPath + L"\\config.toml";
    }
    
    // Last fallback
    outIsPortable = true;
    return L".\\config.toml";
}

static std::string wideToUtf8(const std::wstring& wide) {
    if (wide.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string utf8(size - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, &utf8[0], size, nullptr, nullptr);
    return utf8;
}

// ============================================================
// Initialization
// ============================================================
bool ConfigManager::init() {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    
    if (m_initialized) return true;
    
    m_configPath = detectConfigPath(m_isPortable);
    m_initialized = true;
    
    // Check if config file exists
    bool fileExists = PathFileExistsW(m_configPath.c_str());
    
    if (fileExists) {
        // File exists → just load it, no migration needed
        std::string utf8Path = wideToUtf8(m_configPath);
        auto result = toml::parse_file(utf8Path);
        if (result) {
            m_impl->root = std::move(result.table());
            // Parse into caches
            for (auto& [sectionName, sectionVal] : m_impl->root) {
                if (!sectionVal.is_table()) continue;
                
                std::string section(sectionName.str());
                
                if (section == "smartSwitchData") {
                    for (auto& [appName, langVal] : *sectionVal.as_table()) {
                        if (langVal.is_integer()) {
                            m_impl->smartSwitch[std::string(appName.str())] = static_cast<int>(langVal.as_integer()->get());
                        }
                    }
                    continue;
                }
                
                if (section == "macros") continue; // Handle separately
                
                for (auto& [keyName, value] : *sectionVal.as_table()) {
                    std::string key(keyName.str());
                    
                    if (value.is_integer()) {
                        m_impl->intCache[section][key] = static_cast<int>(value.as_integer()->get());
                    } else if (value.is_boolean()) {
                        m_impl->boolCache[section][key] = value.as_boolean()->get();
                    } else if (value.is_string()) {
                        m_impl->stringCache[section][key] = std::string(value.as_string()->get());
                    } else if (value.is_array()) {
                        std::vector<std::string> arr;
                        for (auto& elem : *value.as_array()) {
                            if (elem.is_string()) {
                                arr.push_back(std::string(elem.as_string()->get()));
                            }
                        }
                        m_impl->arrayCache[section][key] = arr;
                    }
                }
            }
            
            // Parse macros array
            if (auto* macrosNode = m_impl->root.get("macros")) {
                if (macrosNode->is_array()) {
                    for (auto& macroNode : *macrosNode->as_array()) {
                        if (macroNode.is_table()) {
                            auto* tbl = macroNode.as_table();
                            if (tbl->contains("key") && tbl->contains("value")) {
                                auto* keyNode = tbl->get("key");
                                auto* valNode = tbl->get("value");
                                if (keyNode && valNode && keyNode->is_string() && valNode->is_string()) {
                                    m_impl->macros.push_back({
                                        std::string(keyNode->as_string()->get()),
                                        std::string(valNode->as_string()->get())
                                    });
                                }
                            }
                        }
                    }
                }
            }
        }
    } else {
        // File does NOT exist → migrate from Registry and create file
        // Need to unlock before calling migrateFromRegistry which may lock
        // So we mark a flag and handle after
        m_needsMigration = true;
    }
    
    return true;
}

bool ConfigManager::load() {
    m_initialized = false;
    m_impl->intCache.clear();
    m_impl->boolCache.clear();
    m_impl->stringCache.clear();
    m_impl->arrayCache.clear();
    m_impl->macros.clear();
    m_impl->smartSwitch.clear();
    return init();
}

// ============================================================
// Save
// ============================================================
bool ConfigManager::save() {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    
    if (!m_initialized) return false;
    
    // Custom serialization to maintain UI tab ordering with comments
    std::wstring tempPath = m_configPath + L".tmp";
    std::ofstream file(tempPath.c_str());
    if (!file) {
        return false;
    }
    
    file << "# =============================================\n";
    file << "# NextKey Configuration\n";
    file << "# =============================================\n\n";
    
    // Helper to decode switchKey to human-readable format
    // Format: [keyChar(8bit)][unused(8bit)][beep(1bit)][shift(1bit)][win(1bit)][alt(1bit)][ctrl(1bit)][unused(3bit)][vkCode(8bit)]
    auto decodeSwitchKey = [](int value) -> std::string {
        std::string result;
        int keyChar = (value >> 24) & 0xFF;
        bool beep = (value & 0x8000) != 0;
        bool shift = (value & 0x800) != 0;
        bool win = (value & 0x400) != 0;
        bool alt = (value & 0x200) != 0;
        bool ctrl = (value & 0x100) != 0;
        
        if (ctrl) result += "Ctrl+";
        if (alt) result += "Alt+";
        if (win) result += "Win+";
        if (shift) result += "Shift+";
        
        if (keyChar >= 32 && keyChar < 127) {
            result += (char)keyChar;
        } else if (!result.empty() && result.back() == '+') {
            // Remove trailing + if no key character
            result.pop_back();
        }
        
        if (beep) result += " (beep)";
        if (result.empty()) result = "(none)";
        return result;
    };
    
    // Helper to decode hotkey (similar format: modifiers + key)
    auto decodeHotkey = [](int value) -> std::string {
        if (value == 0 || value == 0x7FFFFFFF) return "None";
        
        std::string result;
        int keyChar = (value >> 24) & 0xFF;
        bool shift = (value & 0x800) != 0;
        bool win = (value & 0x400) != 0;
        bool alt = (value & 0x200) != 0;
        bool ctrl = (value & 0x100) != 0;
        
        if (ctrl) result += "Ctrl+";
        if (alt) result += "Alt+";
        if (win) result += "Win+";
        if (shift) result += "Shift+";
        
        if (keyChar >= 32 && keyChar < 127) {
            result += (char)keyChar;
        }
        return result.empty() ? "None" : result;
    };
    
    // Helper to write a section with its values
    auto writeSection = [&](const std::string& section, const std::string& comment) {
        file << "# --- " << comment << " ---\n";
        file << "[" << section << "]\n";
        
        // Int values (with special handling for switchKey and hotkey)
        auto intSecIt = m_impl->intCache.find(section);
        if (intSecIt != m_impl->intCache.end()) {
            for (auto& [key, val] : intSecIt->second) {
                file << key << " = " << val;
                // Add human-readable comment for special keys
                if (key == "switchKey") {
                    file << "  # " << decodeSwitchKey(val);
                } else if (key == "hotkey") {
                    file << "  # " << decodeHotkey(val);
                }
                file << "\n";
            }
        }
        
        // Bool values
        auto boolSecIt = m_impl->boolCache.find(section);
        if (boolSecIt != m_impl->boolCache.end()) {
            for (auto& [key, val] : boolSecIt->second) {
                file << key << " = " << (val ? "true" : "false") << "\n";
            }
        }
        
        // String values
        auto strSecIt = m_impl->stringCache.find(section);
        if (strSecIt != m_impl->stringCache.end()) {
            for (auto& [key, val] : strSecIt->second) {
                file << key << " = \"" << val << "\"\n";
            }
        }
        
        // Array values
        auto arrSecIt = m_impl->arrayCache.find(section);
        if (arrSecIt != m_impl->arrayCache.end()) {
            for (auto& [key, arr] : arrSecIt->second) {
                file << key << " = [";
                for (size_t i = 0; i < arr.size(); i++) {
                    file << "\"" << arr[i] << "\"";
                    if (i < arr.size() - 1) file << ", ";
                }
                file << "]\n";
            }
        }
        
        file << "\n";
    };
    
    // Write sections in UI tab order
    writeSection("general", "General Tab");
    writeSection("typing", "Typing Tab (Bộ gõ)");
    writeSection("macro", "Macro Tab (Gõ tắt)");
    
    // Serialize macros as array of tables (right after [macro] section)
    if (!m_impl->macros.empty()) {
        for (auto& [key, val] : m_impl->macros) {
            file << "[[macros]]\n";
            file << "key = \"" << key << "\"\n";
            file << "value = \"" << val << "\"\n\n";
        }
    }
    
    writeSection("system", "System Tab (Hệ thống)");
    writeSection("excludedApps", "Excluded Apps");
    writeSection("specialApps", "Special Apps");
    writeSection("convertTool", "Convert Tool");
    writeSection("debug", "Debug");
    
    // Serialize smart switch data
    if (!m_impl->smartSwitch.empty()) {
        file << "# --- Smart Switch (per-app language) ---\n";
        file << "[smartSwitchData]\n";
        for (auto& [app, lang] : m_impl->smartSwitch) {
            file << "\"" << app << "\" = " << lang << "\n";
        }
        file << "\n";
    }
    
    file.close();
    
    // Atomic rename
    DeleteFileW(m_configPath.c_str());
    if (!MoveFileW(tempPath.c_str(), m_configPath.c_str())) {
        return false;
    }
    
    m_dirty = false;
    return true;
}

bool ConfigManager::saveIfDirty() {
    if (m_dirty) {
        return save();
    }
    return true;
}

// ============================================================
// Getters/Setters
// ============================================================
int ConfigManager::getInt(const char* section, const char* key, int defaultVal) {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    
    auto secIt = m_impl->intCache.find(section);
    if (secIt != m_impl->intCache.end()) {
        auto keyIt = secIt->second.find(key);
        if (keyIt != secIt->second.end()) {
            return keyIt->second;
        }
    }
    return defaultVal;
}

void ConfigManager::setInt(const char* section, const char* key, int val) {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    m_impl->intCache[section][key] = val;
    m_dirty = true;
}

bool ConfigManager::getBool(const char* section, const char* key, bool defaultVal) {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    
    auto secIt = m_impl->boolCache.find(section);
    if (secIt != m_impl->boolCache.end()) {
        auto keyIt = secIt->second.find(key);
        if (keyIt != secIt->second.end()) {
            return keyIt->second;
        }
    }
    return defaultVal;
}

void ConfigManager::setBool(const char* section, const char* key, bool val) {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    m_impl->boolCache[section][key] = val;
    m_dirty = true;
}

std::string ConfigManager::getString(const char* section, const char* key, const char* defaultVal) {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    
    auto secIt = m_impl->stringCache.find(section);
    if (secIt != m_impl->stringCache.end()) {
        auto keyIt = secIt->second.find(key);
        if (keyIt != secIt->second.end()) {
            return keyIt->second;
        }
    }
    return defaultVal ? defaultVal : "";
}

void ConfigManager::setString(const char* section, const char* key, const std::string& val) {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    m_impl->stringCache[section][key] = val;
    m_dirty = true;
}

std::vector<std::string> ConfigManager::getStringArray(const char* section, const char* key) {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    
    auto secIt = m_impl->arrayCache.find(section);
    if (secIt != m_impl->arrayCache.end()) {
        auto keyIt = secIt->second.find(key);
        if (keyIt != secIt->second.end()) {
            return keyIt->second;
        }
    }
    return {};
}

void ConfigManager::setStringArray(const char* section, const char* key, const std::vector<std::string>& arr) {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    m_impl->arrayCache[section][key] = arr;
    m_dirty = true;
}

bool ConfigManager::hasArrayKey(const char* section, const char* key) {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    
    auto secIt = m_impl->arrayCache.find(section);
    if (secIt != m_impl->arrayCache.end()) {
        return secIt->second.find(key) != secIt->second.end();
    }
    return false;
}

std::vector<std::pair<std::string, std::string>> ConfigManager::getMacros() {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    return m_impl->macros;
}

void ConfigManager::setMacros(const std::vector<std::pair<std::string, std::string>>& macros) {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    m_impl->macros = macros;
    m_dirty = true;
}

std::map<std::string, int> ConfigManager::getSmartSwitchData() {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    return m_impl->smartSwitch;
}

void ConfigManager::setSmartSwitchData(const std::map<std::string, int>& data) {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    // Only set dirty if data actually changed
    if (m_impl->smartSwitch != data) {
        m_impl->smartSwitch = data;
        m_dirty = true;
    }
}

// ============================================================
// Migration from Registry → config.toml
// ============================================================

// Registry key path - read from original OpenKey location
static LPCTSTR REGISTRY_KEY = TEXT("SOFTWARE\\TuyenMai\\OpenKey");

// Helper to read DWORD from Registry
static int readRegInt(LPCTSTR key, int defaultVal) {
    HKEY hKey;
    int val = defaultVal;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, REGISTRY_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD size = sizeof(val);
        if (RegQueryValueEx(hKey, key, 0, 0, (LPBYTE)&val, &size) != ERROR_SUCCESS) {
            val = defaultVal;
        }
        RegCloseKey(hKey);
    }
    return val;
}

// Helper to read binary from Registry
static std::vector<BYTE> readRegBinary(LPCTSTR key) {
    std::vector<BYTE> result;
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, REGISTRY_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD size = 0;
        RegQueryValueEx(hKey, key, 0, 0, 0, &size);
        if (size > 0) {
            result.resize(size);
            if (RegQueryValueEx(hKey, key, 0, 0, result.data(), &size) != ERROR_SUCCESS) {
                result.clear();
            }
        }
        RegCloseKey(hKey);
    }
    return result;
}

// Helper to read string from Registry
static std::wstring readRegStringW(LPCTSTR key, LPCTSTR defaultVal) {
    HKEY hKey;
    TCHAR buffer[4096] = {0};
    if (RegOpenKeyEx(HKEY_CURRENT_USER, REGISTRY_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD size = sizeof(buffer);
        DWORD type = 0;
        if (RegQueryValueEx(hKey, key, NULL, &type, (LPBYTE)buffer, &size) != ERROR_SUCCESS
            || (type != REG_SZ && type != REG_EXPAND_SZ)) {
            wcscpy_s(buffer, defaultVal);
        }
        RegCloseKey(hKey);
    } else {
        wcscpy_s(buffer, defaultVal);
    }
    return std::wstring(buffer);
}

bool ConfigManager::migrateFromRegistry() {
    auto& cfg = ConfigManager::instance();
    
    // If config.toml already has data, skip migration
    if (cfg.m_initialized && !cfg.m_impl->intCache.empty()) {
        return true;  // Already migrated
    }
    
    // Initialize if not done
    if (!cfg.m_initialized) {
        cfg.init();
    }
    
    // Check if Registry has data (vInputType is a reliable indicator)
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, REGISTRY_KEY, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return true;  // No Registry key, nothing to migrate
    }
    DWORD testVal = 0, testSize = sizeof(testVal);
    // Use vLanguage as indicator - it's always present in old registry
    // (vInputType may not exist if user never changed it from default)
    bool hasData = (RegQueryValueEx(hKey, L"vLanguage", 0, 0, (LPBYTE)&testVal, &testSize) == ERROR_SUCCESS);
    RegCloseKey(hKey);
    
    if (!hasData) {
        return true;  // No data in Registry
    }
    
    // === General Tab ===
    cfg.setInt("general", "language", readRegInt(L"vLanguage", 1));
    cfg.setInt("general", "inputType", readRegInt(L"vInputType", 0));
    cfg.setInt("general", "codeTable", readRegInt(L"vCodeTable", 0));
    cfg.setInt("general", "switchKey", readRegInt(L"vSwitchKeyStatus", 0x7A000206));
    cfg.setBool("general", "smartSwitch", readRegInt(L"vUseSmartSwitchKey", 1) != 0);
    
    // === Typing Tab (Bộ gõ) ===
    cfg.setBool("typing", "checkSpelling", readRegInt(L"vCheckSpelling", 1) != 0);
    cfg.setBool("typing", "restoreWrongSpelling", readRegInt(L"vRestoreIfWrongSpelling", 1) != 0);
    cfg.setBool("typing", "modernOrthography", readRegInt(L"vUseModernOrthography", 0) != 0);
    cfg.setBool("typing", "fixRecommendBrowser", readRegInt(L"vFixRecommendBrowser", 1) != 0);
    cfg.setBool("typing", "upperCaseFirstChar", readRegInt(L"vUpperCaseFirstChar", 0) != 0);
    cfg.setBool("typing", "allowZwfj", readRegInt(L"vAllowConsonantZFWJ", 0) != 0);
    cfg.setBool("typing", "tempOffSpellingCtrl", readRegInt(L"vTempOffSpelling", 0) != 0);
    cfg.setBool("typing", "tempOffOpenKeyAlt", readRegInt(L"vTempOffOpenKey", 0) != 0);
    cfg.setBool("typing", "rememberCode", readRegInt(L"vRememberCode", 1) != 0);
    
    // === Macro Tab (Gõ tắt) ===
    cfg.setBool("macro", "enabled", readRegInt(L"vUseMacro", 1) != 0);
    cfg.setBool("macro", "useInEnglishMode", readRegInt(L"vUseMacroInEnglishMode", 0) != 0);
    cfg.setBool("macro", "autoCaps", readRegInt(L"vAutoCapsMacro", 0) != 0);
    cfg.setBool("macro", "quickTelex", readRegInt(L"vQuickTelex", 0) != 0);
    cfg.setBool("macro", "quickStartConsonant", readRegInt(L"vQuickStartConsonant", 0) != 0);
    cfg.setBool("macro", "quickEndConsonant", readRegInt(L"vQuickEndConsonant", 0) != 0);
    
    // === System Tab (Hệ thống) ===
    cfg.setBool("system", "runWithWindows", readRegInt(L"vRunWithWindows", 0) != 0);
    cfg.setBool("system", "runAsAdmin", readRegInt(L"vRunAsAdmin", 0) != 0);
    cfg.setBool("system", "checkNewVersion", readRegInt(L"vCheckNewVersion", 0) != 0);
    cfg.setBool("system", "createDesktopShortcut", readRegInt(L"vCreateDesktopShortcut", 0) != 0);
    cfg.setBool("system", "supportMetroApp", readRegInt(L"vSupportMetroApp", 0) != 0);
    cfg.setBool("system", "fixChromiumBrowser", readRegInt(L"vFixChromiumBrowser", 0) != 0);
    cfg.setBool("system", "useClipboard", readRegInt(L"vSendKeyStepByStep", 1) == 0);  // Inverted!
    cfg.setInt("system", "iconStyle", readRegInt(L"vUseGrayIcon", 0));  // 0=Color, 1=Dark, 2=Light, 3=Custom
    cfg.setInt("system", "customColorV", readRegInt(L"vTrayIconColorV", 0));
    cfg.setInt("system", "customColorE", readRegInt(L"vTrayIconColorE", 0));
    cfg.setBool("system", "showOnStartup", readRegInt(L"vShowOnStartUp", 0) != 0);
    cfg.setInt("system", "showAdvancedSettings", readRegInt(L"vShowAdvancedSettings", 0));
    cfg.setInt("system", "backgroundOpacity", readRegInt(L"vBackgroundOpacity", 80));
    
    // === Excluded Apps ===
    cfg.setBool("excludedApps", "enabled", readRegInt(L"vExcludeApps", 0) != 0);
    // Migrate englishOnlyApps binary data
    {
        std::vector<BYTE> data = readRegBinary(L"englishOnlyApps");
        if (!data.empty() && data.size() >= 2) {
            std::vector<std::string> apps;
            UINT16 count = 0;
            memcpy(&count, data.data(), 2);
            UINT32 cursor = 2;
            for (int i = 0; i < count; i++) {
                // Check if we can read the length byte
                if (cursor >= data.size()) break;
                UINT8 len = data[cursor++];
                // Check if we can read 'len' bytes of app name
                if (cursor + len > data.size()) break;
                std::string app((char*)data.data() + cursor, len);
                apps.push_back(app);
                cursor += len;
            }
            if (!apps.empty()) {
                cfg.setStringArray("excludedApps", "list", apps);
            }
        }
    }
    
    // === Special Apps ===
    auto parseCSV = [](const std::wstring& csv) -> std::vector<std::string> {
        std::vector<std::string> result;
        if (csv.empty()) return result;
        
        std::wstringstream ss(csv);
        std::wstring token;
        while (std::getline(ss, token, L',')) {
            if (!token.empty()) {
                int size = WideCharToMultiByte(CP_UTF8, 0, token.c_str(), -1, nullptr, 0, nullptr, nullptr);
                std::string utf8(size - 1, 0);
                WideCharToMultiByte(CP_UTF8, 0, token.c_str(), -1, &utf8[0], size, nullptr, nullptr);
                result.push_back(utf8);
            }
        }
        return result;
    };
    
    std::wstring qtApps = readRegStringW(L"vQtElectronApps", L"");
    if (!qtApps.empty()) {
        cfg.setStringArray("specialApps", "qtElectron", parseCSV(qtApps));
    }
    
    std::wstring imeApps = readRegStringW(L"vSkipImeCheckApps", L"");
    if (!imeApps.empty()) {
        cfg.setStringArray("specialApps", "skipIme", parseCSV(imeApps));
    }
    
    std::wstring deletedApps = readRegStringW(L"vDeletedDefaultApps", L"");
    if (!deletedApps.empty()) {
        cfg.setStringArray("specialApps", "deletedDefaults", parseCSV(deletedApps));
    }
    
    // === Smart Switch Data (binary) ===
    {
        std::vector<BYTE> data = readRegBinary(L"smartSwitchKey");
        if (!data.empty() && data.size() >= 2) {
            std::map<std::string, int> switchData;
            UINT16 count = 0;
            memcpy(&count, data.data(), 2);
            UINT32 cursor = 2;
            for (int i = 0; i < count; i++) {
                // Check if we can read the length byte
                if (cursor >= data.size()) break;
                UINT8 len = data[cursor++];
                // Check if we can read 'len' bytes of app name + 1 byte of value
                if (cursor + len + 1 > data.size()) break;
                std::string app((char*)data.data() + cursor, len);
                cursor += len;
                INT8 value = data[cursor++];
                switchData[app] = value;
            }
            
            // Filter out excluded apps from smartSwitchData
            auto excludedApps = cfg.getStringArray("excludedApps", "list");
            for (const auto& excluded : excludedApps) {
                switchData.erase(excluded);
            }
            
            if (!switchData.empty()) {
                cfg.setSmartSwitchData(switchData);
            }
        }
    }
    
    // === Macros (binary) ===
    {
        std::vector<BYTE> data = readRegBinary(L"macroData");
        if (!data.empty() && data.size() >= 2) {
            std::vector<std::pair<std::string, std::string>> macros;
            UINT16 count = 0;
            memcpy(&count, data.data(), 2);
            UINT32 cursor = 2;
            for (int i = 0; i < count; i++) {
                // Check if we can read the text length byte
                if (cursor >= data.size()) break;
                UINT8 textLen = data[cursor++];
                // Check if we can read 'textLen' bytes of text
                if (cursor + textLen > data.size()) break;
                std::string text((char*)data.data() + cursor, textLen);
                cursor += textLen;
                
                // Check if we can read 2 bytes for content length
                if (cursor + 2 > data.size()) break;
                UINT16 contentLen = 0;
                memcpy(&contentLen, data.data() + cursor, 2);
                cursor += 2;
                
                // Check if we can read 'contentLen' bytes of content
                if (cursor + contentLen > data.size()) break;
                std::string content((char*)data.data() + cursor, contentLen);
                cursor += contentLen;
                
                macros.push_back({text, content});
            }
            if (!macros.empty()) {
                cfg.setMacros(macros);
            }
        }
    }
    
    // === Convert Tool ===
    cfg.setInt("convertTool", "hotkey", readRegInt(L"convertToolHotKey", 0));
    cfg.setInt("convertTool", "fromCode", readRegInt(L"convertToolFromCode", 0));
    cfg.setInt("convertTool", "toCode", readRegInt(L"convertToolToCode", 0));
    cfg.setBool("convertTool", "toAllCaps", readRegInt(L"convertToolToAllCaps", 0) != 0);
    cfg.setBool("convertTool", "toAllNonCaps", readRegInt(L"convertToolToAllNonCaps", 0) != 0);
    cfg.setBool("convertTool", "removeMark", readRegInt(L"convertToolRemoveMark", 0) != 0);
    cfg.setBool("convertTool", "toCapsEachWord", readRegInt(L"convertToolToCapsEachWord", 0) != 0);
    cfg.setBool("convertTool", "toCapsFirstLetter", readRegInt(L"convertToolToCapsFirstLetter", 0) != 0);
    cfg.setBool("convertTool", "dontAlertCompleted", readRegInt(L"convertToolDontAlertWhenCompleted", 0) != 0);
    
    // === Debug ===
    cfg.setBool("debug", "enablePerfLog", readRegInt(L"vEnablePerfLog", 0) != 0);
    
    // Save to disk
    cfg.save();
    
    // === Registry Cleanup ===
    // After successful migration to TOML, delete the old OpenKey registry key
    HKEY hParent;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, L"SOFTWARE\\TuyenMai", 0, KEY_ALL_ACCESS, &hParent) == ERROR_SUCCESS) {
        RegDeleteKey(hParent, L"OpenKey");
        RegCloseKey(hParent);
        // Try to delete TuyenMai if empty
        RegDeleteKey(HKEY_CURRENT_USER, L"SOFTWARE\\TuyenMai");
    }
    
    return true;
}

