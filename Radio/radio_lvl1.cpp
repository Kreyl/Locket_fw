/*
 * radio_lvl1.cpp
 *
 *  Created on: Nov 17, 2013
 *      Author: kreyl
 */

#include "radio_lvl1.h"
#include "cc1101.h"
#include "uart.h"

#include "led.h"
#include "Sequences.h"
#include "Config.h"


cc1101_t CC(CC_Setup0);

#define DBG_PINS

#ifdef DBG_PINS
#define DBG_GPIO1   GPIOB
#define DBG_PIN1    10
#define DBG1_SET()  PinSetHi(DBG_GPIO1, DBG_PIN1)
#define DBG1_CLR()  PinSetLo(DBG_GPIO1, DBG_PIN1)
#define DBG_GPIO2   GPIOB
#define DBG_PIN2    9
#define DBG2_SET()  PinSetHi(DBG_GPIO2, DBG_PIN2)
#define DBG2_CLR()  PinSetLo(DBG_GPIO2, DBG_PIN2)
#else
#define DBG1_SET()
#define DBG1_CLR()
#endif

rLevel1_t Radio;

#if 1 // ================================ Task =================================
static THD_WORKING_AREA(warLvl1Thread, 256);
__noreturn
static void rLvl1Thread(void *arg) {
    chRegSetThreadName("rLvl1");
    while(true) {
        if(Cfg.MustTxFar) Radio.TaskTransmitFar();
        else {
            if(Cfg.MustTxInEachOther) Radio.TaskFeelEachOther();
            else Radio.TaskFeelEachOtherSilently(); // To allow be invisible
            Radio.TaskFeelFar();
        }
    } // while true
}

void rLevel1_t::TaskTransmitFar() {
    CC.SetChannel(RCHNL_FAR);
    CC.SetTxPower(TX_PWR_FAR);
    CC.SetBitrate(CCBitrate10k);
//    CC.SetBitrate(CCBitrate2k4);
    DBG1_SET();
    CC.Recalibrate();
    CC.Transmit(&PktTx, RPKT_LEN);
    DBG1_CLR();
}

void rLevel1_t::TaskFeelEachOther() {
    CC.EnterIdle();
    CC.SetChannel(RCHNL_EACH_OTH);
    CC.SetTxPower(Cfg.TxPower);
    CC.SetBitrate(CCBitrate500k);
    DBG1_SET();

    // ==== Cycle 0: RX-TX-RX ====
    CC.DoRxAfterTx();
    CC.Recalibrate();
    sysinterval_t CycleStart_st = chVTGetSystemTimeX();

    // First phase: try to TX
//    Printf("First\r");
    while(true) {
        // Get out of zero cycle if there was no success trying to transmit
        if(chVTTimeElapsedSinceX(CycleStart_st) >= TIME_MS2I(CYCLE_DURATION_MS)) goto EndOfZeroCycle;
        // Try to Tx
        if(CC.RxCcaTx_st(&PktTx, RPKT_LEN, &PktRx.Rssi) == retvOk) {
//            Printf("TX: %u\r", chVTTimeElapsedSinceX(CycleStart_st));
            break;
        }
        else { // Tx failed, Rx for some time not trying to Tx
//            Printf("**********************\r");
            sysinterval_t RxStart_st = chVTGetSystemTimeX();
            sysinterval_t RxDuration_st = TIME_MS2I(Random::Generate(2, 18));
            while(chVTTimeElapsedSinceX(RxStart_st) < RxDuration_st) {
                if(CC.RxIfNotYet_st(RxDuration_st, &PktRx, RPKT_LEN, &PktRx.Rssi) == retvOk) {
                    Printf("1 t=%d; Rssi=%d\r", PktRx.Type, PktRx.Rssi);
                    RxTableW->AddOrReplaceExistingPkt(PktRx);
                }
            } // while
        }
    } // while

    // Second phase: RX until end of cycle
//    Printf("Second\r");
    while(true) {
        sysinterval_t Elapsed_st = chVTTimeElapsedSinceX(CycleStart_st);
        if(Elapsed_st >= TIME_MS2I(CYCLE_DURATION_MS)) break;
        else {
            sysinterval_t RxDuration_st = TIME_MS2I(CYCLE_DURATION_MS) - Elapsed_st;
            if(CC.RxIfNotYet_st(RxDuration_st, &PktRx, RPKT_LEN, &PktRx.Rssi) == retvOk) {
                Printf("2 ID=%u; t=%d; Rssi=%d\r", PktRx.ID, PktRx.Type, PktRx.Rssi);
                RxTableW->AddOrReplaceExistingPkt(PktRx);
            }
        }
    }
    EndOfZeroCycle:
    DBG1_CLR();

#if 1 // ==== Other cycles ====
    CC.EnterIdle();
    CC.DoIdleAfterTx();
    for(uint32_t CycleN=1; CycleN < CYCLE_CNT; CycleN++) {   // Iterate cycles
        CC.Recalibrate(); // After this, CC will be in IDLE state
        CycleStart_st = chVTGetSystemTimeX();
        while(true) {
            // Get out of cycle if there was no success trying to transmit
            if(chVTTimeElapsedSinceX(CycleStart_st) >= TIME_MS2I(CYCLE_DURATION_MS)) break;
            // Try to transmit
            if(CC.RxCcaTx_st(&PktTx, RPKT_LEN, &PktRx.Rssi) == retvOk) {
//                Printf("TX: %u\r", chVTTimeElapsedSinceX(CycleStart_st));
                break; // Will be in IDLE if success
            }
            else { // Channel was occupied, wait some time
//                Printf("########################\r");
                CC.EnterIdle();
                uint32_t Delay = Random::Generate(2, 18);
                chThdSleepMilliseconds(Delay);
            }
        }
        // Sleep remainder of the cycle
        sysinterval_t Elapsed_st = chVTTimeElapsedSinceX(CycleStart_st);
        if(Elapsed_st < TIME_MS2I(CYCLE_DURATION_MS)) {
            CC.EnterIdle();
            CC.EnterPwrDown();
            sysinterval_t SleepDuration_st = TIME_MS2I(CYCLE_DURATION_MS) - Elapsed_st;
//            Printf("S %u\r", SleepDuration_st);
            chThdSleep(SleepDuration_st);
        }
    }
#endif
}

