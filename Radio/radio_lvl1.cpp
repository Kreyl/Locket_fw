/*
 * radio_lvl1.cpp
 *
 *  Created on: Nov 17, 2013
 *      Author: kreyl
 */

#include "radio_lvl1.h"
#include "evt_mask.h"
#include "main.h"
#include "cc1101.h"
#include "uart.h"
//#include "led.h"

//#define DBG_PINS

#ifdef DBG_PINS
#define DBG_GPIO1   GPIOB
#define DBG_PIN1    4
#define DBG1_SET()  PinSet(DBG_GPIO1, DBG_PIN1)
#define DBG1_CLR()  PinClear(DBG_GPIO1, DBG_PIN1)
#define DBG_GPIO2   GPIOB
#define DBG_PIN2    9
#define DBG2_SET()  PinSet(DBG_GPIO2, DBG_PIN2)
#define DBG2_CLR()  PinClear(DBG_GPIO2, DBG_PIN2)
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
    Radio.ITask();
}

__noreturn
void rLevel1_t::ITask() {
    while(true) {
        if     (App.Mode == modeDetectorTx) TaskTransmitter();
        else if(App.Mode == modeDetectorRx) TaskReceiverSingle();
        else if(App.Mode == modeBinding)    TaskFeelEachOther();

        // ==== Receiver ====
        else {
        }

#if 0        // Demo
        if(App.Mode == 0b0001) { // RX
            int8_t Rssi;
            Color_t Clr;
            uint8_t RxRslt = CC.ReceiveSync(RX_T_MS, &Pkt, &Rssi);
            if(RxRslt == OK) {
                Uart.Printf("\rRssi=%d", Rssi);
                Clr = clWhite;
                if     (Rssi < -100) Clr = clRed;
                else if(Rssi < -90) Clr = clYellow;
                else if(Rssi < -80) Clr = clGreen;
                else if(Rssi < -70) Clr = clCyan;
                else if(Rssi < -60) Clr = clBlue;
                else if(Rssi < -50) Clr = clMagenta;
            }
            else Clr = clBlack;
            Led.SetColor(Clr);
            chThdSleepMilliseconds(99);
        }
        else {  // TX
            DBG1_SET();
            CC.TransmitSync(&Pkt);
            DBG1_CLR();
//            chThdSleepMilliseconds(99);
        }
//#else
#endif

#if 0
        // Listen if nobody found, and do not if found
        int8_t Rssi;
        // Iterate channels
        for(int32_t i = ID_MIN; i <= ID_MAX; i++) {
            if(i == App.ID) continue;   // Do not listen self
            CC.SetChannel(ID2RCHNL(i));
            uint8_t RxRslt = CC.ReceiveSync(RX_T_MS, &Pkt, &Rssi);
            if(RxRslt == OK) {
//                    Uart.Printf("\rCh=%d; Rssi=%d", i, Rssi);
                App.SignalEvt(EVTMSK_SOMEONE_NEAR);
                break; // No need to listen anymore if someone already found
            }
        } // for
        CC.SetChannel(ID2RCHNL(App.ID));    // Set self channel back
        DBG2_CLR();
        TryToSleep(RX_SLEEP_T_MS);
#endif
    } // while true
}

void rLevel1_t::TaskTransmitter() {
    CC.SetChannel(ID2RCHNL(App.ID));
    Pkt.DWord32 = THE_WORD;
    DBG1_SET();
    CC.TransmitSync(&Pkt);
    DBG1_CLR();
    TryToSleep(54);
}

void rLevel1_t::TaskReceiverSingle() {
    uint8_t Ch = ID2RCHNL(App.ID - 1);
    CC.SetChannel(Ch);
    uint8_t RxRslt = CC.ReceiveSync(117, &Pkt, &Rssi);   // Double pkt duration + TX sleep time
    if(RxRslt == OK) {
        Uart.Printf("Ch=%u; Rssi=%d\r", Ch, Rssi);
        if(Pkt.DWord32 == THE_WORD and Rssi > -63) App.SignalEvt(EVT_RADIO);
    }
    TryToSleep(450);
}

void rLevel1_t::TaskReceiverMany() {
    for(int N=0; N<2; N++) {
        // Iterate channels
        for(int32_t i = ID_MIN; i <= ID_MAX; i++) {
            if(i == App.ID) continue;   // Do not listen self
            CC.SetChannel(ID2RCHNL(i));
            uint8_t RxRslt = CC.ReceiveSync(17, &Pkt, &Rssi);   // Double pkt duration + TX sleep time
            if(RxRslt == OK) {
//                        Uart.Printf("Ch=%u; Rssi=%d\r", ID2RCHNL(i), Rssi);
                if(Pkt.DWord32 == THE_WORD and Rssi > RSSI_MIN) RxTable.AddId(i);
                else Uart.Printf("PktErr\r");
            }
        } // for i
        TryToSleep(270);
    } // For N
    App.SignalEvt(EVT_RADIO); // RX table ready
}

void rLevel1_t::TaskFeelEachOther() {
    chThdSleepMilliseconds(450);
}
#endif // task

#if 1 // ============================
uint8_t rLevel1_t::Init() {
#ifdef DBG_PINS
    PinSetupOut(DBG_GPIO1, DBG_PIN1, omPushPull);
    PinSetupOut(DBG_GPIO2, DBG_PIN2, omPushPull);
#endif    // Init radioIC
    if(CC.Init() == OK) {
        CC.SetTxPower(CC_Pwr0dBm);
        CC.SetPktSize(RPKT_LEN);
        CC.SetChannel(ID2RCHNL(App.ID));
//        CC.EnterPwrDown();
        // Thread
        PThd = chThdCreateStatic(warLvl1Thread, sizeof(warLvl1Thread), HIGHPRIO, (tfunc_t)rLvl1Thread, NULL);
        return OK;
    }
    else return FAILURE;
}
#endif
