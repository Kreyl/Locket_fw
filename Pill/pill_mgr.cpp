/*
 * pill.cpp
 *
 *  Created on: Apr 17, 2013
 *      Author: g.kruglov
 */

#include "pill_mgr.h"
#include "board.h"

PillMgr_t PillMgr { &i2c1, PILL_PWR_PIN };

void PillMgr_t::Init() {
    PillPwr.Init();   // Power
}

void PillMgr_t::Standby() {
    i2c->Standby();
    PillPwr.Lo();
    __NOP(); __NOP(); __NOP(); __NOP(); // Allow power to fade
    PillPwr.Deinit();
}

void PillMgr_t::Resume() {
    PillPwr.Init();
    PillPwr.Hi();
    __NOP(); __NOP(); __NOP(); __NOP(); // Allow power to rise
    i2c->Resume();
}

void PillMgr_t::Check() {
    Resume();
    if(IsConnectedNow) {    // Check if disconnected
        uint8_t Rslt = i2c->Write(PILL_I2C_ADDR, NULL, 0);
        if(Rslt == OK) State = pillNoChange;
        else {
            IsConnectedNow = false;
            State = pillJustDisconnected;
        }
    }
    else {  // Was not connected
        uint8_t Rslt = Read(PILL_DATA_ADDR, &Pill, PILL_SZ);
        if(Rslt == OK) {
            IsConnectedNow = true;
            State = pillJustConnected;
        }
        else State = pillNoChange;
    }
    Standby();
}

uint8_t PillMgr_t::WritePill() {
    return Write(PILL_DATA_ADDR, &Pill, PILL_SZ);
}

uint8_t PillMgr_t::Read(uint8_t MemAddr, void *Ptr, uint32_t Length) {
    Resume();
    uint8_t Rslt = i2c->WriteRead(PILL_I2C_ADDR, &MemAddr, 1, (uint8_t*)Ptr, Length);
    Standby();
    return Rslt;
}

uint8_t PillMgr_t::Write(uint8_t MemAddr, void *Ptr, uint32_t Length) {
    uint8_t *p8 = (uint8_t*)Ptr;
    Resume();
    // Write page by page
    while(Length) {
        uint8_t ToWriteCnt = (Length > PILL_PAGE_SZ)? PILL_PAGE_SZ : Length;
        // Try to write
        uint32_t Retries = 0;
        while(true) {
//            Uart.Printf("Wr: try %u\r", Retries);
            if(i2c->WriteWrite(PILL_I2C_ADDR, &MemAddr, 1, p8, ToWriteCnt) == OK) {
                Length -= ToWriteCnt;
                p8 += ToWriteCnt;
                MemAddr += ToWriteCnt;
                break;  // get out of trying
            }
            else {
                Retries++;
                if(Retries > 4) {
                    Uart.Printf("Timeout\r");
                    Standby();
                    return TIMEOUT;
                }
                chThdSleepMilliseconds(4);   // Allow memory to complete writing
            }
        } // while trying
    }
    // Wait completion
    uint32_t Retries = 0;
    do {
//        Uart.Printf("Wait: try %u\r", Retries);
        chThdSleepMilliseconds(1);
        Retries++;
        if(Retries > 4) {
            Uart.Printf("Timeout\r");
            Standby();
            return TIMEOUT;
        }
    } while(i2c->Write(PILL_I2C_ADDR, NULL, 0) != OK);
    Standby();
    return OK;
}
