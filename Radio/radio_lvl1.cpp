/*
 * radio_lvl1.cpp
 *
 *  Created on: Nov 17, 2013
 *      Author: kreyl
 */

#include "radio_lvl1.h"
#include "cc1101.h"
#include "uart.h"

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

static rPkt_t pkt_rx, pkt_tx;
static uint32_t supercycle_cnt = 0;
static RxTable_t tbl1, tbl2, *curr_tbl = &tbl1;
static uint8_t tx_power;

static inline void TryToReceive(uint32_t rx_duration_ms) {
    sysinterval_t total_duration_st = TIME_MS2I(rx_duration_ms);
    sysinterval_t start_time_st = chVTGetSystemTimeX();
    sysinterval_t time_left_st = total_duration_st;
    CC.Recalibrate();
    while(true) {
        DBG2_SET();
        retv rx_rslt = CC.Receive_st(time_left_st, (uint8_t*)&pkt_rx, RPKT_LEN, &pkt_rx.rssi);
        DBG2_CLR();
        if(rx_rslt == retv::Ok) {
//            Printf("t=%d; Rssi=%d\r", PktRx.Type, PktRx.Rssi);
            curr_tbl->AddOrReplaceExistingPkt(pkt_rx);
        }
        // Check if rx more or get out
        systime_t elapsed_st = chVTTimeElapsedSinceX(start_time_st);
        if(elapsed_st >= total_duration_st) break;
        else time_left_st = total_duration_st - elapsed_st;
    }
}

static inline void TryToSleep(uint32_t sleep_duration_ms) {
    if(sleep_duration_ms >= MIN_SLEEP_DURATION_MS) CC.EnterPwrDown();
    else CC.EnterIdle();
    chThdSleepMilliseconds(sleep_duration_ms);
}

static inline void TaskFeelEachOther() {
    for(uint32_t cycle_n=0; cycle_n < CYCLE_CNT; cycle_n++) {   // Iterate cycles
        uint32_t tx_slot = Random::Generate(0, (SLOT_CNT-1)); // Decide when to transmit
        // If TX slot is not zero: receive in zero cycle, sleep in non-zero cycle
        if(tx_slot != 0) {
            uint32_t time_before_tx = tx_slot * SLOT_DURATION_MS;
            if(cycle_n == 0) TryToReceive(time_before_tx);
            else TryToSleep(time_before_tx);
        }
        // ==== TX ====
        pkt_tx.id = cfg.id;
        pkt_tx.type = (uint8_t)cfg.type;
        DBG1_SET();
        CC.Recalibrate();
        CC.Transmit((uint8_t*)&pkt_tx, RPKT_LEN);
        DBG1_CLR();

        // If TX slot is not last: receive in zero cycle, sleep in non-zero cycle
        if(tx_slot != (SLOT_CNT-1)) {
            uint32_t time_after_tx = ((SLOT_CNT-1) - tx_slot) * SLOT_DURATION_MS;
            if(cycle_n == 0) TryToReceive(time_after_tx);
            else TryToSleep(time_after_tx);
        }
    } // for
}

static THD_WORKING_AREA(warLvl1Thread, 256);
__noreturn
static void rLvl1Thread(void *arg) {
    chRegSetThreadName("rLvl1");
    while(true) {
        TaskFeelEachOther();
        // Set new tx pwr if changed
        if(tx_power != cfg.tx_power) {
            tx_power = cfg.tx_power;
            CC.SetTxPower(tx_power);
        }
        // Is it time to check?
        supercycle_cnt++;
        if(supercycle_cnt >= CHECK_RXTABLE_PERIOD_SC) {
            supercycle_cnt = 0;
            if(curr_tbl->cnt != 0) { // Report and switch table if not empty
                chSysLock();
                EvtQMain.SendNowOrExitI(EvtMsg_t(EvtId::CheckRxTable, (void*)curr_tbl));
                curr_tbl = (curr_tbl == &tbl1)? &tbl2 : &tbl1;
                curr_tbl->Clear();
                chSysUnlock();
            }
        }
    } // while true
}

namespace radio {

retv Init() {
#ifdef DBG_PINS
    PinSetupOut(DBG_GPIO1, DBG_PIN1, omPushPull);
    PinSetupOut(DBG_GPIO2, DBG_PIN2, omPushPull);
#endif

    if(CC.Init() == retv::Ok) {
        CC.SetPktSize(RPKT_LEN);
        CC.SetChannel(RCHNL_EACH_OTH);
        CC.SetTxPower(cfg.tx_power);
        CC.SetBitrate(CCBitrate500k);
        // Thread
        chThdCreateStatic(warLvl1Thread, sizeof(warLvl1Thread), HIGHPRIO, (tfunc_t)rLvl1Thread, NULL);
        return retv::Ok;
    }
    else return retv::Fail;
}

} // namespace
