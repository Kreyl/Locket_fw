/*
 * pill.cpp
 *
 *  Created on: Apr 17, 2013
 *      Author: g.kruglov
 */

#include "pill_mgr.h"
#include "board.h"

PillMgr_t PillMgr { &i2c1, PILL_PWR_PIN };

//static THD_WORKING_AREA(waPillMgrThread, 128);
//__noreturn
//static void PillMgrThread(void *arg) {
//    chRegSetThreadName("PillMgr");
//    while(true) {
//        chThdSleep
//    }
//}

void PillMgr_t::Init() {
    PillPwr.Init();   // Power
}

void PillMgr_t::Standby() {
    i2c->Standby();
    PillPwr.Lo();
    __NOP(); __NOP(); __NOP(); __NOP(); // Allow power to fade
    PillPwr.SetupAnalog();
}

void PillMgr_t::Resume() {
    PillPwr.SetupOutput();
    PillPwr.Hi();
    __NOP(); __NOP(); __NOP(); __NOP(); // Allow power to rise
    i2c->Resume();
}

void PillMgr_t::Check() {
    Resume();
    uint8_t Rslt = i2c->Write(PILL_I2C_ADDR, NULL, 0);
    Standby();
    if(Rslt == OK and !IsConnectedNow) {
        IsConnectedNow = true;
        State = pillJustConnected;
    }
    else if(Rslt != OK and IsConnectedNow) {
        IsConnectedNow = false;
        State = pillJustDisconnected;
    }
    else State = pillNoChange;
}

uint8_t PillMgr_t::Read(uint8_t MemAddr, void *Ptr, uint32_t Length) {
    Resume();
    uint8_t Rslt = i2c->WriteRead(PILL_I2C_ADDR, &MemAddr, 1, (uint8_t*)Ptr, Length);
    Standby();
    return Rslt;
}

uint8_t PillMgr_t::Write(uint8_t MemAddr, void *Ptr, uint32_t Length) {
    uint8_t Rslt = OK;
    uint8_t *p8 = (uint8_t*)Ptr;
    Resume();
    // Write page by page
    while(Length) {
        uint8_t ToWriteCnt = (Length > PILL_PAGE_SZ)? PILL_PAGE_SZ : Length;
        Rslt = i2c->WriteWrite(PILL_I2C_ADDR, &MemAddr, 1, p8, ToWriteCnt);
        if(Rslt == OK) {
            chThdSleepMilliseconds(5);   // Allow memory to complete writing
            Length -= ToWriteCnt;
            p8 += ToWriteCnt;
            MemAddr += ToWriteCnt;
        }
        else break;
    }
    Standby();
    return Rslt;
}
