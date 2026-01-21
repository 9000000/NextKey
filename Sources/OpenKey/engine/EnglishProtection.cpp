//
//  EnglishProtection.cpp
//  OpenKey - English Protection Module
//
//  3-Tier English Protection System:
//  TIER 1: Hard reject impossible patterns (cl, cr, ending x/r/z/f)
//  TIER 2: Soft bias for ambiguous patterns (y + vowel)
//  TIER 3: User override via same-key insistence
//

#include "EnglishProtection.h"
#include <cstring>
#include <cctype>

// External references from Engine.cpp
extern Uint32 TypingWord[];
extern Byte _index;

// =============================================================================
// State Variables
// =============================================================================
static LanguageBias _langBias = LanguageBias::Unknown;
static Uint16 _lastToneKey = 0;
static int _sameToneKeyCount = 0;

// =============================================================================
// TIER 1: Hard Reject Patterns
// =============================================================================
static const char* _hardEnglishStartClusters[] = {
    "cl", "cr", "br", "dr", "fr", "gr", "pr",  // Consonant clusters
    "sm", "sn", "sp", "sw", "st", "sc", "sk",  // S-clusters
    "bl", "fl", "gl", "pl", "sl", "wr",        // Other impossible clusters
    nullptr
};

static const Uint16 _hardEnglishEndConsonants[] = { 
    KEY_X, KEY_R, KEY_Z, KEY_F, 0  // Vietnamese words never end with these
};

// English vowel patterns for lookahead check
static const char* _englishVowelPatterns[] = {
    "ou",   // our, your, four
    "ea",   // ear, bear, fear
    "ei",   // their, weird
    "oo",   // poor, door
    nullptr
};

// =============================================================================
// Helper Functions
// =============================================================================

// CHR macro equivalent
#define CHR(index) (Uint16)TypingWord[index]

// Strip internal mask flags from keycode for comparison
inline Uint16 getKeyCode(Byte index) {
    const Uint16 KEYCODE_MASK = ~(END_CONSONANT_MASK | CONSONANT_ALLOW_MASK);
    return CHR(index) & KEYCODE_MASK;
}

// =============================================================================
// Public API Implementation
// =============================================================================

LanguageBias getLanguageBias() {
    return _langBias;
}

void setLanguageBias(LanguageBias bias) {
    _langBias = bias;
}

void resetEnglishProtectionState() {
    _langBias = LanguageBias::Unknown;
    _lastToneKey = 0;
    _sameToneKeyCount = 0;
}

bool updateToneKeyInsistence(Uint16 toneKey) {
    if (toneKey == _lastToneKey) {
        _sameToneKeyCount++;
    } else {
        _lastToneKey = toneKey;
        _sameToneKeyCount = 1;
    }
    return _sameToneKeyCount >= 2;  // Return true if user insisted
}

// =============================================================================
// TIER 1: Hard English Patterns
// =============================================================================

bool checkHardEnglishPatterns(bool checkEndConsonant) {
    if (_index < 2) return false;
    
    // Check start clusters (PREFIX match at index 0-1 only)
    char startCluster[3] = { 
        (char)tolower(getKeyCode(0)), 
        (char)tolower(getKeyCode(1)), 
        '\0' 
    };
    for (int idx = 0; _hardEnglishStartClusters[idx] != nullptr; idx++) {
        if (strcmp(startCluster, _hardEnglishStartClusters[idx]) == 0) {
            return true;
        }
    }
    
    // Check end consonants
    if (checkEndConsonant) {
        Uint16 lastChar = getKeyCode(_index - 1);
        for (int idx = 0; _hardEnglishEndConsonants[idx] != 0; idx++) {
            if (lastChar == _hardEnglishEndConsonants[idx]) {
                return true;
            }
        }
    }
    
    // Special case: Q without U
    if (getKeyCode(0) == KEY_Q && getKeyCode(1) != KEY_U) {
        return true;
    }
    
    return false;
}

// =============================================================================
// LOOKAHEAD: Would word + incoming key form English?
// =============================================================================

bool wouldFormHardEnglishWithKey(Uint16 incomingKey) {
    if (_index < 2) return false;
    
    // Only check for specific end consonants
    bool isEnglishEndConsonant = false;
    for (int idx = 0; _hardEnglishEndConsonants[idx] != 0; idx++) {
        if (incomingKey == _hardEnglishEndConsonants[idx]) {
            isEnglishEndConsonant = true;
            break;
        }
    }
    if (!isEnglishEndConsonant) return false;
    
    // If word already has VN markers, allow tone
    for (int i = 0; i < _index; i++) {
        if (TypingWord[i] & (TONE_MASK | TONEW_MASK | MARK_MASK)) {
            return false;
        }
    }
    
    // Check if word ends with English vowel pattern
    if (_index >= 2) {
        char lastTwo[3] = {
            (char)tolower(getKeyCode(_index - 2 >= 0 ? _index - 2 : 0)),
            (char)tolower(getKeyCode(_index - 1)),
            '\0'
        };
        
        for (int idx = 0; _englishVowelPatterns[idx] != nullptr; idx++) {
            if (strcmp(lastTwo, _englishVowelPatterns[idx]) == 0) {
                return true;
            }
        }
    }
    
    // Check start clusters
    if (checkHardEnglishPatterns(false)) {
        return true;
    }
    
    return false;
}

// =============================================================================
// TIER 2: Soft English Bias (y + vowel patterns)
// =============================================================================

static bool isValidVietnameseYSequence() {
    if (_index < 2 || getKeyCode(0) != KEY_Y) return false;
    
    // Check for y + ê (e with circumflex)
    if ((TypingWord[1] & TONE_MASK) && getKeyCode(1) == KEY_E) {
        if (_index == 2) return true;
        
        Uint16 thirdChar = getKeyCode(2);
        return (thirdChar == KEY_U || thirdChar == KEY_N || 
                thirdChar == KEY_M || thirdChar == KEY_T);
    }
    
    return false;
}

bool checkSoftEnglishBias() {
    if (_index < 2) return false;
    
    if (getKeyCode(0) == KEY_Y) {
        Uint16 secondChar = getKeyCode(1);
        
        // ya, ye, yo are ambiguous
        bool isAmbiguous = (secondChar == KEY_A || secondChar == KEY_O || secondChar == KEY_E);
        
        if (isAmbiguous && !isValidVietnameseYSequence()) {
            return true;
        }
    }
    
    return false;
}
