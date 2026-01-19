/*----------------------------------------------------------
NextKey - Vietnamese Input Engine Optimization

RuntimeProfile.cpp - Implementation of HWND-based profile cache
- Framework detection via window class heuristics
- Latency probe and auto-classification (Milestone 2)
- O(1) profile lookup in hot path
-----------------------------------------------------------*/

#include "RuntimeProfile.h"
#include "PerformanceLogger.h"
#include "ConfigManager.h"
#include <algorithm>
#include <cctype>
#include <psapi.h>

// Global cache instances
std::unordered_map<HWND, RuntimeProfile> g_hwndProfileCache;
int g_focusChangeCount = 0;

// Latency thresholds for classification
constexpr uint16_t LATENCY_THRESHOLD_HIGH = 80;   // ms - Qt/Electron lag
constexpr uint16_t LATENCY_THRESHOLD_NORMAL = 30; // ms - Native apps

// Helper to lowercase string
static std::string toLower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return result;
}

// Detect framework type via window class name heuristics
// This is called once per HWND in focus change handler, NOT in hot path
FrameworkType detectFramework(HWND hwnd, const std::string& exeName) {
    TCHAR className[256] = {0};
    GetClassName(hwnd, className, 256);
    
    std::string lowerExe = toLower(exeName);
    
    // Electron detection: Chrome widget but NOT actual browser
    // Electron apps use Chromium's window class but aren't Chrome/Edge/Brave
    if (wcsstr(className, L"Chrome_WidgetWin") != nullptr) {
        // Exclude actual browsers
        if (lowerExe.find("chrome") == std::string::npos &&
            lowerExe.find("msedge") == std::string::npos &&
            lowerExe.find("brave") == std::string::npos) {
            return FrameworkType::Electron;
        }
    }
    
    // Qt detection via window class
    if (wcsstr(className, L"Qt5") != nullptr ||
        wcsstr(className, L"Qt6") != nullptr ||
        wcsstr(className, L"QWidget") != nullptr) {
        return FrameworkType::Qt;
    }
    
    return FrameworkType::Native;
}

// Get hint for app (soft classification, can be overridden by probe)
// NOTE: BrowserLike ONLY comes from hint, not from probe
static ProfileType getHintForApp(const std::string& exeName) {
    std::string lower = toLower(exeName);
    
    // Qt/Electron hints
    if (lower.find("code") != std::string::npos ||      // VS Code
        lower.find("discord") != std::string::npos ||
        lower.find("slack") != std::string::npos) {
        return ProfileType::QtElectronLike;
    }
    
    // Browser hints (autocomplete bug - hint-only, no probe detection)
    if (lower.find("chrome") != std::string::npos ||
        lower.find("msedge") != std::string::npos ||
        lower.find("firefox") != std::string::npos ||
        lower.find("brave") != std::string::npos) {
        return ProfileType::BrowserLike;
    }
    
    // Native/Rich TSF hints (stable apps)
    if (lower.find("powerpnt") != std::string::npos ||
        lower.find("winword") != std::string::npos ||
        lower.find("excel") != std::string::npos ||
        lower.find("notepad") != std::string::npos) {
        return ProfileType::NativeRichTSF;
    }
    
    return ProfileType::Unknown;
}

// Apply flags based on profile type
static void applyProfileTypeFlags(RuntimeProfile* profile) {
    switch (profile->type) {
        case ProfileType::NativeRichTSF:
            // Native apps don't need special handling
            profile->clearFlag(ProfileFlags::SkipEmptyChar);
            profile->clearFlag(ProfileFlags::SkipImeCheck);
            break;
            
        case ProfileType::QtElectronLike:
            // Skip empty char to avoid lazy init lag
            profile->setFlag(ProfileFlags::SkipEmptyChar);
            break;
            
        case ProfileType::BrowserLike:
            // Browsers need empty char for autocomplete fix
            profile->clearFlag(ProfileFlags::SkipEmptyChar);
            break;
            
        case ProfileType::LegacyFallback:
        case ProfileType::Unknown:
        default:
            // Keep existing flags
            break;
    }
}

// Classify profile based on latency probe results
// Called after 3 probes to ensure stability
void classifyProfile(RuntimeProfile* profile) {
    if (profile->isProbeComplete) return;
    
    // Use MIN latency from probes (avoids false positives from background load)
    uint16_t latency = profile->minLatencyMs;
    
    // Classification logic based on spec
    // NOTE: BrowserLike is NOT set by probe - only by hint
    // Probe only determines: NativeRichTSF, QtElectronLike, or LegacyFallback
    
    if (latency <= LATENCY_THRESHOLD_NORMAL) {
        // Low latency, stable - Native/Rich TSF (e.g., PowerPoint, Word)
        profile->type = ProfileType::NativeRichTSF;
    } 
    else if (latency >= LATENCY_THRESHOLD_HIGH) {
        // High latency on first keystroke - Qt/Electron pattern
        profile->type = ProfileType::QtElectronLike;
    }
    else {
        // Middle range - uncertain, use fallback
        // BrowserLike only comes from hint, not probe (no autocomplete detection yet)
        if (profile->type != ProfileType::BrowserLike) {
            profile->type = ProfileType::LegacyFallback;
        }
    }
    
    // Apply flags based on final classification
    applyProfileTypeFlags(profile);
    
    profile->isProbeComplete = true;
    
    if (PerformanceLogger::isEnabled()) {
        char buf[128];
        sprintf_s(buf, "PROFILE_CLASSIFIED[type=%d,minLatency=%dms,probes=%d]", 
                  (int)profile->type, latency, profile->probeCount);
        PerformanceLogger::log(buf, 0);
    }
}

