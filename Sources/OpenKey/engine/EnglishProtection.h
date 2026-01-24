/*----------------------------------------------------------
NextKey - The Modern Vietnamese Input Method Engine.
Based on OpenKey architecture.

Copyright (C) 2026 NextKey Project
Author: Mai Tan Phat
License: GPL (Inherited from OpenKey)
-----------------------------------------------------------*/
//
//  EnglishProtection.h
//  OpenKey - English Protection Module
//
//  3-Tier English Protection System:
//  TIER 1: Hard reject impossible patterns (cl, cr, ending x/r/z/f)
//  TIER 2: Soft bias for ambiguous patterns (y + vowel)
//  TIER 3: User override via same-key insistence
//
//  Only active when spell checking is enabled (vCheckSpelling)
//

#ifndef EnglishProtection_h
#define EnglishProtection_h

#include "DataType.h"

// =============================================================================
// Language Bias State
// =============================================================================
enum class LanguageBias { 
    Unknown,      // Initial state, not yet determined
    HardEnglish,  // TIER 1: Definitely not Vietnamese (disable composition)
    SoftEnglish,  // TIER 2: Ambiguous, defer tone until user insists
    Vietnamese    // Confirmed Vietnamese pattern
};

// =============================================================================
// Public API
// =============================================================================

// Get current language bias state
LanguageBias getLanguageBias();

// Reset state on word boundary or backspace
void resetEnglishProtectionState();

// TIER 1: Check for impossible Vietnamese patterns
// checkEndConsonant: false during insertKey(), true at word boundary
bool checkHardEnglishPatterns(bool checkEndConsonant = true);

// LOOKAHEAD: Would word + incoming mark key form English pattern?
bool wouldFormHardEnglishWithKey(Uint16 incomingKey);

// TIER 2: Check for ambiguous "gray zone" patterns (y+vowel)
bool checkSoftEnglishBias();

// TIER 3: Update insistence tracking for same-key detection
// Returns true if user has "insisted" (pressed same tone key twice)
bool updateToneKeyInsistence(Uint16 toneKey);

// Set language bias
void setLanguageBias(LanguageBias bias);

#endif /* EnglishProtection_h */
