/*
 * Config.h
 *
 *  Created on: Jul 4, 2020
 *      Author: layst
 */

#ifndef CONFIG_H__
#define CONFIG_H__

#define ID_BAD                  (-1)
#define ID_MIN                  0
#define ID_MAX                  49
#define ID_DEFAULT              ID_MIN

// === Types ===
#define TYPE_OBSERVER           0
#define TYPE_TRANSMITTER        1

#define TYPE_CNT                2

class Config_t {
public:
    int32_t ID = ID_MIN;
    uint8_t Type = TYPE_OBSERVER;
    bool MustTxInEachOther() { return Type != TYPE_OBSERVER; }
    uint8_t TxPower = 0;
};

extern Config_t Cfg;

#endif //CONFIG_H__