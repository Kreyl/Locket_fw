/*
 * Config.h
 *
 *  Created on: Jul 4, 2020
 *      Author: layst
 */

#pragma once

#include <vector>
#include "ChunkTypes.h"
#include <string>
#include "shell.h"
#include "Sequences.h"

#define ID_BAD                  (-1)
#define ID_MIN                  0
#define ID_MAX                  99
#define ID_DEFAULT              ID_MIN

#define TYPE_LIGHTSIDE          0
#define TYPE_DARKSIDE           1
#define TYPE_OBSERVER           2

#define TYPE_CNT                3

class Config_t {
public:
    int32_t ID = ID_MIN;
    uint8_t Type = 0;
    void SetSelfType(uint8_t AType);
    bool MustTxInEachOther() { return Type != TYPE_OBSERVER; }
    uint8_t TxPower = 0;
};

extern Config_t Cfg;
