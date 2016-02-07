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

thread_t *PThd;
#define EVTMSK_ACC_NEW_DATA     EVENT_MASK(1)   // Inner use
#define EVTMSK_GYRO_NEW_DATA    EVENT_MASK(2)   // Inner use

// IRQ Pins
const PinIrq_t GPinDrdy(ACC_DRDY_PIN);
const PinIrq_t XMPinDrdy(ACC_INT1_PIN);

#define G_SETUP_SZ  5
const uint8_t GSetupData[G_SETUP_SZ] = {
        0b00111111, // CTRL_REG1_G: DR=00, BW=11, Pwr=1, XYZ=Enable
        0b00000011, // CTRL_REG2_G: HiPassMode = normal(00), HighPass Cut-off=0.9(0011)
        0b00001000, // CTRL_REG3_G: int dis, boot st dis, irq act hi, push-pull; DRDY=1, other 0
        0b00100000, // CTRL_REG4_G: cont. update, data LSB, 2000dps (10), self-test disable, spi doesn not matter
        0b00010010, // CTRL_REG5_G: norm mode(0), FIFI dis, HiPass en, INT ignore, out after all filters
};

#define XM_SETUP_SZ 8
const uint8_t XMSetupData[XM_SETUP_SZ] = {
        0b00000000, // CTRL_REG0_XM: Normal mode, FIFO dis, HiPass ignore
        0b01100111, // CTRL_REG1_XM: Acc data rate = 100Hz, cont. update, XYZ en
        0b00011000, // CTRL_REG2_XM: anti-alias 773Hz, +-8g, no self-test, serial ignore
        0b00000100, // CTRL_REG3_XM: boot dis, tap dis, acc irqs dis, magnetic irq dis, acc drdy en
        0b00000000, // CTRL_REG4_XM: irq2 disabled
        0b00010100, // CTRL_REG5_XM: temp sns dis, m res low, m data rate 100Hz, irqs not latched
        0b00100000, // CTRL_REG6_XM: m full-scale +-4Gauss
        0b00000000, // CTRL_REG7_XM: HiPass normal, filter dis, m loPwr dis, m cont. conv XXX
};

const uint8_t GDataAddr = (0x28 | 0x80);    // Gyro Data addr + autoincrease
const uint8_t MDataAddr = (0x08 | 0x80);    // Magnet Data addr + autoincrease
const uint8_t XDataAddr = (0x28 | 0x80);    // Acc Data addr + autoincrease

// ============================== Implementation ===============================
static THD_WORKING_AREA(waAccThread, 128);
__NORETURN static void AccThread(void *arg) {
    chRegSetThreadName("Acc");
    Acc.ITask();
}

void LSM9DS0_t::Init() {
    // IRQ pins
//    GPinDrdy.Init(ACC_DRDY_GPIO, pudNone, ttRising);
    XMPinDrdy.Init(ACC_INT1_GPIO, pudNone, ttRising);
    i2c.Init();
    //i2c.BusScan();
    // Check device
    uint32_t Retries = 207;
    while(true) {
        uint8_t gID  = GReadReg(0x0F);
        uint8_t xmID = XMReadReg(0x0F);
        if(gID == 0xD4 and xmID == 0x49) break;
        else {
            Uart.Printf("gID=%X; xmID=%X\r", gID, xmID);
            Retries--;
            if(Retries == 0) {
                Uart.Printf("Acc Fail\r");
                return;
            }
            chThdSleepMilliseconds(4);
        }
    }
    // Setup gyro
//    GWriteBuf(0x20, (uint8_t*)GSetupData, G_SETUP_SZ);
//    for(uint8_t i=0; i<G_SETUP_SZ; i++) Uart.Printf("g %X\r", GReadReg(0x20 + i));

    // Setup Acc
    Uart.Printf("%X\r", XMReadReg(0x12));
    XMWriteBuf(0x1F, (uint8_t*)XMSetupData, XM_SETUP_SZ);
//    for(uint8_t i=0; i<XM_SETUP_SZ; i++) Uart.Printf("xm %X\r", XMReadReg(0x1F + i));
    XMWriteReg(0x2E, 0x00); // FIFO disable
    XMWriteReg(0x12, 0x08); // IRQ active-high, magnetic irq Dis

    // Create and start thread
    PThd = chThdCreateStatic(waAccThread, sizeof(waAccThread), NORMALPRIO, (tfunc_t)AccThread, NULL);
//    GPinDrdy.EnableIrq(IRQ_PRIO_MEDIUM);
//    GPinDrdy.GenerateIrq();
    XMPinDrdy.EnableIrq(IRQ_PRIO_MEDIUM);
    XMPinDrdy.GenerateIrq();
}

