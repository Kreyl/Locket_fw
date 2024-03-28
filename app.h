/*
 * app.h
 *
 *  Created on: 28 мар. 2024 г.
 *      Author: layst
 */

#ifndef APP_H_
#define APP_H_

enum class DevType { Witch, SaintPlace, WitchPlace };

struct Dev_t {
    DevType type = DevType::Witch;
    uint8_t tx_power;

};

extern Dev_t Dev;

#endif /* APP_H_ */
