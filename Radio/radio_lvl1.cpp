/*
 * radio_lvl1.cpp
 *
 *  Created on: Nov 17, 2013
 *      Author: kreyl
 */

#include "radio_lvl1.h"
#include "cc1101.h"
#include "shell.h"
// #include "Settings.h"
#include "MsgQ.h"
#include "app_types.h"

cc1101_t CC(CC_Setup0);

// #define DBG_PINS

#ifdef DBG_PINS
#define DBG_GPIO1   GPIOB
#define DBG_PIN1    6
#define DBG1_SET()  PinSetHi(DBG_GPIO1, DBG_PIN1)
#define DBG1_CLR()  PinSetLo(DBG_GPIO1, DBG_PIN1)
#define DBG_GPIO2   GPIOB
#define DBG_PIN2    7
#define DBG2_SET()  PinSetHi(DBG_GPIO2, DBG_PIN2)
#define DBG2_CLR()  PinSetLo(DBG_GPIO2, DBG_PIN2)
#else
#define DBG1_SET()
#define DBG1_CLR()
#define DBG2_SET()
#define DBG2_CLR()
#endif

static rPkt pkt_rx, pkt_tx;
static uint8_t itx_power;
static int32_t tx_slot = 0;
static bool must_transmit;


namespace Radio {

static inline void TryToReceive(uint32_t rx_duration_ms) {
    sysinterval_t total_duration_st = TIME_MS2I(rx_duration_ms);
    systime_t start_time_st = chVTGetSystemTimeX();
    sysinterval_t time_left_st = total_duration_st;
    int8_t rssi;
    while(true) {
        DBG2_SET();
        retv rx_rslt = CC.Receive_st(time_left_st, reinterpret_cast<uint8_t*>(&pkt_rx), kRPktSz, &rssi);
        DBG2_CLR();
        if(rx_rslt == retv::Ok) {
            // Printf("%u %d\n", pkt_rx.to, rssi);
            rx_table.AddPkt(pkt_rx, rssi);
        }
        // Check if rx more or get out
        sysinterval_t elapsed_st = chVTTimeElapsedSinceX(start_time_st);
        if(elapsed_st >= total_duration_st) break;
        else time_left_st = total_duration_st - elapsed_st;
    }
}

static void TryToSleep(uint32_t sleep_duration_ms) {
    if(sleep_duration_ms >= kMinSleepDuration_ms) {
        CC.EnterPwrDown();
        chThdSleepMilliseconds(sleep_duration_ms);
        CC.Recalibrate(); // Recalibrate after power down
    }
    else { // No need to recalibrate
        CC.EnterIdle();
        chThdSleepMilliseconds(sleep_duration_ms);
    }
}

static void DoRxOnlyCycle() {
    TryToReceive(kCycleDuration_ms);
}

static void DoTxRxSleepCycle(ftVoidU32 WhenNoTx) {
    // tx_slot is known already
    int32_t curr_slot = 0, slots_to_wait;
    while(curr_slot < kSlotCnt) {
        // Wait before tx
        slots_to_wait = tx_slot - curr_slot;
        if(slots_to_wait != 0) {
            WhenNoTx(slots_to_wait * kSlotDuration_ms);
            curr_slot += slots_to_wait;
        }
        // Transmit
        DBG1_SET();
        CC.Transmit(reinterpret_cast<uint8_t*>(&pkt_tx), kRPktSz);
        DBG1_CLR();
        curr_slot++;
        // Calc the next tx slot
        int32_t next_slot = tx_slot + Random::Generate(kSlotDiffMin, kSlotCnt - 1U);
        if(next_slot >= kSlotCnt) { // No more tx in this cycle
            tx_slot = next_slot - kSlotCnt;
            slots_to_wait = kSlotCnt - curr_slot; // wait end of the cycle
            if(slots_to_wait != 0) {
                WhenNoTx(slots_to_wait * kSlotDuration_ms);
                return;
            }
        }
        else tx_slot = next_slot;
    }
}

// ==== Radio thread ====
static THD_WORKING_AREA(warLvl1Thread, 384);
static void rLvl1Thread(void *arg) {
    while(true) {
        // Get tx pkt if needed
        must_transmit = GetPktToTx(pkt_tx);

        // ==== FeelEachOther Supercycle ====
        CC.Recalibrate(); // At the beginning of the supercycle
        // Zero Cycle
        if(must_transmit) DoTxRxSleepCycle(TryToReceive);
        else              DoRxOnlyCycle();
        // Report Rx table even if empty. Don't forget to tick it when processed.
        evt_q_main.SendNowOrExit(EvtMsg_t(EvtId::CheckRxTable));
        // Other cycles
        if(must_transmit) {
            for(uint32_t cycle_n=1; cycle_n < kCycleCnt; cycle_n++) {
                DoTxRxSleepCycle(TryToSleep);
            }
        }
        else TryToSleep((kCycleCnt-1) * kCycleDuration_ms);

        // ==== Set new tx pwr if changed ====
        if(itx_power != tx_pwr) {
            itx_power = tx_pwr;
            CC.SetTxPower(itx_power);
        }
    } // while true
}


retv Init() {
#ifdef DBG_PINS
    PinSetupOut(DBG_GPIO1, DBG_PIN1, omPushPull);
    PinSetupOut(DBG_GPIO2, DBG_PIN2, omPushPull);
#endif
    tx_slot = Random::Generate(0, kSlotCnt-1);
    itx_power = tx_pwr;
    if(CC.Init() == retv::Ok) {
        CC.SetPktSize(kRPktSz);
        CC.SetChannel(0);
        Printf("CC pwr: %S\r", CC_PwrToString(itx_power));
        CC.SetTxPower(itx_power);
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
