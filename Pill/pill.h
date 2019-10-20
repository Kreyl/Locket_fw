/*
 * pill.h
 *
 *  Created on: 22 мая 2014 г.
 *      Author: g.kruglov
 */

#pragma once

#include "color.h"

//__unused
//static const Color_t GetPillColor(PillType_t PillType) {
//    switch(PillType) {
//        case ptCure:      return clDarkYellow; break;
//        case ptPanacea:   return clDarkWhite; break;
//    }
//    return clBlack;
//}

struct Pill_t {
    uint32_t DWord32;
} __attribute__ ((__packed__));
#define PILL_SZ     sizeof(Pill_t)
#define PILL_SZ32   (sizeof(Pill_t) / sizeof(int32_t))
