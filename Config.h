/*
 * Config.h
 *
 *  Created on: Jul 4, 2020
 *      Author: layst
 */

#ifndef CONFIG_H__
#define CONFIG_H__

#include <vector>
#include "ChunkTypes.h"
#include <string>
#include "shell.h"
#include "Sequences.h"

#define ID_BAD                  (-1)
#define ID_MIN                  0
#define ID_MAX                  50
#define ID_DEFAULT              ID_MIN

enum class DevType { Witch=0, SaintPlace=1, WitchPlace=2 };

class Config_t {
private:
public:
    DevType type = DevType::Witch;
    int32_t id = ID_MIN;
    uint8_t tx_power = 0;
    int32_t novibro_time_left_s = 0;
};

extern Config_t cfg;

#endif //CONFIG_H__
