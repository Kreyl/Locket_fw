/*
 * main.h
 *
 *  Created on: 6 марта 2018 г.
 *      Author: Kreyl
 */

#pragma once

#define ID_MIN                  1
#define ID_MAX                  20
#define ID_DEFAULT              ID_MIN

extern int32_t ID;

enum AppMode_t : uint32_t {
    appmTxGrp1 = 0,
    appmRxGrp1 = 1,
    appmTxGrp2 = 2,
    appmRxGrp2 = 3,
    appmFeelEachOther = 4,
};

extern AppMode_t AppMode;
