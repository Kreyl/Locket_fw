/*
 * Config.h
 *
 *  Created on: Jul 4, 2020
 *      Author: layst
 */

#ifndef CONFIG_H__
#define CONFIG_H__

#define ID_BAD                  (-1)
#define ID_MIN                  1
#define ID_MAX                  36
#define ID_DEFAULT              ID_MIN

#define SUPERCYCLES_2CHECK_RXTBL    4

class Config_t {
public:
    int32_t ID = ID_MIN;
    uint8_t TxPower = 0;
};

extern Config_t Cfg;

#endif //CONFIG_H__
