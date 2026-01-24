//
//  ConvertTool.h
//  OpenKey
//
//  Created by Tuyen on 9/4/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//
//  Portions Copyright (C) 2026 NextKey Project
//  Maintainer: Mai Tan Phat

#ifndef ConvertTool_h
#define ConvertTool_h

#include "DataType.h"
#include <string>

extern bool convertToolDontAlertWhenCompleted;
extern bool convertToolToAllCaps;
extern bool convertToolToAllNonCaps;
extern bool convertToolToCapsFirstLetter;
extern bool convertToolToCapsEachWord;
extern bool convertToolRemoveMark;
extern Uint8 convertToolFromCode;
extern Uint8 convertToolToCode;
extern int convertToolHotKey;

std::string convertUtil(const std::string& sourceString);

#endif /* ConvertTool_h */

