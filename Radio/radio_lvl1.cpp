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
#define DBG_PIN2    11
#define DBG2_SET()  PinSetHi(DBG_GPIO2, DBG_PIN2)
#define DBG2_CLR()  PinSetLo(DBG_GPIO2, DBG_PIN2)
#else
#define DBG1_SET()
#define DBG1_CLR()
#endif

rLevel1_t Radio;

static Timer_t IHwTmr(TIM9);
static volatile uint32_t NextTick = 0;
static volatile uint16_t TimeSrcId = 0;
static volatile rPkt_t PktRx, PktTx;

/*
static volatile enum CCState_t {ccstIdle, ccstRx, ccstTx} CCState = ccstIdle;


static class RadioTime_t {
private:
    void StopTimerI()  { IHwTmr.Disable(); }
    uint16_t TimeSrcTimeout;

    void IncCycle() {
//        DBG1_CLR();
        CycleN++;
        if(CycleN >= RCYCLE_CNT) { // Supercycle ended
//            DBG1_SET();
            CycleN = 0;
            Radio.RMsgQ.SendNowOrExitI(RMsg_t(rmsgEachOthRx));
            // Check TimeSrc timeout
            if(TimeSrcTimeout >= SCYCLES_TO_KEEP_TIMESRC) TimeSrcId = Cfg.ID;
            else TimeSrcTimeout++;
        }
        StartTimerForTSlotI();
    }
public:
    volatile int32_t CycleN = 0, TimeSlot = 0;
    uint16_t TimeSrcId = 0; // dummy
    void StartTimerForTSlotI() {
        IHwTmr.SetCounter(0);
        IHwTmr.SetTopValue(TIMESLOT_DUR_TICKS);
        IHwTmr.GenerateUpdateEvt();
        IHwTmr.Enable();
    }
    void StartTimerForAdjustI() {
        IHwTmr.Disable();
        TIM9->SR &= ~TIM_SR_UIF;
        IHwTmr.SetCounter(0);
        IHwTmr.SetTopValue(ADJUST_DELAY_TICS);
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
        IncTimeSlot();
        // Transmit, if it is our timeslot and transmitting allowed
        if(TimeSlot == Cfg.ID and Cfg.MustTxInEachOther()) Radio.RMsgQ.SendNowOrExitI(RMsg_t(rmsgEachOthTx));
        else { // Not our timeslot
            if(CycleN == 0) { // Enter RX if not yet
                if(CCState != ccstRx) Radio.RMsgQ.SendNowOrExitI(RMsg_t(rmsgEachOthRx));
            }
            else { // CycleN != 0
                if(CCState != ccstIdle) Radio.RMsgQ.SendNowOrExitI(RMsg_t(rmsgEachOthSleep));
            }
        }
    }
} RadioTime;
*/

static void RxCallback() {
    DBG1_SET();
    if(CC.ReadFIFO((uint8_t*)&PktRx, (int8_t*)&PktRx.Rssi, RPKT_LEN) == retvOk) {  // if pkt successfully received
//        if(Radio.PktRx.Salt == RPKT_SALT) {
//            RadioTime.AdjustI();
//            Radio.RMsgQ.SendNowOrExitI(RMsg_t(rmsgPktRx));
        PrintfI("rx %u\r", PktRx.ID);
//        }
    }
    else PrintfI("errx\r");
    if(IHwTmr.GetCounter() < CYCLE_DUR_TICKS) CC.ReceiveAsyncI(RxCallback);
    DBG1_CLR();
}

// After TX done, enter either RX in cycle 0 or Sleep in other case
static void TxCallback() {
    if(IHwTmr.GetCounter() < CYCLE_DUR_TICKS) CC.ReceiveAsyncI(RxCallback);
//    else CC.EnterIdle(); // XXX
//    PrintfI("t");
}

static void IOnNewSupercycleI() {
    DBG2_SET();
    // Prepare to tx if needed
//    if(Cfg.MustTxInEachOther()) {

        NextTick = (Cfg.ID + 1) * TIMESLOT_DUR_TICKS; // skip first timeslot to allow recalibration to complete
        IHwTmr.SetCCR1(NextTick);
        IHwTmr.EnableIrqOnCompare1();
        PktTx.ID = Cfg.ID;
        PktTx.Type = Cfg.Type;
        //*/
//    }
//    else IHwTmr.DisableIrqOnCompare1();
    CC.Recalibrate();
    // Enter RX if ID is not 0 - otherwise immediate TX is required
    if(Cfg.ID != 0) CC.ReceiveAsyncI(RxCallback);
    DBG2_CLR();
}

