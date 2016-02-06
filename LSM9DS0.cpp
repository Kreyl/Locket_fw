/*
 * LSM9DS0.cpp
 *
 *  Created on: 6 февр. 2016 г.
 *      Author: Kreyl
 */

#include "LSM9DS0.h"
#include "uart.h"

LSM9DS0_t Acc;
i2c_t i2c (I2C_ACC, ACC_I2C_GPIO, ACC_I2C_SCL_PIN, ACC_I2C_SDA_PIN, 400000, I2C_ACC_DMA_TX, I2C_ACC_DMA_RX );

void LSM9DS0_t::Init() {
    PinSetupIn(ACC_DRDY_GPIO, ACC_DRDY_PIN, pudNone);
    PinSetupIn(ACC_INT1_GPIO, ACC_INT1_PIN, pudNone);
    i2c.Init();
    //i2c.BusScan();
    // Check device
    uint8_t b = GReadReg(0x0F);
    if(b != 0xD4) {
        Uart.Printf("GyroID Err: %X\r", b);
        return;
    }
    b = XMReadReg(0x0F);
    if(b != 0x49) {
        Uart.Printf("AccID Err: %X\r", b);
        return;
    }
    //
}


#if 1 // ========================== Inner use ==================================
uint8_t LSM9DS0_t::GReadReg(uint8_t Addr) {
    uint8_t rslt = 0;
    i2c.WriteRead(ADDR_GYRO, &Addr, 1, &rslt, 1);
    return rslt;
}
uint8_t LSM9DS0_t::XMReadReg(uint8_t Addr) {
    uint8_t rslt = 0;
    i2c.WriteRead(ADDR_ACC, &Addr, 1, &rslt, 1);
    return rslt;
}
#endif
