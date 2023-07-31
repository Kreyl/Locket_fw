/*
 * radio_lvl1.cpp
 *
 *  Created on: Nov 17, 2013
 *      Author: kreyl
 */

#include "radio_lvl1.h"
#include "cc1101.h"
#include "uart.h"
#include "Config.h"

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

rLevel1_t Radio;
static rPkt_t PktRx, PktTx;
static uint8_t ITxPower;
static uint32_t SuperCycleCnt = 0;

void rLevel1_t::TaskFeelEachOther() {
    for(uint32_t CycleN=0; CycleN < CYCLE_CNT; CycleN++) {   // Iterate cycles
        uint32_t TxSlot = Random::Generate(0, (SLOT_CNT-1)); // Decide when to transmit
        // If TX slot is not zero: receive in zero cycle, sleep in non-zero cycle
        if(TxSlot != 0) {
            uint32_t TimeBefore = TxSlot * SLOT_DURATION_MS;
            if(CycleN == 0 and IdBuf.GetCount() < RXTABLE_SZ) TryToReceive(TimeBefore);
            else TryToSleep(TimeBefore);
        }
        // ==== TX ====
        PktTx.ID = Cfg.ID;
        DBG1_SET();
        CC.Recalibrate();
        CC.Transmit((uint8_t*)&PktTx, RPKT_LEN);
        DBG1_CLR();

        // If TX slot is not last: receive in zero cycle, sleep in non-zero cycle
        if(TxSlot != (SLOT_CNT-1)) {
            uint32_t TimeAfter = ((SLOT_CNT-1) - TxSlot) * SLOT_DURATION_MS;
            if(CycleN == 0 and IdBuf.GetCount() < RXTABLE_SZ) TryToReceive(TimeAfter);
            else TryToSleep(TimeAfter);
        }
    } // for
}

void rLevel1_t::TryToReceive(uint32_t RxDuration) {
    sysinterval_t TotalDuration_st = TIME_MS2I(RxDuration);
    sysinterval_t TimeStart_st = chVTGetSystemTimeX();
    sysinterval_t RxDur_st = TotalDuration_st;
    CC.Recalibrate();
    while(true) {
        DBG2_SET();
        uint8_t RxRslt = CC.Receive_st(RxDur_st, (uint8_t*)&PktRx, RPKT_LEN, &PktRx.Rssi);
        DBG2_CLR();
        if(RxRslt == retvOk) {
//            Printf("t=%d; Rssi=%d\r", PktRx.Type, PktRx.Rssi);
            AddPktToRxTableI(&PktRx);
        }
        // Check if repeat or get out
        systime_t Elapsed_st = chVTTimeElapsedSinceX(TimeStart_st);
        if(Elapsed_st >= TotalDuration_st) break;
        else RxDur_st = TotalDuration_st - Elapsed_st;
    }
}

void rLevel1_t::TryToSleep(uint32_t SleepDuration) {
    if(SleepDuration >= MIN_SLEEP_DURATION_MS) CC.EnterPwrDown();
    else CC.EnterIdle();
    chThdSleepMilliseconds(SleepDuration);
}


static THD_WORKING_AREA(warLvl1Thread, 256);
__noreturn
static void rLvl1Thread(void *arg) {
    chRegSetThreadName("rLvl1");
    while(true) {
        if(Cfg.TxPower != ITxPower) {
            ITxPower = Cfg.TxPower;
            CC.SetTxPower(Cfg.TxPower);
        }
        Radio.TaskFeelEachOther();
        // Is it time to check?
        SuperCycleCnt++;
        if(SuperCycleCnt >= CHECK_RXTABLE_PERIOD_SC) {
            SuperCycleCnt = 0;
            EvtQMain.SendNowOrExit(EvtMsg_t(evtIdCheckRxTable));
        }
    } // while true
}

#if 1 // ============================
uint8_t rLevel1_t::Init() {
#ifdef DBG_PINS
    PinSetupOut(DBG_GPIO1, DBG_PIN1, omPushPull);
    PinSetupOut(DBG_GPIO2, DBG_PIN2, omPushPull);
#endif

    if(CC.Init() == retvOk) {
        CC.SetPktSize(RPKT_LEN);
        CC.SetChannel(RCHNL_EACH_OTH);
        CC.SetTxPower(Cfg.TxPower);
        ITxPower = Cfg.TxPower;
        CC.SetBitrate(CCBitrate500k);
        PktTx.Salt = RPKT_SALT;
        // Thread
        chThdCreateStatic(warLvl1Thread, sizeof(warLvl1Thread), HIGHPRIO, (tfunc_t)rLvl1Thread, NULL);
        return retvOk;
    }
    else return retvFail;
}
#endif
