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

static Timer_t IHwTmr(TIM9);

static volatile enum CCState_t {ccstIdle, ccstRx, ccstTx} CCState = ccstIdle;

static class RadioTime_t {
private:
    void StopTimerI()  { IHwTmr.Disable(); }
    uint16_t TimeSrcTimeout;
    void IncCycle() {
        DBG1_CLR();
        CycleN++;
        if(CycleN >= RCYCLE_CNT) {
            DBG1_SET();
            CycleN = 0;
            Radio.RMsgQ.SendNowOrExitI(RMsg_t(rmsgEachOthRx));
            // Check TimeSrc timeout
            if(TimeSrcTimeout >= SCYCLES_TO_KEEP_TIMESRC) TimeSrcId = Cfg.ID;
            else TimeSrcTimeout++;
            StartTimerForTSlotI();
        }
        else if(CycleN == FAR_CYCLE_INDX) {
            Radio.RMsgQ.SendNowOrExitI(RMsg_t(rmsgFar));
            StartTimerForFarCycleI();
        }
        else StartTimerForTSlotI();
    }
public:
    volatile int32_t CycleN = 0, TimeSlot = 0;
    uint16_t TimeSrcId = 63;
    void StartTimerForTSlotI() {
        IHwTmr.SetCounter(0);
        IHwTmr.SetTopValue(360);
        IHwTmr.GenerateUpdateEvt();
        IHwTmr.Enable();
    }
    void StartTimerForFarCycleI() {
        IHwTmr.SetCounter(0);
        IHwTmr.SetTopValue(5625);
        IHwTmr.GenerateUpdateEvt();
        IHwTmr.Enable();
    }
    void StartTimerForAdjustI() {
        IHwTmr.SetCounter(90);
        IHwTmr.SetTopValue(72);
        IHwTmr.GenerateUpdateEvt();
        IHwTmr.Enable();
    }

    void IncTimeSlot() {
        TimeSlot++;
        if(TimeSlot >= RSLOT_CNT) {
            TimeSlot = 0;
            IncCycle();
        }
        else StartTimerForTSlotI();
    }
    void AdjustI() {
        if(Radio.PktRx.TimeSrcID <= TimeSrcId) { // Adjust time if theirs TimeSrc < OursTimeSrc
            CycleN = Radio.PktRx.Cycle;
            TimeSlot = Radio.PktRx.ID; // Will be increased later, in IOnTimerI
            TimeSrcId = Radio.PktRx.TimeSrcID;
            TimeSrcTimeout = 0; // Reset Time Src Timeout
            StartTimerForAdjustI();
        }
    }

    void IOnTimerI() {
        if(CycleN == FAR_CYCLE_INDX) {
            IncCycle();
        }
        else { // Cycles 0...3
            IncTimeSlot();
            if(TimeSlot == Cfg.ID and Cfg.MustTxInEachOther) Radio.RMsgQ.SendNowOrExitI(RMsg_t(rmsgEachOthTx));
            else { // Not our timeslot
                if(CycleN == 0) { // Enter RX if not yet
                    if(CCState != ccstRx) Radio.RMsgQ.SendNowOrExitI(RMsg_t(rmsgEachOthRx));
                }
                else { // CycleN != 0
                    if(CCState != ccstIdle) Radio.RMsgQ.SendNowOrExitI(RMsg_t(rmsgEachOthSleep));
                }
            }
        }
    }
} RadioTime;

void RxCallback() {
    if(CC.ReadFIFO(&Radio.PktRx, &Radio.PktRx.Rssi, RPKT_LEN) == retvOk) {  // if pkt successfully received
        RadioTime.AdjustI();
        Radio.RMsgQ.SendNowOrExitI(RMsg_t(rmsgPktRx));
    }
}

extern "C"
void VectorA4() {
    CH_IRQ_PROLOGUE();
    chSysLockFromISR();
    TIM9->SR &= ~TIM_SR_UIF;
    RadioTime.IOnTimerI();
    chSysUnlockFromISR();
    CH_IRQ_EPILOGUE();
}

#if 1 // ================================ Task =================================
static THD_WORKING_AREA(warLvl1Thread, 256);
__noreturn
static void rLvl1Thread(void *arg) {
    chRegSetThreadName("rLvl1");
    Radio.ITask();
}

