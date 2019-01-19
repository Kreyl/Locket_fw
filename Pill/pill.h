#pragma once

#include "color.h"

//__unused
//static const Color_t GetPillColor(PillType_t PillType) {
//    switch(PillType) {
//        case 0: return clDarkYellow; break;
//        case 1: return clDarkWhite; break;
//
//    }
//    return clBlack;
//}

struct Pill_t {
    int32_t DWord32;      // offset 0
//    bool IsOk() const { return (Type == ptCure or Type == ptPanacea); }
}; // __attribute__ ((__packed__));

#define PILL_SZ     sizeof(Pill_t)
#define PILL_SZ32   (sizeof(Pill_t) / sizeof(int32_t))
