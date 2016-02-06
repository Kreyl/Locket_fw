/*
 * LSM9DS0.h
 *
 *  Created on: 6 февр. 2016 г.
 *      Author: Kreyl
 */

#pragma once

#include "kl_lib.h"

#define ADDR_GYRO   0x6A
#define ADDR_ACC    0x1E

class LSM9DS0_t {
private:

    uint8_t GReadReg(uint8_t Addr);
    uint8_t XMReadReg(uint8_t Addr);
public:
    void Init();

};

extern LSM9DS0_t Acc;
