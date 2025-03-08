/*
 * radio_lvl1.cpp
 *
 *  Created on: Nov 17, 2013
 *      Author: kreyl
 */

#include "radio_lvl1.h"
#include "cc1101.h"
#include "uart.h"
#include "App.h"

cc1101_t CC(CC_Setup0);

#define DBG_PINS

#ifdef DBG_PINS
#define DBG_GPIO1   GPIOB
#define DBG_PIN1    10
#define DBG1_SET()  PinSetHi(DBG_GPIO1, DBG_PIN1)
#define DBG1_CLR()  PinSetLo(DBG_GPIO1, DBG_PIN1)
#define DBG_GPIO2   GPIOB
#define DBG_PIN2    11
#define DBG2_SET()  PinSetHi(DBG_GPIO2, DBG_PIN2)
#define DBG2_CLR()  PinSetLo(DBG_GPIO2, DBG_PIN2)
#else
#define DBG1_SET()
#define DBG1_CLR()
#endif

static rPkt pkt_rx, pkt_tx;

static uint32_t supercycle_cnt = 0;
static RxTable tbl1, tbl2, *curr_tbl = &tbl1;
static uint8_t tx_power;

static inline void TryToReceive(uint32_t rx_duration_ms) {
    sysinterval_t total_duration_st = TIME_MS2I(rx_duration_ms);
    sysinterval_t start_time_st = chVTGetSystemTimeX();
    sysinterval_t time_left_st = total_duration_st;
    CC.Recalibrate();
    while(true) {
        DBG2_SET();
        retv rx_rslt = CC.Receive_st(time_left_st, reinterpret_cast<uint8_t*>(&pkt_rx), kRPktSz, &pkt_rx.rssi);
        DBG2_CLR();
        if(rx_rslt == retv::Ok) {
//            Printf("%u %d; %d\r", pkt_rx.id, pkt_rx.type, pkt_rx.rssi);
            curr_tbl->AddOrReplaceExistingPkt(pkt_rx);
        }
        // Check if rx more or get out
        systime_t elapsed_st = chVTTimeElapsedSinceX(start_time_st);
        if(elapsed_st >= total_duration_st) break;
        else time_left_st = total_duration_st - elapsed_st;
    }
}

namespace Radio {

static void TryToSleep(uint32_t sleep_duration_ms) {
    if(sleep_duration_ms >= kMinSleepDuration_ms) CC.EnterPwrDown();
    else CC.EnterIdle();
    chThdSleepMilliseconds(sleep_duration_ms);
}

static void TaskFeelEachOther(bool must_tx, bool must_rx) {
    if(!must_tx and !must_rx) {
        CC.EnterPwrDown();
        chThdSleepMilliseconds(kCycleDuration_ms);
    }
    else if(!must_tx and must_rx) {
        TryToReceive(kCycleDuration_ms); // Zero cycle: receive
        CC.EnterPwrDown();               // Other cycles - just sleep
        chThdSleepMilliseconds(kCycleDuration_ms * (kCycleCnt-1));
    }
    else { // must_tx and maybe must_rx
        for(uint32_t cycle_n=0; cycle_n < kCycleCnt; cycle_n++) {
            int32_t tx_slot = Random::Generate(0, (kSlotCnt-1)); // Decide when to transmit
            // If TX slot is not zero: receive in zero cycle, sleep in non-zero cycle
            if(tx_slot != 0) {
                uint32_t time_before_tx = tx_slot * kSlotDuration_ms;
                if(must_rx and cycle_n == 0) TryToReceive(time_before_tx);
                else TryToSleep(time_before_tx);
            }
            // ==== TX ====
            DBG1_SET();
            CC.Recalibrate();
            CC.Transmit(reinterpret_cast<uint8_t*>(&pkt_tx), kRPktSz);
            DBG1_CLR();
            // If TX slot is not last: receive in zero cycle, sleep in non-zero cycle
            if(tx_slot != (kSlotCnt-1)) {
                uint32_t time_after_tx = ((kSlotCnt-1) - tx_slot) * kSlotDuration_ms;
                if(must_rx and cycle_n == 0) TryToReceive(time_after_tx);
                else TryToSleep(time_after_tx);
            }
        } // for
    } // else
}

static THD_WORKING_AREA(warLvl1Thread, 256);
__noreturn
static void rLvl1Thread(void *arg) {
    chRegSetThreadName("rLvl1");
    while(true) {
        bool must_tx = App::CheckIfTxAndPrepareRPkt(&pkt_tx);
        bool must_rx = App::CheckIfRx();
        TaskFeelEachOther(must_tx, must_rx);
        // Set new tx pwr if changed
        if(tx_power != cfg.tx_power) {
            tx_power = cfg.tx_power;
            CC.SetTxPower(tx_power);
        }
        supercycle_cnt++;
        if(supercycle_cnt >= kCheckRxTablePeriod_sc) {
            supercycle_cnt = 0;
            // Report and switch table even if empty
            chSysLock();
            EvtMsg_t msg{EvtMsg_t(EvtId::CheckRxTable, static_cast<void*>(curr_tbl))};
            curr_tbl = (curr_tbl == &tbl1)? &tbl2 : &tbl1;
            curr_tbl->Clear();
            evt_q_main.SendNowOrExitI(msg);
            chSysUnlock();
        }
    } // while true
}


retv Init() {
#ifdef DBG_PINS
    PinSetupOut(DBG_GPIO1, DBG_PIN1, omPushPull);
    PinSetupOut(DBG_GPIO2, DBG_PIN2, omPushPull);
#endif

    if(CC.Init() == retv::Ok) {
        CC.SetPktSize(kRPktSz);
        CC.SetChannel(0);
        CC.SetTxPower(cfg.tx_power);
        CC.SetBitrate(CCBitrate500k);
        // CC.SetBitrate(CCBitrate250k);
        // CC.SetBitrate(CCBitrate100k);
        // Thread
        chThdCreateStatic(warLvl1Thread, sizeof(warLvl1Thread), HIGHPRIO, (tfunc_t)rLvl1Thread, NULL);
        return retv::Ok;
    }
    else return retv::Fail;
}

} // namespace Radio