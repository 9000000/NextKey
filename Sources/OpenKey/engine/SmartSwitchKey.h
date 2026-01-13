//
//  SmartSwitchKey.h
//  OpenKey
//
//  Created by Tuyen on 8/13/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//

#ifndef SmartSwitchKey_h
#define SmartSwitchKey_h

#include "DataType.h"
#include <string>
#include <map>

using namespace std;

void initSmartSwitchKey(const Byte* pData, const int& size);

/**
 * convert all data to save on disk
 */
void getSmartSwitchKeySaveData(vector<Byte>& outData);

/**
 * find and get language input method, if don't has set @currentInputMethod value for this app
 * return:
 * -1: don't have this bundleId
 * 0: English
 * 1: Vietnamese
 */
int getAppInputMethodStatus(const string& bundleId, const int& currentInputMethod);

/**
 * Set default language for this @bundleId
 */
void setAppInputMethodStatus(const string& bundleId, const int& language);

/**
 * Initialize English-only apps list from saved data
 */
void initEnglishOnlyApps(const Byte* pData, const int& size);

/**
 * Convert all English-only apps data to save on disk
 */
void getEnglishOnlyAppsSaveData(vector<Byte>& outData);

/**
 * Check if an app is in English-only list
 */
bool isEnglishOnlyApp(const string& bundleId);

/**
 * Add an app to English-only list
 */
void addEnglishOnlyApp(const string& bundleId);

/**
 * Remove an app from English-only list
 */
void removeEnglishOnlyApp(const string& bundleId);

/**
 * Get all apps in English-only list
 */
void getAllEnglishOnlyApps(vector<string>& apps);

// === TOML-friendly helpers (Phase 3b) ===

/**
 * Initialize English-only apps from string list (TOML format)
 */
void initEnglishOnlyAppsFromList(const vector<string>& apps);

/**
 * Initialize smart switch data from map (TOML format)
 */
void initSmartSwitchKeyFromMap(const map<string, int>& data);

/**
 * Get smart switch data as map (for TOML save)
 */
map<string, int> getSmartSwitchKeyAsMap();

#endif /* SmartSwitchKey_h */
