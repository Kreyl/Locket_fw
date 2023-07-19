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
#define ID_MAX                  99
#define ID_DEFAULT              ID_MIN

// === Types ===
#define TYPE_ARI                0
#define TYPE_KAESU              1
#define TYPE_NORTH              2
#define TYPE_SOUTH              3
#define TYPE_VATEY              4

#define TYPE_CNT                5

class Config_t {
public:
    int32_t ID = ID_MIN;
    uint8_t Type = TYPE_ARI; // Dummy
    uint8_t TxPower = 0;
    bool VibroEn = false;
    bool MustTxInEachOther() { return !(Type == TYPE_ARI or Type == TYPE_KAESU); }
    bool IsAriKaesu() { return (Type == TYPE_ARI or Type == TYPE_KAESU); }
};

extern Config_t Cfg;

#endif //CONFIG_H__