__NORETURN
void LSM9DS0_t::ITask() {
    while(true) {
        __unused eventmask_t EvtMsk = chEvtWaitAny(ALL_EVENTS);
//        Uart.PrintfI("Evt\r");
//            uint8_t b = GReadReg(0x27);
//            Uart.Printf("st: %X\r", b);
//        if(EvtMsk & EVTMSK_GYRO_NEW_DATA) {
//            GReadData();
////            Uart.Printf("G: %d %d %d\r", Gyro.x, Gyro.y, Gyro.z);
//        }

        if(EvtMsk & EVTMSK_ACC_NEW_DATA) {
            // Check which data is ready
            uint8_t b = XMReadReg(0x27);    // X status reg
            if(BitIsSet(b, 0x08)) {
                XReadData();
//                Uart.Printf("X: %06d %06d %06d", Accel.x, Accel.y, Accel.z);
            }
            b = XMReadReg(0x07);    // M status reg
            if(BitIsSet(b, 0x08)) {
                MReadData();
//                Uart.Printf("   M: %06d %06d %06d\r", Magnet.x, Magnet.y, Magnet.z);
            }
//            else Uart.Printf("\r");
        }
    } // while true
}

void LSM9DS0_t::GReadData() {
    i2c.WriteRead(ADDR_G, (uint8_t*)&GDataAddr, 1, (uint8_t*)&Gyro, ACC_DATA_SZ);
}
void LSM9DS0_t::MReadData() {
    i2c.WriteRead(ADDR_XM, (uint8_t*)&MDataAddr, 1, (uint8_t*)&Magnet, ACC_DATA_SZ);
}
void LSM9DS0_t::XReadData() {
    i2c.WriteRead(ADDR_XM, (uint8_t*)&XDataAddr, 1, (uint8_t*)&Accel, ACC_DATA_SZ);
}

#if 1 // ========================== Inner use ==================================
uint8_t LSM9DS0_t::GReadReg(uint8_t Addr) {
    uint8_t rslt = 0;
    i2c.WriteRead(ADDR_G, &Addr, 1, &rslt, 1);
    return rslt;
}
uint8_t LSM9DS0_t::XMReadReg(uint8_t Addr) {
    uint8_t rslt = 0;
    i2c.WriteRead(ADDR_XM, &Addr, 1, &rslt, 1);
    return rslt;
}

void LSM9DS0_t::XMWriteReg(uint8_t Addr, uint8_t Value) {
    uint8_t FBuf[2];
    FBuf[0] = Addr;
    FBuf[1] = Value;
    i2c.Write(ADDR_XM, FBuf, 2);
}

void LSM9DS0_t::GWriteBuf(uint8_t Addr, uint8_t *Ptr, uint32_t Sz) {
    Addr |= 0x80; // Auto increase address
    i2c.WriteWrite(ADDR_G, &Addr, 1, Ptr, Sz);
}
void LSM9DS0_t::XMWriteBuf(uint8_t Addr, uint8_t *Ptr, uint32_t Sz) {
    Addr |= 0x80; // Auto increase address
    i2c.WriteWrite(ADDR_XM, &Addr, 1, Ptr, Sz);
}
#endif

#if 1 // ============================= Interrupts ==============================
extern "C" {
CH_IRQ_HANDLER(ACC_IRQ_HANDLER) {
    CH_IRQ_PROLOGUE();
    chSysLockFromISR();
    if(GPinDrdy.IsIrqPending()) {
        GPinDrdy.CleanIrqFlag();
//        Uart.PrintfI("GIrq\r");
        chEvtSignalI(PThd, EVTMSK_GYRO_NEW_DATA);
    }
    if(XMPinDrdy.IsIrqPending()) {
//        Uart.PrintfI("XMIrq\r");
        XMPinDrdy.CleanIrqFlag();
        chEvtSignalI(PThd, EVTMSK_ACC_NEW_DATA);
    }
    chSysUnlockFromISR();
    CH_IRQ_EPILOGUE();
}
} // extern c
#endif
