/*
 * LSM9DS0.h
 *
 *  Created on: 6 февр. 2016 г.
 *      Author: Kreyl
 */

#pragma once

#include "kl_lib.h"

#define ADDR_G      0x6A
#define ADDR_XM     0x1E

struct AccData_t {
    int16_t x, y, z;
} __packed;
#define ACC_DATA_SZ    sizeof(AccData_t)

class LSM9DS0_t {
private:
    uint8_t GReadReg(uint8_t Addr);
    uint8_t XMReadReg(uint8_t Addr);
    void XMWriteReg(uint8_t Addr, uint8_t Value);
    void GWriteBuf(uint8_t Addr, uint8_t *Ptr, uint32_t Sz);
    void XMWriteBuf(uint8_t Addr, uint8_t *Ptr, uint32_t Sz);
public:
    AccData_t Gyro, Magnet, Accel;
    void Init();
    void GReadData();
    void MReadData();
    void XReadData();
    void ITask();
};

extern LSM9DS0_t Acc;
