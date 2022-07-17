/*
 * Config.h
 *
 *  Created on: Jul 4, 2020
 *      Author: layst
 */

#pragma once

#define ID_BAD                  (-1)
#define ID_MIN                  0
#define ID_MAX                  99
#define ID_DEFAULT              ID_MIN

// === Types ===
#define TYPE_OBSERVER           0
#define TYPE_NOTHING            0 // What to show
#define TYPE_DARKSIDE           1
#define TYPE_LIGHTSIDE          2
#define TYPE_BOTH               3 // Allow to tx just in case

#define TYPE_CNT                4

class Config_t {
public:
    int32_t ID = ID_MIN;
    uint8_t Type = TYPE_OBSERVER;
    uint8_t TypeToShow = TYPE_NOTHING;
    bool MustTxInEachOther() { return Type != TYPE_OBSERVER; }
    uint8_t TxPower = 0;
};

extern Config_t Cfg;
