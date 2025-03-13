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
// Adaptive cycle cnt related
static uint32_t sc_left_before_rare_mode = Radio::kNoReceptionScCnt;
static uint32_t cycle_cnt = Radio::kCycleCntNominal;


static inline uint32_t TryToReceive(uint32_t rx_duration_ms) {
    uint32_t rcvd_cnt = 0;
    sysinterval_t total_duration_st = TIME_MS2I(rx_duration_ms);
    sysinterval_t start_time_st = chVTGetSystemTimeX();
    sysinterval_t time_left_st = total_duration_st;
    CC.Recalibrate();
    while(true) {
        DBG2_SET();
        retv rx_rslt = CC.Receive_st(time_left_st, reinterpret_cast<uint8_t*>(&pkt_rx), kRPktSz, &pkt_rx.rssi);
        DBG2_CLR();
        if(rx_rslt == retv::Ok) {
            rcvd_cnt++;
//            Printf("%u %d; %d\r", pkt_rx.id, pkt_rx.type, pkt_rx.rssi);
            curr_tbl->AddOrReplaceExistingPkt(pkt_rx);
        }
        // Check if rx more or get out
        systime_t elapsed_st = chVTTimeElapsedSinceX(start_time_st);
        if(elapsed_st >= total_duration_st) break;
        else time_left_st = total_duration_st - elapsed_st;
    }
    return rcvd_cnt;
}

namespace Radio {

static void TryToSleep(uint32_t sleep_duration_ms) {
    if(sleep_duration_ms >= kMinSleepDuration_ms) CC.EnterPwrDown();
    else CC.EnterIdle();
    chThdSleepMilliseconds(sleep_duration_ms);
}

static uint32_t ProcessCycle(bool must_rx) {
    uint32_t rcvd_cnt = 0; // Count received packets
    int32_t tx_slot = Random::Generate(0, (kSlotCnt-1)); // Decide when to transmit
    // If TX slot is not zero, receive or sleep
    if(tx_slot != 0) {
        uint32_t time_before_tx = tx_slot * kSlotDuration_ms;
        if(must_rx) rcvd_cnt += TryToReceive(time_before_tx);
        else TryToSleep(time_before_tx);
    }
    // ==== TX ====
    DBG1_SET();
    CC.Recalibrate();
    CC.Transmit(reinterpret_cast<uint8_t*>(&pkt_tx), kRPktSz);
    DBG1_CLR();
    // If TX slot is not last: receive or sleep
    if(tx_slot != (kSlotCnt-1)) {
        uint32_t time_after_tx = ((kSlotCnt-1) - tx_slot) * kSlotDuration_ms;
        if(must_rx) rcvd_cnt += TryToReceive(time_after_tx);
        else TryToSleep(time_after_tx);
    }
    return rcvd_cnt;
}

static void TaskFeelEachOther() {
    // Run zero cycle with rx enabled and check if something was received
    if(ProcessCycle(true) > 0) { // Something rcvd,
        cycle_cnt = Radio::kCycleCntNominal;  // now receive often
        sc_left_before_rare_mode = Radio::kNoReceptionScCnt; // and reset counter-to-rare-mode
    }
    else { // Silence around
        if(sc_left_before_rare_mode > 0) sc_left_before_rare_mode--; // Decrement counter-to-rare-mode
        else cycle_cnt = Radio::kCycleCntRare; // Or receive rarely if zero
    }
    // Run remaining transmit-only cycles
    for(uint32_t cycle_n=1; cycle_n < cycle_cnt; cycle_n++) ProcessCycle(false);
}

static THD_WORKING_AREA(warLvl1Thread, 256);
__noreturn
static void rLvl1Thread(void *arg) {
    chRegSetThreadName("rLvl1");
    while(true) {
        App::PrepareTxPkt(&pkt_tx);
        TaskFeelEachOther();
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
        // CC.SetBitrate(CCBitrate500k);
        // CC.SetBitrate(CCBitrate250k);
        CC.SetBitrate(CCBitrate100k);
        // CC.SetBitrate(CCBitrate38k4);
        // CC.SetBitrate(CCBitrate10k);
        // CC.SetBitrate(CCBitrate2k4);
        // Thread
        chThdCreateStatic(warLvl1Thread, sizeof(warLvl1Thread), HIGHPRIO, (tfunc_t)rLvl1Thread, NULL);
        return retv::Ok;
    }
    else return retv::Fail;
}

} // namespace Radio