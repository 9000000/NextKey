/*----------------------------------------------------------
NextKey - The Modern Vietnamese Input Method Engine.
Based on OpenKey architecture.

Copyright (C) 2026 NextKey Project
Author: Mai Tan Phat
License: GPL (Inherited from OpenKey)

StringUtils.h - Centralized string utility functions
Fixes: CR-007, CR-008 - Eliminates duplicate toLower/wideToUtf8 implementations
-----------------------------------------------------------*/

#pragma once

#include <string>
#include <algorithm>
#include <cctype>

#ifdef _WIN32
#include <windows.h>
#endif

namespace StringUtils {

/**
 * Convert string to lowercase (ASCII only)
 * Thread-safe: Creates new string, doesn't modify input
 */
inline std::string toLower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

/**
 * Convert string to uppercase (ASCII only)
 * Thread-safe: Creates new string, doesn't modify input
 */
inline std::string toUpper(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return result;
}

#ifdef _WIN32
/**
 * Convert wide string (UTF-16) to UTF-8 string
 * Uses Windows WideCharToMultiByte API
 * @param wide Input wide string
 * @return UTF-8 encoded string
 */
inline std::string wideToUtf8(const std::wstring& wide) {
    if (wide.empty()) return "";
    
    int size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size <= 0) return "";
    
    std::string result(size - 1, 0);  // -1 to exclude null terminator
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, &result[0], size, nullptr, nullptr);
    return result;
}

/**
 * Convert UTF-8 string to wide string (UTF-16)
 * Uses Windows MultiByteToWideChar API
 * @param utf8 Input UTF-8 string
 * @return Wide string (UTF-16)
 */
inline std::wstring utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return L"";
    
    int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (size <= 0) return L"";
    
    std::wstring result(size - 1, 0);  // -1 to exclude null terminator
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &result[0], size);
    return result;
}
#endif

/**
 * Check if string starts with prefix
 */
inline bool startsWith(const std::string& str, const std::string& prefix) {
    if (prefix.size() > str.size()) return false;
    return str.compare(0, prefix.size(), prefix) == 0;
}

/**
 * Check if string ends with suffix
 */
inline bool endsWith(const std::string& str, const std::string& suffix) {
    if (suffix.size() > str.size()) return false;
    return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

/**
 * Trim whitespace from both ends of string
 */
inline std::string trim(const std::string& str) {
    const char* whitespace = " \t\n\r\f\v";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

} // namespace StringUtils
