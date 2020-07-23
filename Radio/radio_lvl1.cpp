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
                Printf("Far: %u\r", CntTxFar);
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
                if(CntTxFar > 0) {
                    Printf("Far: %u\r", CntTxFar);
                    CntTxFar--;
                    TaskTransmitFar();
                }

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
                            Printf("Far t=%d; Rssi=%d\r", PktRx.Type, PktRx.Rssi);
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

/*
 *
         if(Cfg.IsAriKaesu()) {
            if(CntTxFar > 0) {
                Printf("FarAK: %u\r", CntTxFar);
                CntTxFar--;
                TaskTransmitFar();
            }
            else {
                CC.EnterIdle();
                chThdSleepMilliseconds(540);
            }
        }
        else {
            if(Cfg.MustTxInEachOther) TaskFeelEachOther();
            else TaskFeelEachOtherSilently(); // To allow be invisible
            // Transmit far if needed
            if(CntTxFar > 0) {
                Printf("Far: %u\r", CntTxFar);
                CntTxFar--;
                TaskTransmitFar();
            }
            else TaskFeelFar();
        }
 */


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
    sysinterval_t CycleDur_st = CYCLE_DURATION_ST;

    // First phase: try to Rx
//    Printf("F\r");
//    sysinterval_t Delay_st = TIME_MS2I(Random::Generate(0, MAX_RANDOM_DURATION_MS));
//    while(true) {
//        sysinterval_t Elapsed_st = chVTTimeElapsedSinceX(CycleStart_st);
//        if(Elapsed_st >= Delay_st) break;
//        else {
//            sysinterval_t RxDuration_st = Delay_st - Elapsed_st;
//            if(CC.RxIfNotYet_st(RxDuration_st, &PktRx, RPKT_LEN, &PktRx.Rssi) == retvOk) {
//                Printf("1 ID=%u; t=%d; Rssi=%d\r", PktRx.ID, PktRx.Type, PktRx.Rssi);
//                RxTableW->AddOrReplaceExistingPkt(PktRx);
//            }
//        }
//    }

    // Second phase: try to TX
//    Printf("S\r");
    while(true) {
        // Get out if there was no success trying to transmit
        if(chVTTimeElapsedSinceX(CycleStart_st) >= CycleDur_st) goto EndOfZeroCycle;
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
                    Printf("2 t=%d; Rssi=%d\r", PktRx.Type, PktRx.Rssi);
                    RxTableW->AddOrReplaceExistingPkt(PktRx);
                }
            } // while
        }
    } // while

    // Third phase: RX until end of cycle
//    Printf("T\r");
    while(true) {
        sysinterval_t Elapsed_st = chVTTimeElapsedSinceX(CycleStart_st);
        if(Elapsed_st >= CycleDur_st) break;
        else {
            sysinterval_t RxDuration_st = CycleDur_st - Elapsed_st;
            if(CC.RxIfNotYet_st(RxDuration_st, &PktRx, RPKT_LEN, &PktRx.Rssi) == retvOk) {
                Printf("3 ID=%u; t=%d; Rssi=%d\r", PktRx.ID, PktRx.Type, PktRx.Rssi);
                RxTableW->AddOrReplaceExistingPkt(PktRx);
            }
        }
    }
    EndOfZeroCycle:
    DBG1_CLR();

#if 1 // ==== Other cycles ====
    CC.EnterIdle();
    CC.DoIdleAfterTx();
    for(uint32_t CycleN=1; CycleN < RCYCLE_CNT; CycleN++) {   // Iterate cycles
//        Delay_st = TIME_MS2I(Random::Generate(0, MAX_RANDOM_DURATION_MS));
//        Printf("%u: %u\r", CycleN, Delay_st);
        CC.Recalibrate(); // After this, CC will be in IDLE state
        CycleStart_st = chVTGetSystemTimeX();
        // Sleep for Delay
//        if(Delay_st != 0) {
//            CC.EnterIdle();
//            chThdSleep(Delay_st);
//        }
        // Try to TX
        while(true) {
            // Get out of cycle if there was no success trying to transmit
            if(chVTTimeElapsedSinceX(CycleStart_st) >= CycleDur_st) break;
            // Try to transmit
            if(CC.RxCcaTx_st(&PktTx, RPKT_LEN, &PktRx.Rssi) == retvOk) {
//                Printf("TX: %u\r", chVTTimeElapsedSinceX(CycleStart_st));
                break; // Will be in IDLE if success
            }
            else { // Channel was occupied, wait some time
//                Printf("########################\r");
                CC.EnterIdle();
                uint32_t Delay_ms = Random::Generate(2, 18);
                chThdSleepMilliseconds(Delay_ms);
            }
        }
        // Sleep remainder of the cycle
        sysinterval_t Elapsed_st = chVTTimeElapsedSinceX(CycleStart_st);
        if(Elapsed_st < CycleDur_st) {
            CC.EnterIdle();
            CC.EnterPwrDown();
            sysinterval_t SleepDuration_st = CycleDur_st - Elapsed_st;
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
        if(Elapsed_st >= CYCLE_DURATION_ST) break;
        else {
            sysinterval_t RxDuration_st = CYCLE_DURATION_ST - Elapsed_st;
            if(CC.RxIfNotYet_st(RxDuration_st, &PktRx, RPKT_LEN, &PktRx.Rssi) == retvOk) {
                Printf("3 ID=%u; t=%d; Rssi=%d\r", PktRx.ID, PktRx.Type, PktRx.Rssi);
                RxTableW->AddOrReplaceExistingPkt(PktRx);
            }
        }
    }
    // Second phase: sleep
    CC.EnterIdle();
    CC.EnterPwrDown();
//    chThdSleepMilliseconds(CYCLE_DURATION_MS * (RCYCLE_CNT - 1));
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
