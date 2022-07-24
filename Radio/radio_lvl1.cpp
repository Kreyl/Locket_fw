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
volatile uint32_t tdelay = 53;

static Timer_t IHwTmr(TIM9);
static volatile uint8_t TimeSrc, HopCnt;
static volatile rPkt_t PktRx, PktTx;
static volatile uint32_t TimeSrcTimeout = 0;

// ================== Tx preparation and time adjustment =======================
static void PrepareNextTx() {
    if(Cfg.MustTxInEachOther()) {
        IHwTmr.EnableIrqOnCompare1(); // Always, as nothing changes when enabled
        uint32_t TxTime = (Cfg.ID + ZERO_ID_INCREMENT) * TIMESLOT_DUR_TICKS; // skip first timeslot to allow recalibration to complete
        uint32_t t = IHwTmr.GetCounter();
        while(t >= TxTime) TxTime += CYCLE_DUR_TICKS;
        if(TxTime < SUPERCYCLE_DUR_TICKS) IHwTmr.SetCCR1(TxTime); // Do not set CCR greater than TOP value, as it will fire at overflow.
        PktTx.ID = Cfg.ID;
        PktTx.Type = Cfg.Type;
    }
    else IHwTmr.DisableIrqOnCompare1();
}

// Adjust time if hops cnt is not too large and if theirs TimeSrc < OursTimeSrc or if TimeSrc is the same, use one with less hops.
static void AdjustRadioTimeI() {
    if(PktRx.TimeSrc < Cfg.ID and PktRx.HopCnt < HOPS_CNT_MAX and (PktRx.TimeSrc < TimeSrc or (PktRx.TimeSrc == TimeSrc and PktRx.HopCnt < HopCnt))) {
        TimeSrc = PktRx.TimeSrc;
        HopCnt = PktRx.HopCnt + 1; // Increment hopcnt to show that it is from someone else
        TimeSrcTimeout = 0; // Reset Time Src Timeout
        TIM9->SR = 0; // Clear flags
        uint32_t t = PktRx.iTime;
        // Increment time to take into account duration of pkt transmission
        t += tdelay; //TX_DUR_TICS;
        IHwTmr.SetCounter(t);
        PrepareNextTx();
    }
}

static inline bool IsInZeroCycle() { return (IHwTmr.GetCounter() < CYCLE_DUR_TICKS); }

// =========================== TX and RX callbacks =============================
static void RxCallback() {
    if(CC.ReadFIFO((uint8_t*)&PktRx, (int8_t*)&PktRx.Rssi, RPKT_LEN) == retvOk) {  // if pkt successfully received
        if(PktRx.Salt == RPKT_SALT) {
//        PrintfI("%u ID=%u ts=%u h=%u t=%u\r", IHwTmr.GetCounter(), PktRx.ID, PktRx.TimeSrc, PktRx.HopCnt, PktRx.iTime);
            AdjustRadioTimeI();
            Radio.AddPktToRxTableI((rPkt_t*)&PktRx);
        }
    }
    if(IsInZeroCycle()) CC.ReceiveAsyncI(RxCallback);
    else CC.EnterIdle();
    DBG1_CLR();
}

// After TX done, enter either RX in cycle 0 or Sleep in other case
static void TxCallback() {
    if(IsInZeroCycle()) CC.ReceiveAsyncI(RxCallback);
    DBG1_CLR();
}

// ============================ Timing IRQ handlers ============================
static void IOnNewSupercycleI() {
    DBG2_SET();
    if(Radio.TxPower != Cfg.TxPower) {
        Radio.TxPower = Cfg.TxPower;
        CC.SetTxPower(Cfg.TxPower);
    }
    // Check if time to reset TimeSrc to self ID. Will overflow eventually, does not make sense
    if(++TimeSrcTimeout > SCYCLES_TO_KEEP_TIMESRC) {
        TimeSrc = Cfg.ID;
        HopCnt = 0;
    }
    CC.Recalibrate();
    // Enter RX if ID is not 0 - otherwise immediate TX is required
    CC.ReceiveAsyncI(RxCallback);
    PrepareNextTx(); // Prepare to tx if needed
    DBG2_CLR();
}

// Will be here if IRQ is enabled, which is determined at end of supercycle
static void IOnTxSlotI() {
    DBG1_SET();
    CC.Recalibrate();
    PktTx.TimeSrc = TimeSrc;
    PktTx.HopCnt = HopCnt;
    PktTx.iTime = IHwTmr.GetCounter();
    CC.TransmitCcaX((uint8_t*)&PktTx, RPKT_LEN, TxCallback);
    PrepareNextTx();
}

static void IOnCycle0EndI() {
    CC.EnterIdle();
}

// ==== Timer IRQ ====
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
        Radio.TxPower = Cfg.TxPower;
        CC.SetBitrate(CCBitrate500k);

        PktTx.Salt = RPKT_SALT;
        TimeSrc = Cfg.ID;
        HopCnt = 0;

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

        return retvOk;
    }
    else return retvFail;
}
#endif
