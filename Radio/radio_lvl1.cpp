/*
 * radio_lvl1.cpp
 *
 *  Created on: Nov 17, 2013
 *      Author: kreyl
 */

#include "radio_lvl1.h"
#include "cc1101.h"
#include "uart.h"
#include "main.h"

#include "led.h"
#include "Sequences.h"

extern uint32_t SelfType;

cc1101_t CC(CC_Setup0);


//#define DBG_PINS

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
        // Process queue
        RMsg_t msg = Radio.RMsgQ.Fetch(TIME_IMMEDIATE);
        if(msg.Cmd == R_MSG_SET_PWR) CC.SetTxPower(msg.Value);
//        if(msg.Cmd == R_MSG_SET_CHNL) CC.SetChannel(msg.Value);
        // Process task
        Radio.TaskFeelEachOtherMany();
    } // while true
}

void rLevel1_t::TaskFeelEachOtherMany() {
    for(uint32_t CycleN=0; CycleN < CYCLE_CNT; CycleN++) {   // Iterate cycles
        uint32_t TxSlot = Random::Generate(0, (SLOT_CNT-1)); // Decide when to transmit
        // If TX slot is not zero: receive in zero cycle, sleep in non-zero cycle
        if(TxSlot != 0) {
            uint32_t TimeBefore = TxSlot * SLOT_DURATION_MS;
            if(CycleN == 0 and RxTableW->Cnt < RXTABLE_SZ) TryToReceive(TimeBefore);
            else TryToSleep(TimeBefore);
        }
        // ==== TX ====
        if(MustTx) {
            PktTx.ID = ID;
            PktTx.Type = SelfType;
            DBG1_SET();
            CC.Recalibrate();
            CC.Transmit(&PktTx, RPKT_LEN);
            DBG1_CLR();
        }
        else chThdSleepMilliseconds(SLOT_DURATION_MS);

        // If TX slot is not last: receive in zero cycle, sleep in non-zero cycle
        if(TxSlot != (SLOT_CNT-1)) {
            uint32_t TimeAfter = ((SLOT_CNT-1) - TxSlot) * SLOT_DURATION_MS;
            if(CycleN == 0 and RxTableW->Cnt < RXTABLE_SZ) TryToReceive(TimeAfter);
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
        uint8_t RxRslt = CC.Receive_st(RxDur_st, &PktRx, RPKT_LEN, &PktRx.Rssi);
        if(RxRslt == retvOk) {
            Printf("Rssi=%d\r", PktRx.Rssi);
            RxTableW->AddOrReplaceExistingPkt(PktRx);
        }
        // Check if repeat or get out
        systime_t Elapsed_st = chVTTimeElapsedSinceX(TimeStart_st);
        if(Elapsed_st >= TotalDuration_st) break;
        else RxDur_st = TotalDuration_st - Elapsed_st;
    }
}
#endif // task

void rLevel1_t::TryToSleep(uint32_t SleepDuration) {
    if(SleepDuration >= MIN_SLEEP_DURATION_MS) CC.EnterPwrDown();
    chThdSleepMilliseconds(SleepDuration);
}

#if 1 // ============================
uint8_t rLevel1_t::Init() {
#ifdef DBG_PINS
    PinSetupOut(DBG_GPIO1, DBG_PIN1, omPushPull);
    PinSetupOut(DBG_GPIO2, DBG_PIN2, omPushPull);
#endif

    RMsgQ.Init();
    if(CC.Init() == retvOk) {
        CC.SetTxPower(CC_Pwr0dBm);
        CC.SetPktSize(RPKT_LEN);
        CC.SetChannel(RCHNL_EACH_OTH);
//        CC.EnterPwrDown();
        // Thread
        chThdCreateStatic(warLvl1Thread, sizeof(warLvl1Thread), HIGHPRIO, (tfunc_t)rLvl1Thread, NULL);
        return retvOk;
    }
    else return retvFail;
}
#endif
