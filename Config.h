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
#define ID_MAX                  243
#define ID_DEFAULT              ID_MIN

#define TYPE_ARI                0
#define TYPE_KAESU              1
#define TYPE_NORTH              2
#define TYPE_NORTH_STRONG       3
#define TYPE_SOUTH              4
#define TYPE_SOUTH_STRONG       5
#define TYPE_NORTH_CURSED       6
#define TYPE_SOUTH_CURSED       7

#define TYPE_CNT                8

#define RCMD_NONE               0
#define RCMD_NORTH_ATTACK       1
#define RCMD_NORTH_RETREAT      2
#define RCMD_SOUTH_ATTACK       3
#define RCMD_SOUTH_RETREAT      4
#define RCMD_NORTH_ARI_KAESU    5
#define RCMD_SOUTH_ARI_KAESU    6
#define RCMD_BESILENT           7



class Config_t {
private:
public:
    int32_t ID = ID_MIN;
    uint8_t Type = 0;
    void SetSelfType(uint8_t AType);

    bool IsNorth() { return (Type == TYPE_NORTH or Type == TYPE_NORTH_STRONG or Type == TYPE_NORTH_CURSED); }
    bool IsSouth() { return (Type == TYPE_SOUTH or Type == TYPE_SOUTH_STRONG or Type == TYPE_SOUTH_CURSED); }
    bool IsStrong() { return (Type == TYPE_NORTH_STRONG or Type == TYPE_SOUTH_STRONG); }
    bool IsAriKaesu() { return (Type == TYPE_ARI or Type == TYPE_KAESU); }

    bool MustTxInEachOther = false;
    uint8_t TxPower = 0;
};

extern Config_t Cfg;
