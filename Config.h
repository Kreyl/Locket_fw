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

class Config {
public:
    static const int32_t kIdMin=0, kIdMax=50, kIdBad=-1, kIdDefault = kIdMin;
    // Indication
    bool do_vibro, do_blink, do_beep;
};

extern Config cfg;

#endif //CONFIG_H__
