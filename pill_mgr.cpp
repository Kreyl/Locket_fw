/*
 * pill.cpp
 *
 *  Created on: Apr 17, 2013
 *  Modified on: Mar 1, 2025
 *      Author: Kreyl
 */

#include "pill_mgr.h"
#include "board.h"
#include "kl_i2c.h"
#include "MsgQ.h"

#if PILL_ENABLED

const PinOutput_t pwr_pin {PILL_PWR_PIN};
static i2c_t &ii2c = I2C_PILL;
bool pill_is_connected = false;

static void Standby() {
    ii2c.Standby();
    pwr_pin.SetLo();
    __NOP(); __NOP(); __NOP(); __NOP(); // Allow power to fade
    pwr_pin.Deinit();
}

static void Resume() {
    pwr_pin.Init();
    pwr_pin.SetHi();
    __NOP(); __NOP(); __NOP(); __NOP(); // Allow power to rise
    ii2c.Resume();
}


namespace PillMgr {

PillData pill_data;

static TmrKL_t tmr_check_pill {TIME_MS2I(kCheckPeriod_ms), EvtId::CheckPill, tktPeriodic};

void Init() {
    pwr_pin.Init();  // Power
    ii2c.Init();
    tmr_check_pill.StartOrRestart();
}

static retv IRead32(int32_t addr, uint32_t *pdata, uint32_t len) {
    uint8_t mem_addr = addr;
    return ii2c.WriteRead(kPillI2CAddr, &mem_addr, 1, reinterpret_cast<uint8_t*>(pdata), len * 4);
}

static retv IWrite32(int32_t addr, uint32_t *pdata, uint32_t len) {
    uint32_t sz = len * 4;
    uint8_t *p8 = reinterpret_cast<uint8_t*>(pdata);
    uint8_t mem_addr = addr;
    while(sz) {
        uint32_t bytes_to_write = (sz > kPillPageSize)? kPillPageSize : sz; // Write page by page
        uint32_t retries = 0;
        while(true) {
            retv r = ii2c.WriteWrite(kPillI2CAddr, &mem_addr, 1, p8, bytes_to_write);
            chThdSleepMilliseconds(5);   // Allow memory to complete writing (or wait for some time)
            if(r.IsOk()) {
                sz -= bytes_to_write;
                p8 += bytes_to_write;
                mem_addr += bytes_to_write;
                break;  // stop trying
            }
            else {
                retries++;
                if(retries > 4) {
                    Printf("Timeout1\r");
                    return retv::Timeout;
                }
            }
        } // while trying
    } // while len
    // Wait completion
    uint32_t retries = 0;
    do {
        chThdSleepMilliseconds(1);
        retries++;
        if(retries > 5) {
            Printf("Timeout2\r");
            Standby();
            return retv::Timeout;
        }
    } while(ii2c.CheckAddress(kPillI2CAddr).NotOk());
    return retv::Ok;
}

void Check() {
    Resume();
    if(pill_is_connected) {  // Check if disconnected
        if(ii2c.CheckAddress(kPillI2CAddr).NotOk()) {
            pill_is_connected = false;
            evt_q_main.SendNowOrExit(EvtId::PillDisconnected);
        }
    }
    else {  // Was not connected, try to read
        if(IRead32(kPillDataAddr, reinterpret_cast<uint32_t*>(&pill_data), kPillDataSz32).IsOk()) {
            pill_is_connected = true;
            evt_q_main.SendNowOrExit(EvtId::PillConnected);
        }
    }
    Standby();
}

retv WritePill(PillData &pill_data) {
    Resume();
    retv r = IWrite32(kPillDataAddr, reinterpret_cast<uint32_t*>(&pill_data), kPillDataSz32);
    Standby();
    return r;
}

retv Read32(uint32_t addr, uint32_t *pdata, uint32_t len) {
    Resume();
    retv r = IRead32(addr, pdata, len);
    Standby();
    return r;
}

retv Write32(uint32_t addr, uint32_t *pdata, uint32_t len) {
    Resume();
    retv r = IWrite32(addr, pdata, len);
    Standby();
    return r;
}

} // namespace PillMgr

#endif // PILL_ENABLED