__noreturn
void rLevel1_t::ITask() {
    while(true) {
        if(CntTxFar > 0) {
            CC.SetChannel(RCHNL_FAR);
            CC.SetTxPower(TX_PWR_FAR);
            CC.SetBitrate(CCBitrate10k);
        //    CC.SetBitrate(CCBitrate2k4);
            while(CntTxFar--) {
//                Printf("Far: %u\r", CntTxFar);
                CC.Recalibrate();
                CC.Transmit(&PktTxFar, RPKT_LEN);
            }
            CC.EnterIdle();
            CC.SetChannel(RCHNL_EACH_OTH);
            CC.SetTxPower(Cfg.TxPower);
            CC.SetBitrate(CCBitrate500k);
        }
        else {
            if(Cfg.IsAriKaesu()) {
                CC.EnterIdle();
                chThdSleepMilliseconds(540);
            }
            else { // Not Ari/Kaesu
                RMsg_t msg = RMsgQ.Fetch(TIME_INFINITE);
                switch(msg.Cmd) {
                    case rmsgEachOthTx:
                        CC.EnterIdle();
                        CCState = ccstTx;
                        PktTx.ID = Cfg.ID;
                        PktTx.Cycle = RadioTime.CycleN;
                        PktTx.TimeSrcID = RadioTime.TimeSrcId;
                        CC.Recalibrate();
                        CC.Transmit(&PktTx, RPKT_LEN);
                        break;

                    case rmsgEachOthRx:
                        CCState = ccstRx;
                        CC.ReceiveAsync(RxCallback);
                        break;

                    case rmsgEachOthSleep:
                        CCState = ccstIdle;
                        CC.EnterIdle();
                        break;

                    case rmsgPktRx:
                        CCState = ccstIdle;
        //                    Printf("ID=%u; t=%d; Rssi=%d\r", PktRx.ID, PktRx.Type, PktRx.Rssi);
                        RxTableW->AddOrReplaceExistingPkt(PktRx);
                        break;

                    case rmsgFar:
                        CC.SetChannel(RCHNL_FAR);
                        CC.SetBitrate(CCBitrate10k);
                        CC.Recalibrate();
                        if(CC.Receive(36, &PktRx, RPKT_LEN, &PktRx.Rssi) == retvOk) {
//                            Printf("Far t=%d; Rssi=%d\r", PktRx.Type, PktRx.Rssi);
                            RxTableW->AddOrReplaceExistingPkt(PktRx);
                        }
                        CC.EnterIdle();
                        CC.SetChannel(RCHNL_EACH_OTH);
                        CC.SetBitrate(CCBitrate500k);
                        CC.Recalibrate();
                        break;
                } // switch
            } // if Not Ari/Kaesu
        }// if nothing to tx far
    } // while true
}
#endif // task

#if 1 // ============================
uint8_t rLevel1_t::Init() {
#ifdef DBG_PINS
    PinSetupOut(DBG_GPIO1, DBG_PIN1, omPushPull);
    PinSetupOut(DBG_GPIO2, DBG_PIN2, omPushPull);
#endif

    RMsgQ.Init();
    if(CC.Init() == retvOk) {
        CC.SetPktSize(RPKT_LEN);
        CC.DoIdleAfterTx();
        CC.SetChannel(RCHNL_EACH_OTH);
        CC.SetTxPower(Cfg.TxPower);
        CC.SetBitrate(CCBitrate500k);
//        CC.EnterPwrDown();

        PinSetupAlterFunc(GPIOA, 2, omPushPull, pudNone, AF3);
        IHwTmr.Init();
        IHwTmr.SetTopValue(14000);
        IHwTmr.SelectSlaveMode(smExternal);
        IHwTmr.SetTriggerInput(tiTI1FP1);
        IHwTmr.SetupInput1(0b01, Timer_t::pscDiv1, rfRising);
        IHwTmr.EnableIrq(TIM9_IRQn, IRQ_PRIO_HIGH);
        IHwTmr.EnableIrqOnUpdate();
        TIM9->CR1 |= TIM_CR1_URS;
        IHwTmr.Enable();

        // Thread
        chThdCreateStatic(warLvl1Thread, sizeof(warLvl1Thread), HIGHPRIO, (tfunc_t)rLvl1Thread, NULL);
        chSysLock();
        RadioTime.StartTimerForTSlotI();
        RadioTime.TimeSrcId = Cfg.ID;
        chSysUnlock();
        return retvOk;
    }
    else return retvFail;
}
#endif
