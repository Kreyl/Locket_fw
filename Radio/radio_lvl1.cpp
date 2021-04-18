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
#endif

rLevel1_t Radio;
static rPkt_t PktRx;

static Timer_t IHwTmr(TIM9);

static volatile enum CCState_t {ccstIdle, ccstRx, ccstTx} CCState = ccstIdle;

static class RadioTime_t {
private:
    void StopTimerI()  { IHwTmr.Disable(); }
    uint16_t TimeSrcTimeout;
public:
    volatile uint8_t CycleN = 0, TimeSlot = 0;
    uint16_t TimeSrcId = ID_MAX;

    void IncTimeSlot() {
        TimeSlot++;
        if(TimeSlot >= RSLOT_CNT) {  // Increment cycle
            TimeSlot = 0;
            DBG1_CLR();
            CycleN++;
            if(CycleN >= RCYCLE_CNT) { // New supercycle begins
                DBG1_SET();
                CycleN = 0;
                Radio.RMsgQ.SendNowOrExitI(RMsg_t(rmsgEachOthRx));
                // Check TimeSrc timeout
                if(TimeSrcTimeout >= SCYCLES_TO_KEEP_TIMESRC) TimeSrcId = Cfg.ID;
                else TimeSrcTimeout++;
            }
        }
    }

    void AdjustI() {
        if(PktRx.TimeSrcID <= TimeSrcId) { // Adjust time if theirs TimeSrc <= OursTimeSrc
            CycleN = PktRx.CycleN;
            TimeSlot = PktRx.ID;
            TimeSrcId = PktRx.TimeSrcID;
            TimeSrcTimeout = 0; // Reset Time Src Timeout
            IHwTmr.SetCounter(END_OF_PKT_SHIFT_tics);
        }
    }

    void IOnTimerI() {
        IncTimeSlot();
        // Tx if now is our timeslot
        if(TimeSlot == Cfg.ID) Radio.RMsgQ.SendNowOrExitI(RMsg_t(rmsgEachOthTx));
        // Not our timeslot
        else {
            if(CycleN == 0) { // Enter RX if not yet
                if(CCState != ccstRx) Radio.RMsgQ.SendNowOrExitI(RMsg_t(rmsgEachOthRx));
            }
            else { // CycleN != 0, enter sleep
                if(CCState != ccstIdle) Radio.RMsgQ.SendNowOrExitI(RMsg_t(rmsgEachOthSleep));
            }
        }
    }
} RadioTime;

void RxCallback() {
    if(CC.ReadFIFO(&PktRx, &PktRx.Rssi, RPKT_LEN) == retvOk) {  // if pkt successfully received
        DBG2_SET();
        RadioTime.AdjustI();
        Radio.RMsgQ.SendNowOrExitI(RMsg_t(rmsgPktRx));
    }
}

// RadioTime HW Timer IRQ. Calls OnTimer method.
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
    RMsg_t msg = RMsgQ.Fetch(TIME_INFINITE);
        switch(msg.Cmd) {
            case rmsgEachOthTx: {
                DBG2_SET();
                CC.EnterIdle();
                CCState = ccstTx;
                rPkt_t PktTx;
                PktTx.ID = Cfg.ID;
                PktTx.CycleN = RadioTime.CycleN;
                PktTx.TimeSrcID = RadioTime.TimeSrcId;
                PktTx.Type = Cfg.Type;
                CC.Recalibrate();
                CC.Transmit(&PktTx, RPKT_LEN);
                DBG2_CLR();
            } break;

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
//                Printf("ID=%u; Cyc=%d; TmrSrc=%u; Rssi=%d\r", PktRx.ID, PktRx.CycleN, PktRx.TimeSrcID, PktRx.Rssi);
//                Printf("ID=%u; Now=%u; Rxt: %u %u, RxSlot=%d; Slot=%u; sh=%u\r", PktRx.ID, IHwTmr.GetCounter(), rxtime1,rxtime2, rxslot, RadioTime.TimeSlot, TmrShift);
                RxTableW->AddOrReplaceExistingPkt(PktRx);
                DBG2_CLR();
                break;
            } // switch
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

        // Setup HW timer: it generates IRQ at timeslot end
//        Printf("RSLOT dur us: %u; tics: %u\r", RSLOT_DURATION_us, RSLOT_DURATION_tics);
        PinSetupAlterFunc(GPIOA, 2, omPushPull, pudNone, AF3); // TIM9 Ch1. This is clock input. CC generates stable clock at GDO2: CLK_XOSC/192 (27MHz / 192 = 140625 Hz)
        IHwTmr.Init();
        IHwTmr.SetTopValue(RSLOT_DURATION_tics);
        IHwTmr.SelectSlaveMode(smExternal);                     // }
        IHwTmr.SetTriggerInput(tiTI1FP1);                       // }
        IHwTmr.SetupInput1(0b01, Timer_t::pscDiv1, rfRising);   // } Timer is clocked by external clock, provided by CC
        IHwTmr.EnableIrq(TIM9_IRQn, IRQ_PRIO_HIGH);
        IHwTmr.EnableIrqOnUpdate();

        // Thread
        chThdCreateStatic(warLvl1Thread, sizeof(warLvl1Thread), HIGHPRIO, (tfunc_t)rLvl1Thread, NULL);
        chSysLock();
        RadioTime.TimeSrcId = Cfg.ID;
        IHwTmr.Enable(); // Start timer
        chSysUnlock();
        return retvOk;
    }
    else return retvFail;
}
#endif
