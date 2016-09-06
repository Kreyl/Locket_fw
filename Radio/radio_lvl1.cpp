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
    while(true) {
        if(Radio.MustSleep) Radio.TryToSleep(450);
        if     (App.Mode == modeDetectorTx) Radio.TaskTransmitter();
        else if(App.Mode == modeDetectorRx) Radio.TaskReceiverSingle();
        else if(App.Mode == modeBinding)    Radio.TaskFeelEachOtherSingle();
        else Radio.TryToSleep(450);
    } // while true
}

void rLevel1_t::TaskTransmitter() {
    CC.SetChannel(ID2RCHNL(App.ID));
    PktTx.DWord32 = THE_WORD;
    DBG1_SET();
    CC.TransmitSync(&PktTx);
    DBG1_CLR();
    TryToSleep(54);
}

void rLevel1_t::TaskReceiverSingle() {
    uint8_t Ch = ID2RCHNL(App.ID - 1);
    CC.SetChannel(Ch);
    uint8_t RxRslt = CC.ReceiveSync(117, &PktRx, &Rssi);   // Double pkt duration + TX sleep time
    if(RxRslt == OK) {
        Uart.Printf("Ch=%u; Rssi=%d\r", Ch, Rssi);
        if(PktRx.DWord32 == THE_WORD and Rssi > -63) App.SignalEvt(EVT_RADIO);
    }
    TryToSleep(450);
}

void rLevel1_t::TaskReceiverMany() {
    for(int N=0; N<2; N++) {
        // Iterate channels
        for(int32_t i = ID_MIN; i <= ID_MAX; i++) {
            if(i == App.ID) continue;   // Do not listen self
            CC.SetChannel(ID2RCHNL(i));
            uint8_t RxRslt = CC.ReceiveSync(17, &PktRx, &Rssi);   // Double pkt duration + TX sleep time
            if(RxRslt == OK) {
//                Uart.Printf("Ch=%u; Rssi=%d\r", ID2RCHNL(i), Rssi);
                if(PktRx.DWord32 == THE_WORD and Rssi > RSSI_MIN) RxTable.AddId(i);
                else Uart.Printf("PktErr\r");
            }
        } // for i
        TryToSleep(270);
    } // For N
    App.SignalEvt(EVT_RADIO); // RX table ready
}

void rLevel1_t::TaskFeelEachOtherSingle() {
    int8_t TopRssi = -126;
    // Alice is boss
    if((App.ID & 0x01) == 1) {
        CC.SetChannel(ID2RCHNL(App.ID));
        for(int i=0; i<7; i++) {
            DBG1_SET();
            CC.TransmitSync(&PktTx);
            DBG1_CLR();
            // Listen for an answer
            uint8_t RxRslt = CC.ReceiveSync(18, &PktRx, &Rssi);   // Double pkt duration + TX sleep time
            if(RxRslt == OK) {
                Uart.Printf("i=%d; Rssi=%d\r", i, Rssi);
                if(Rssi > TopRssi) TopRssi = Rssi;
            }
            TryToSleep(126);
        } // for
    }
    // Bob does what Alice says
    else {
        CC.SetChannel(ID2RCHNL(App.ID - 1));
        for(int i=0; i<7; i++) {
            // Listen for a command
            uint8_t RxRslt = CC.ReceiveSync(144, &PktRx, &Rssi);   // Double pkt duration + TX sleep time
            if(RxRslt == OK) {
                Uart.Printf("i=%d; Rssi=%d\r", i, Rssi);
                if(Rssi > TopRssi) TopRssi = Rssi;
                // Transmit reply
                DBG1_SET();
                CC.TransmitSync(&PktTx);
                DBG1_CLR();
            }
            TryToSleep(45);
        } // for
    }
    // Signal Evt if something received
    if(TopRssi > -126) {
        Rssi = TopRssi;
        App.SignalEvt(EVT_RADIO);
    }
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
