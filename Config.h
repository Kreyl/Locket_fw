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

enum class DevType { Witch=0, SaintPlace=1, WitchPlace=2 };

class Config {
public:
    static const int32_t kIdMin=0, kIdMax=50, kIdBad=-1, kIdDefault = kIdMin;
    DevType type = DevType::Witch;
    int32_t id = kIdMin;
    uint8_t tx_power = 0;
    // Brightness
    static const uint8_t kBrtCnt = 4;
    static constexpr uint8_t kBrtTable[kBrtCnt] = { 4, 37, 115, 255 };
    uint8_t brt_indx = kBrtCnt - 1;
    // Vibro
    static const int32_t kNoVibroTime_s = 15 * 60; // 15 minutes
    int32_t novibro_time_left_s = 0; // Vibro enabled
    bool VibroEnabled() { return novibro_time_left_s == 0; }
    void DisableVibro() { novibro_time_left_s = kNoVibroTime_s; }
    void EnableVibro()  { novibro_time_left_s = 0; }
};

extern Config cfg;

#endif //CONFIG_H__
