/*
 * pill.h
 *
 *  Created on: 22 мая 2014 г.
 *      Author: g.kruglov
 */

#pragma once

#include "color.h"

enum PillType_t {
    ptVitamin = 101,
    ptCure = 102,
    ptPanacea = 103,
    ptEnergetic = 104,
    ptMaster = 105,
    ptTest = 106
};
#define PILL_TYPE_CNT   6

__unused
static const Color_t GetPillColor(PillType_t PillType) {
//    uint32_t i = (uint32_t)PillType - 101;
    switch(PillType) {
        case ptVitamin:   return clDarkGreen; break;
        case ptCure:      return clDarkYellow; break;
        case ptPanacea:   return clDarkWhite; break;
        case ptEnergetic: return clDarkBlue; break;
        case ptMaster:    return clDarkRed; break;
        case ptTest:      return clBlack; break;
    }
    return clBlack;
}

struct Pill_t {
    union { // Type
        int32_t TypeInt32;      // offset 0
        PillType_t Type;        // offset 0
    };
    // Contains dose value after pill application
    int32_t ChargeCnt;          // offset 4
    bool IsOk() const {
        if(Type < ptVitamin or Type > ptMaster) return false;
        if((Type == ptVitamin or Type == ptCure) and (ChargeCnt < 0 or ChargeCnt > 1)) return false;
        if((Type == ptPanacea or Type == ptEnergetic) and (ChargeCnt < 0 or ChargeCnt > 5)) return false;
        return true;
    }
} __attribute__ ((__packed__));
#define PILL_SZ     sizeof(Pill_t)
#define PILL_SZ32   (sizeof(Pill_t) / sizeof(int32_t))