// Will be here if IRQ is enabled, which is determined at end of supercycle
static void IOnTxSlotI() {
//    DBG1_SET();
    // Setup next time
    NextTick += CYCLE_DUR_TICKS;
    if(NextTick < SUPERCYCLE_DUR_TICKS) IHwTmr.SetCCR1(NextTick); // Do not set CCR greater than TOP value, as it will fire at overflow.
    // Transmit pkt
    PktTx.TimeSrcID = TimeSrcId;
    PktTx.iTime = IHwTmr.GetCounter();
    CC.TransmitAsyncX((uint8_t*)&PktTx, RPKT_LEN, TxCallback);
//    DBG1_CLR();
}

static void IOnCycle0EndI() {
    CC.EnterIdle();
//    DBG1_SET();
}


extern "C"
void VectorA4() {
    CH_IRQ_PROLOGUE();
    chSysLockFromISR();
    uint32_t SR = TIM9->SR & TIM9->DIER; // Process enabled irqs only
    TIM9->SR = 0; // Clear flags
    if(SR & TIM_SR_CC1IF) IOnTxSlotI();
    if(SR & TIM_SR_CC2IF) IOnCycle0EndI();
    if(SR & TIM_SR_UIF)   IOnNewSupercycleI();
    chSysUnlockFromISR();
    CH_IRQ_EPILOGUE();
}

#if 0 // ================================ Task =================================
static THD_WORKING_AREA(warLvl1Thread, 256);
__noreturn
static void rLvl1Thread(void *arg) {
    chRegSetThreadName("rLvl1");
    Radio.ITask();
}

__noreturn
void rLevel1_t::ITask() {
    while(true) {
        CC.Recalibrate();
//        CC.Transmit((uint8_t*)&PktTx, RPKT_LEN);
//        CC.TransmitAsyncX((uint8_t*)&PktTx, RPKT_LEN, TxCallback);
//        chThdSleepMilliseconds(45);

        if(CC.Receive(360, (uint8_t*)&PktRx, RPKT_LEN, (int8_t*)&PktRx.Rssi) == retvOk) {
            Printf("rx\r");
        }
        Printf("t");

//
        /*
        RMsg_t msg = RMsgQ.Fetch(TIME_INFINITE);
        switch(msg.Cmd) {
            case rmsgEachOthTx:
                CC.EnterIdle();
                CCState = ccstTx;
                PktTx.ID = Cfg.ID;
//                PktTx.Cycle = RadioTime.CycleN;
                PktTx.TimeSrcID = RadioTime.TimeSrcId;
                PktTx.Salt = 0xCA11;
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
//                            Printf("ID=%u; t=%d; Rssi=%d\r", PktRx.ID, PktRx.Type, PktRx.Rssi);
                RxTableW->AddOrReplaceExistingPkt(PktRx);
                break;
        } // switch
        */
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
        CC.SetChannel(RCHNL_EACH_OTH);
//        CC.SetTxPower(Cfg.TxPower);
        CC.SetTxPower(CC_Pwr0dBm);
        CC.SetBitrate(CCBitrate500k);
//        CC.SetBitrate(CCBitrate100k);
//        CC.EnterPwrDown();

        PktTx.Salt = RPKT_SALT;
        TimeSrcId = Cfg.ID;

        // Setup HW Timer: input is CC's (27MHz/192)
        // IRQs: UPD is new supercycle, CCR1 is TX, CCR2 is end of Cycle0
        IHwTmr.Init();
        // Setup ext input
        PinSetupAlterFunc(GPIOA, 2, omPushPull, pudNone, AF3);
        IHwTmr.SelectSlaveMode(smExternal);
        IHwTmr.SetTriggerInput(tiTI1FP1);
        // Setup timings
        IHwTmr.SetPrescaler(RTIM_PRESCALER);
        IHwTmr.SetTopValue(SUPERCYCLE_DUR_TICKS); // Will update at supercycle end
        IHwTmr.SetCCR2(CYCLE_DUR_TICKS);          // Will fire at end of cycle 0
        // Setup IRQs
        IHwTmr.EnableIrqOnUpdate();   // New supercycle
        IHwTmr.EnableIrqOnCompare2(); // End of cycle 0
        IHwTmr.EnableIrq(TIM9_IRQn, IRQ_PRIO_VERYHIGH);
        IHwTmr.Enable();
//*/
        // Thread
//        chThdCreateStatic(warLvl1Thread, sizeof(warLvl1Thread), HIGHPRIO, (tfunc_t)rLvl1Thread, NULL);
        return retvOk;
    }
    else return retvFail;
}
#endif
