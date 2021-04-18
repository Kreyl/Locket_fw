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

#define ID_BAD                  0xFF
#define ID_MIN                  0
#define ID_MAX                  49 // Defines max count of lockets in the system
#define ID_DEFAULT              ID_MIN

class Config_t {
private:
public:
    uint8_t ID = ID_DEFAULT;
    uint8_t Type = 0;
    uint8_t TxPower = CC_Pwr0dBm;
};

extern Config_t Cfg;
