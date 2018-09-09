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
        if(msg.Cmd == R_MSG_SET_CHNL) CC.SetChannel(msg.Value);
        // Process task
        switch(AppMode) {
            case appmTxGrp1:
            case appmTxGrp2:
                Radio.TaskTransmitter();
                break;

            case appmRxGrp1:
            case appmRxGrp2:
                Radio.TaskReceiverManyByID();
                break;

            case appmFeelEachOther:
                Radio.TaskFeelEachOtherMany();
                break;
        }
    } // while true
}

void rLevel1_t::TaskTransmitter() {
    PktTx.DWord32 = THE_WORD;
    PktTx.AppMode = (uint32_t)AppMode;
    DBG1_SET();
    CC.Recalibrate();
    CC.Transmit(&PktTx);
    DBG1_CLR();
    chThdSleepMilliseconds(18);
}

void rLevel1_t::TaskReceiverManyByID() {
    for(int N=0; N<4; N++) { // Iterate channels N times
        // Iterate channels
        for(int32_t i = ID_MIN; i <= ID_MAX; i++) {
            if(i == ID) continue;   // Do not listen self
            CC.SetChannel(ID2RCHNL(i));
            CC.Recalibrate();
            uint8_t RxRslt = CC.Receive(27, &PktRx, &Rssi);   // Double pkt duration + TX sleep time
            if(RxRslt == retvOk) {
//                Printf("Ch=%u; Rssi=%d\r", ID2RCHNL(i), Rssi);
                if(PktRx.DWord32 == THE_WORD and Rssi > RSSI_MIN) {
                    if((AppMode == appmRxGrp1 and PktRx.AppMode == appmTxGrp1) or
                       (AppMode == appmRxGrp2 and PktRx.AppMode == appmTxGrp2)) {
                        RxTable.AddId(i);
                    }
                }
            }
        } // for i
        TryToSleep(270);
    } // For N
    EvtQMain.SendNowOrExit(EvtMsg_t(evtIdRadio, RxTable.GetCount()));
    RxTable.Clear();
}

void rLevel1_t::TaskFeelEachOtherMany() {
    static uint32_t PollCounter = 0;
    PktTx.ID = ID;
    PktTx.AppMode = (uint32_t)AppMode;
    for(uint32_t CycleN=0; CycleN < CYCLE_CNT; CycleN++) {  // Iterate cycles
        uint32_t TxSlot = Random::Generate(0, (SLOT_CNT-1));          // Decide when to transmit
        // If TX slot is not zero, receive at zero cycle or sleep otherwise
        if(TxSlot != 0) {
            uint32_t TimeBefore_ms = TxSlot * SLOT_DURATION_MS;
            if(CycleN == 0 and RxTable.GetCount() < RXTABLE_SZ) TryToReceive(TimeBefore_ms);
            else TryToSleep(TimeBefore_ms);
        }
        // ==== TX ====
        DBG1_SET();
        CC.Recalibrate();
        CC.Transmit(&PktTx);
        DBG1_CLR();

        // If TX slot is not last, receive at zero cycle or sleep otherwise
        if(TxSlot != (SLOT_CNT-1)) {
            uint32_t TimeAfter_ms = ((SLOT_CNT-1) - TxSlot) * SLOT_DURATION_MS;
            if(CycleN == 0 and RxTable.GetCount() < RXTABLE_SZ) TryToReceive(TimeAfter_ms);
            else TryToSleep(TimeAfter_ms);
        }
    } // for
    // Supercycle ended, decide if send message
    if(PollCounter == 1) {
        PollCounter = 0;
        EvtQMain.SendNowOrExit(EvtMsg_t(evtIdRadio, RxTable.GetCount()));
        RxTable.Clear();
    }
    else PollCounter++;
}

void rLevel1_t::TryToReceive(uint32_t TotalDuration_ms) {
    systime_t TimeStart_st = chVTGetSystemTimeX();
    uint32_t RxDur_ms = TotalDuration_ms;
    while(true) {
        CC.Recalibrate();
        uint8_t RxRslt = CC.Receive(RxDur_ms, &PktRx, &Rssi);
        if(RxRslt == retvOk) {
//            Printf("ID=%u; Rssi=%d\r", PktRx.ID, Rssi);
            if(Rssi > RSSI_MIN) {
                chSysLock();
                RxTable.AddId(PktRx.ID);
                chSysUnlock();
            }
        }
        // Check if repeat or get out
        uint32_t Elapsed_ms = ST2MS(chVTTimeElapsedSinceX(TimeStart_st));
        if(Elapsed_ms >= TotalDuration_ms) break;
        else RxDur_ms = TotalDuration_ms - Elapsed_ms;
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
        CC.SetTxPower(CC_PwrMinus30dBm);
        CC.SetPktSize(RPKT_LEN);
        CC.SetChannel(ID2RCHNL(ID));

        // Measure transmit duration
//        systime_t Start = chVTGetSystemTimeX();
//        CC.Recalibrate();
//        CC.Transmit(&PktTx);
//        Printf("Tx dur, st: %u\r", chVTTimeElapsedSinceX(Start));

//        CC.EnterPwrDown();
        // Thread
        chThdCreateStatic(warLvl1Thread, sizeof(warLvl1Thread), HIGHPRIO, (tfunc_t)rLvl1Thread, NULL);
        return retvOk;
    }
    else return retvFail;
}
#endif
