/*
 * LSM9DS0.cpp
 *
 *  Created on: 6 февр. 2016 г.
 *      Author: Kreyl
 */

#include "LSM9DS0.h"
#include "uart.h"
#include "main.h"

#include "radio_lvl1.h"

LSM9DS0_t Acc;
i2c_t i2c (I2C_ACC, ACC_I2C_GPIO, ACC_I2C_SCL_PIN, ACC_I2C_SDA_PIN, 400000, I2C_ACC_DMA_TX, I2C_ACC_DMA_RX );

thread_t *PThd;
#define EVT_ACC_NEW_DATA     EVENT_MASK(1)   // Inner use
// IRQ Pin
const PinIrq_t XMPinDrdy(ACC_INT1_PIN);

#if 1 // ============================ Setup data ===============================
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
        0b00000000, // CTRL_REG7_XM: HiPass normal, filter dis, m loPwr dis, m cont. conv
};

const uint8_t GDataAddr = (0x28 | 0x80);    // Gyro Data addr + autoincrease
const uint8_t MDataAddr = (0x08 | 0x80);    // Magnet Data addr + autoincrease
const uint8_t XDataAddr = (0x28 | 0x80);    // Acc Data addr + autoincrease
#endif

// ============================== Implementation ===============================
static THD_WORKING_AREA(waAccThread, 128);
__NORETURN static void AccThread(void *arg) {
    chRegSetThreadName("Acc");
    Acc.ITask();
}

void LSM9DS0_t::Init() {
    IPRead = &IData[1];
    IPWrite = &IData[0];
    gNew = false;
    mNew = false;
    xNew = false;
    // IRQ pins
    XMPinDrdy.Init(ACC_INT1_GPIO, pudNone, ttRising);
    // Power
    PinSetupOut(GPIOB, 12, omPushPull, pudNone);
    i2c.Init();
    //i2c.BusScan();
    uint32_t Retries = 99;
    while(true) {
        PinSet(GPIOB, 12);
        chThdSleepMilliseconds(45);
        // Read IDs
        uint8_t gID  = GReadReg(0x0F);
        uint8_t xmID = XMReadReg(0x0F);
        Uart.Printf("gID=%X; xmID=%X\r", gID, xmID);
        if(gID == 0xD4 and xmID == 0x49) {
            // Setup gyro
            GWriteBuf(0x20, (uint8_t*)GSetupData, G_SETUP_SZ);
            // Setup Acc
            XMWriteBuf(0x1F, (uint8_t*)XMSetupData, XM_SETUP_SZ);
            XMWriteReg(0x2E, 0x00); // FIFO disable
            XMWriteReg(0x12, 0x08); // IRQ active-high, magnetic irq Dis
            // Read Acc value
            while(true) {
                uint8_t b = XMReadReg(0x27);    // X status reg
                if(BitIsSet(b, 0x08)) {
                    XReadData();
                    Uart.Printf("A: %d %d %d\r", IPWrite->Acc.x, IPWrite->Acc.y, IPWrite->Acc.z);
                    int16_t a1 = ABS(IPWrite->Acc.x);
                    int16_t a2 = ABS(IPWrite->Acc.y);
                    int16_t a3 = ABS(IPWrite->Acc.z);
                    if(IS_LIKE(a1, 24538, 4) and IS_LIKE(a2, 24538, 4) and IS_LIKE(a3, 24538, 4)) break; // Bad acc
                    else goto SetupSuccess;
                } // if data
                else chThdSleepMilliseconds(11);
            } // while true
        } // Check IDs
        Retries--;
        if(Retries == 0) {
            Uart.Printf("Acc Fail\r");
            return;
        }
        // Power off
        PinClear(GPIOB, 12);
        chThdSleepMilliseconds(45);
    } // while true

    SetupSuccess:
    // Create and start thread
    PThd = chThdCreateStatic(waAccThread, sizeof(waAccThread), NORMALPRIO, (tfunc_t)AccThread, NULL);
    XMPinDrdy.EnableIrq(IRQ_PRIO_MEDIUM);
    chEvtSignal(PThd, (EVT_ACC_NEW_DATA));  // Start measurement
}

__NORETURN
void LSM9DS0_t::ITask() {
    while(true) {
        uint8_t b;
        eventmask_t EvtMsk = chEvtWaitAny(ALL_EVENTS);
        if(EvtMsk & EVT_ACC_NEW_DATA) {
            // Check which data is ready
            while(true) {
                b = XMReadReg(0x27);    // X status reg
                if(BitIsSet(b, 0x08)) {
                    XReadData();
                    xNew = true;
//                    Uart.Printf("A: %d %d %d\r", IPWrite->Acc.x, IPWrite->Acc.y, IPWrite->Acc.z);
                }
                else break;
            }
            b = XMReadReg(0x07);    // M status reg
            if(BitIsSet(b, 0x08)) {
                MReadData();
                mNew = true;
            }
            b = GReadReg(0x27); // G status reg
            if(BitIsSet(b, 0x08)) {
                GReadData();
                gNew = true;
            }
        } // if new data

        // Check if new data set is ready
        if(gNew and xNew and mNew) {
            gNew = false;
            mNew = false;
            xNew = false;
            // Switch buffers
            if(IPWrite == &IData[0]) {
                IPWrite = &IData[1];
                IPRead  = &IData[0];
            }
            else {
                IPWrite = &IData[0];
                IPRead  = &IData[1];
            }
            // Signal event
//            App.SignalEvt(EVT_NEW_9D);
            if(Radio.PThd != nullptr) chEvtSignal(Radio.PThd, EVT_NEW_9D);
        }
    } // while true
}

void LSM9DS0_t::GReadData() {
    i2c.WriteRead(ADDR_G, (uint8_t*)&GDataAddr, 1, (uint8_t*)&IPWrite->Gyro, ACC_DATA_SZ);
}
void LSM9DS0_t::MReadData() {
    i2c.WriteRead(ADDR_XM, (uint8_t*)&MDataAddr, 1, (uint8_t*)&IPWrite->Magnet, ACC_DATA_SZ);
}
void LSM9DS0_t::XReadData() {
    i2c.WriteRead(ADDR_XM, (uint8_t*)&XDataAddr, 1, (uint8_t*)&IPWrite->Acc, ACC_DATA_SZ);
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

void LSM9DS0_t::GWriteReg(uint8_t Addr, uint8_t Value) {
    uint8_t FBuf[2];
    FBuf[0] = Addr;
    FBuf[1] = Value;
    i2c.Write(ADDR_G, FBuf, 2);
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
    if(XMPinDrdy.IsIrqPending()) {
//        Uart.PrintfI("XMIrq\r");
        XMPinDrdy.CleanIrqFlag();
        chEvtSignalI(PThd, EVT_ACC_NEW_DATA);
    }
    chSysUnlockFromISR();
    CH_IRQ_EPILOGUE();
}
} // extern c
#endif