void rLevel1_t::TaskFeelEachOtherSilently() {
    CC.EnterIdle();
    CC.SetChannel(RCHNL_EACH_OTH);
    CC.SetBitrate(CCBitrate500k);
    CC.Recalibrate();
    sysinterval_t CycleStart_st = chVTGetSystemTimeX();
    // First phase: RX only
    while(true) {
        sysinterval_t Elapsed_st = chVTTimeElapsedSinceX(CycleStart_st);
        if(Elapsed_st >= TIME_MS2I(CYCLE_DURATION_MS)) break;
        else {
            sysinterval_t RxDuration_st = TIME_MS2I(CYCLE_DURATION_MS) - Elapsed_st;
            if(CC.RxIfNotYet_st(RxDuration_st, &PktRx, RPKT_LEN, &PktRx.Rssi) == retvOk) {
                Printf("3 ID=%u; t=%d; Rssi=%d\r", PktRx.ID, PktRx.Type, PktRx.Rssi);
                RxTableW->AddOrReplaceExistingPkt(PktRx);
            }
        }
    }
    // Second phase: sleep
    CC.EnterIdle();
    CC.EnterPwrDown();
    chThdSleepMilliseconds(CYCLE_DURATION_MS * (CYCLE_CNT - 1));
}


void rLevel1_t::TaskFeelFar() {
    CC.SetChannel(RCHNL_FAR);
    CC.SetBitrate(CCBitrate10k);
    CC.Recalibrate();
    uint8_t RxRslt = CC.Receive(45, &PktRx, RPKT_LEN, &PktRx.Rssi);
    if(RxRslt == retvOk) {
        Printf("Far t=%d; Rssi=%d\r", PktRx.Type, PktRx.Rssi);
        RxTableW->AddOrReplaceExistingPkt(PktRx);
    }
}
#endif // task

#if 1 // ============================
uint8_t rLevel1_t::Init() {
#ifdef DBG_PINS
    PinSetupOut(DBG_GPIO1, DBG_PIN1, omPushPull);
    PinSetupOut(DBG_GPIO2, DBG_PIN2, omPushPull);
#endif

    if(CC.Init() == retvOk) {
        CC.SetPktSize(RPKT_LEN);
//        CC.EnterPwrDown();
        // Thread
        chThdCreateStatic(warLvl1Thread, sizeof(warLvl1Thread), HIGHPRIO, (tfunc_t)rLvl1Thread, NULL);
        return retvOk;
    }
    else return retvFail;
}
#endif
