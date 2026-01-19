#pragma once
/*----------------------------------------------------------
NextKey - Vietnamese Input Engine Optimization

RuntimeProfile.h - HWND-based typing behavior cache
- Separate from SmartSwitch (language preference by EXE)
- This caches typing behavior per HWND for O(1) hot path lookup
-----------------------------------------------------------*/

#include <unordered_map>
#include <windows.h>
#include <string>

// Framework type detected via heuristics (NOT hardcoded exe list)
// Detected once per HWND in focus change handler, NOT in hot path
enum class FrameworkType : uint8_t {
    Native = 0,     // Win32 native apps (Notepad, Word)
    Qt = 1,         // Qt5/Qt6 apps (NotepadNext, Qt Creator)
    Electron = 2,   // Electron apps (VSCode, Discord, Slack)
};

// Profile type - auto-classified based on runtime behavior
// Hint may set initial type, but probe can override
enum class ProfileType : uint8_t {
    Unknown = 0,        // Not yet classified
    NativeRichTSF = 1,  // PowerPoint, Word, Notepad - stable TSF, low latency
    QtElectronLike = 2, // VSCode, Discord - lazy init, high first-keystroke latency
    BrowserLike = 3,    // Chrome, Edge - hint-only (autocomplete bug)
    LegacyFallback = 4, // Unreliable apps, middle latency range
};

// Profile flags - bitwise for branchless hot path checks
enum class ProfileFlags : uint8_t {
    None = 0,
    SkipImeCheck = (1 << 0),     // MS Office apps (false IME detection)
    SkipEmptyChar = (1 << 1),    // Qt/Electron (lazy Input Context init)
    PreferClipboard = (1 << 2),  // Problematic apps needing clipboard
};

// Enable bitwise operators for ProfileFlags
inline ProfileFlags operator|(ProfileFlags a, ProfileFlags b) {
    return static_cast<ProfileFlags>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
inline ProfileFlags operator&(ProfileFlags a, ProfileFlags b) {
    return static_cast<ProfileFlags>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}
inline ProfileFlags& operator|=(ProfileFlags& a, ProfileFlags b) {
    a = a | b;
    return a;
}

// Runtime profile cached per HWND
struct RuntimeProfile {
    FrameworkType framework = FrameworkType::Native;
    ProfileType type = ProfileType::Unknown;  // Auto-classified by probe
    ProfileFlags flags = ProfileFlags::None;
    int8_t injectionMethod = -1;  // -1=default SendInput, 0=ShiftInsert, 1=CtrlV, 2=SendInputKey
    uint16_t delayMs = 0;
    uint8_t failureCount = 0;     // For clipboard feedback loop
    
    // Latency probe data (Milestone 2)
    uint16_t minLatencyMs = UINT16_MAX;  // Track MIN latency (avoid false positives)
    uint8_t probeCount = 0;               // Number of probes done
    bool isProbeComplete = false;         // Classification locked-in after 3 probes
    
    // Branchless flag check for hot path
    inline bool hasFlag(ProfileFlags f) const {
        return (flags & f) != ProfileFlags::None;
    }
    
    inline void setFlag(ProfileFlags f) {
        flags |= f;
    }
    
    inline void clearFlag(ProfileFlags f) {
        flags = static_cast<ProfileFlags>(static_cast<uint8_t>(flags) & ~static_cast<uint8_t>(f));
    }
};

// HWND-based cache - O(1) lookup in hot path
extern std::unordered_map<HWND, RuntimeProfile> g_hwndProfileCache;

// Track focus changes for cleanup trigger
extern int g_focusChangeCount;

// Detect framework type (called once per HWND in focus change, NOT hot path)
FrameworkType detectFramework(HWND hwnd, const std::string& exeName);

// Get or create profile for HWND (called on focus change)
RuntimeProfile& getOrCreateProfile(HWND hwnd, const std::string& exeName);

// Hot path lookup - O(1), inline for maximum speed
inline RuntimeProfile* getProfileForHwnd(HWND hwnd) {
    auto it = g_hwndProfileCache.find(hwnd);
    return (it != g_hwndProfileCache.end()) ? &it->second : nullptr;
}

// Clear stale HWND entries (call every ~100 focus changes)
void cleanupStaleProfiles();

// Classify profile based on latency probe (call after 3 probes)
void classifyProfile(RuntimeProfile* profile);

// Apply user-configured per-app overrides from ConfigManager
// Called after auto-detection, user overrides take precedence over heuristics
void applyUserOverrides(RuntimeProfile& profile, const std::string& exeName);

// Trigger cleanup if needed (call in focus change handler)
inline void triggerCleanupIfNeeded() {
    if (++g_focusChangeCount % 100 == 0) {
        cleanupStaleProfiles();
    }
}
