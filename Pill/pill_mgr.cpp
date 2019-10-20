#include "pill_mgr.h"
#include "board.h"
#include "MsgQ.h"
#include "kl_lib.h"

#if PILL_ENABLED
PillMgr_t PillMgr { &I2C_PILL, PILL_PWR_PIN };

void PillMgr_t::Init() {
    PillPwr.Init();   // Power
}

void PillMgr_t::Standby() {
    i2c->Standby();
    PillPwr.SetLo();
    __NOP(); __NOP(); __NOP(); __NOP(); // Allow power to fade
    PillPwr.Deinit();
}

void PillMgr_t::Resume() {
    PillPwr.Init();
    PillPwr.SetHi();
    __NOP(); __NOP(); __NOP(); __NOP(); // Allow power to rise
    i2c->Resume();
}

void PillMgr_t::Check() {
//    Uart.Printf("PillChk\r");
    Resume();
    if(IsConnectedNow) {    // Check if disconnected
        if(i2c->CheckAddress(PILL_I2C_ADDR) == retvOk) State = pillNoChange;
        else {
            IsConnectedNow = false;
            State = pillJustDisconnected;
        }
    }
    else {  // Was not connected
        uint8_t Rslt = Read(PILL_DATA_ADDR, &Pill, PILL_SZ);
        if(Rslt == retvOk) {
            IsConnectedNow = true;
            State = pillJustConnected;
        }
        else State = pillNoChange;
    }
    Standby();
    switch(State) {
        case pillJustConnected:
            EvtQMain.SendNowOrExit(EvtMsg_t(evtIdPillConnected));
            break;
        case pillJustDisconnected:
            EvtQMain.SendNowOrExit(EvtMsg_t(evtIdPillDisconnected));
            break;
        case pillNoChange:
            break;
    }
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
            if(i2c->WriteWrite(PILL_I2C_ADDR, &MemAddr, 1, p8, ToWriteCnt) == retvOk) {
                Length -= ToWriteCnt;
                p8 += ToWriteCnt;
                MemAddr += ToWriteCnt;
                break;  // get out of trying
            }
            else {
                Retries++;
                if(Retries > 4) {
                    Printf("Timeout1\r");
                    Standby();
                    return retvTimeout;
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
        if(Retries > 5) {
//            Uart.Printf("Timeout2\r");
            Standby();
            return retvTimeout;
        }
    } while(i2c->CheckAddress(PILL_I2C_ADDR) != retvOk);
    Standby();
    return retvOk;
}
#endif // PILL_ENABLED
