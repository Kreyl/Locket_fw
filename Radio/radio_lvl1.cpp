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
        if(msg.Cmd == R_MSG_SET_PWR) {
            CC.SetTxPower(msg.Value);
            Printf("NewPwr: %u\r", msg.Value);
        }
//        if(msg.Cmd == R_MSG_SET_CHNL) CC.SetChannel(msg.Value);
        // Process task
        if(AppMode == appmTx) Radio.TaskTransmitter();
        else if(AppMode == appmRx) Radio.TaskReceiverManyByID();
        else chThdSleepMilliseconds(270);
    } // while true
}

void rLevel1_t::TaskTransmitter() {
    CC.SetChannel(ID2RCHNL(ID));
//    CC.SetChannel(RCHNL_COMMON);
    PktTx.ID = ID;
//    PktTx.R = 0;
//    PktTx.G = 255;
//    PktTx.B = 0;
    DBG1_SET();
    CC.Recalibrate();
    CC.Transmit(&PktTx, RPKT_LEN);
    DBG1_CLR();
    chThdSleepMilliseconds(27);
}

void rLevel1_t::TaskReceiverManyByID() {
    int8_t Rssi;
    for(int N=0; N<4; N++) { // Iterate channels N times
        TryToSleep(270);
        // Iterate channels
        for(int32_t i = ID_MIN; i <= ID_MAX; i++) {
            if(i == ID) continue;   // Do not listen self
            CC.SetChannel(ID2RCHNL(i));
//            Printf("%u\r", i);
            CC.Recalibrate();
            uint8_t RxRslt = CC.Receive(54, &PktRx, RPKT_LEN, &Rssi);   // Double pkt duration + TX sleep time
            if(RxRslt == retvOk) {
                Printf("Ch=%u; Rssi=%d\r", ID2RCHNL(i), Rssi);
                if(PktRx.ID == i) RxTable.AddId(i);
            }
        } // for i
    } // For N
    EvtQMain.SendNowOrExit(EvtMsg_t(evtIdCheckRxTable));
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
        CC.SetChannel(ID2RCHNL(ID));
//        CC.EnterPwrDown();
        // Thread
        chThdCreateStatic(warLvl1Thread, sizeof(warLvl1Thread), HIGHPRIO, (tfunc_t)rLvl1Thread, NULL);
        return retvOk;
    }
    else return retvFail;
}
#endif