// Get or create profile for HWND
// Called in focus change handler, populates flags based on framework + hint
RuntimeProfile& getOrCreateProfile(HWND hwnd, const std::string& exeName) {
    auto it = g_hwndProfileCache.find(hwnd);
    if (it != g_hwndProfileCache.end()) {
        return it->second;
    }
    
    // Create new profile
    RuntimeProfile profile;
    profile.framework = detectFramework(hwnd, exeName);
    
    // Apply hint (soft classification)
    ProfileType hint = getHintForApp(exeName);
    if (hint != ProfileType::Unknown) {
        profile.type = hint;
        applyProfileTypeFlags(&profile);
    }
    
    // Framework detection can override hint for Qt/Electron
    // Window class is a stronger signal than exe name hint
    if (profile.framework == FrameworkType::Qt || 
        profile.framework == FrameworkType::Electron) {
        profile.type = ProfileType::QtElectronLike;
        profile.setFlag(ProfileFlags::SkipEmptyChar);
        profile.isProbeComplete = true;  // Window class is strong enough signal
    }
    
    // Insert and return reference
    g_hwndProfileCache[hwnd] = profile;
    
    // Apply user overrides AFTER auto-detection (overrides take precedence)
    applyUserOverrides(g_hwndProfileCache[hwnd], exeName);
    
    return g_hwndProfileCache[hwnd];
}

// Apply user-configured per-app overrides from ConfigManager
// Called after auto-detection, user overrides take precedence over heuristics
void applyUserOverrides(RuntimeProfile& profile, const std::string& exeName) {
    auto& config = ConfigManager::instance();
    auto overrides = config.getAppOverrides();
    
    // Debug: log function call
    if (PerformanceLogger::isEnabled()) {
        char buf[128];
        sprintf_s(buf, "CHECKING_OVERRIDES[App=%s,Count=%zu]", exeName.c_str(), overrides.size());
        PerformanceLogger::log(buf, 0);
    }
    
    std::string lowerExe = toLower(exeName);
    
    for (const auto& cfg : overrides) {
        if (toLower(cfg.exeName) == lowerExe) {
            // Apply behavior override
            if (cfg.behaviorType == 1) {  // Skip IME Check
                profile.setFlag(ProfileFlags::SkipImeCheck);
                
                if (PerformanceLogger::isEnabled()) {
                    char buf[128];
                    sprintf_s(buf, "USER_OVERRIDE[App=%s,Behavior=SkipIME]", exeName.c_str());
                    PerformanceLogger::log(buf, 0);
                }
            } 
            else if (cfg.behaviorType == 2) {  // Qt/Electron
                profile.setFlag(ProfileFlags::SkipEmptyChar);
                profile.type = ProfileType::QtElectronLike;
                
                if (PerformanceLogger::isEnabled()) {
                    char buf[128];
                    sprintf_s(buf, "USER_OVERRIDE[App=%s,Behavior=QtElectron]", exeName.c_str());
                    PerformanceLogger::log(buf, 0);
                }
            }
            
            // Apply clipboard override
            if (cfg.clipboardMethod >= 0) {
                profile.injectionMethod = cfg.clipboardMethod;
                
                if (PerformanceLogger::isEnabled()) {
                    char buf[128];
                    sprintf_s(buf, "USER_OVERRIDE[App=%s,Clipboard=%d]", exeName.c_str(), cfg.clipboardMethod);
                    PerformanceLogger::log(buf, 0);
                }
            }
            
            return;  // Found match, stop searching
        }
    }
    
    // No user override - log auto-detect result
    if (PerformanceLogger::isEnabled() && profile.type != ProfileType::Unknown) {
        char buf[128];
        sprintf_s(buf, "AUTO_DETECT[App=%s,Type=%d]", exeName.c_str(), (int)profile.type);
        PerformanceLogger::log(buf, 0);
    }
}

// Cleanup stale HWND entries that are no longer valid windows
// Called periodically (every ~100 focus changes) to prevent memory growth
void cleanupStaleProfiles() {
    for (auto it = g_hwndProfileCache.begin(); it != g_hwndProfileCache.end(); ) {
        if (!IsWindow(it->first)) {
            it = g_hwndProfileCache.erase(it);
        } else {
            ++it;
        }
    }
}
