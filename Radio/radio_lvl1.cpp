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

extern Presser_t Presser;

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
static rPkt_t Pkt;
static int8_t Rssi;

#if 1 // ================================ Task =================================
static THD_WORKING_AREA(warLvl1Thread, 256);
__noreturn
static void rLvl1Thread(void *arg) {
    chRegSetThreadName("rLvl1");
    while(true) {
        CC.Recalibrate();
        uint8_t RxRslt = CC.Receive(RX_T_MS, &Pkt, RPKT_LEN, &Rssi);
        if(RxRslt == retvOk and Pkt.ID == ID) {
//            Printf("%u: %d; %u; %X\r", Pkt.ID, Rssi, Pkt.Cmd, Pkt.Clr.DWord32);
            Radio.PktRx = Pkt;
            if(Pkt.Cmd == CMD_GET) {
                Pkt.TimeAfterPress = Presser.GetTimeAfterPress();
//                if(Pkt.TimeAfterPress != -1) Printf("TAP: %d\r", Pkt.TimeAfterPress);
            }
            else {
                Pkt.Cmd = CMD_OK;
                Presser.Reset();
            }
            CC.Transmit(&Pkt, RPKT_LEN);
            EvtQMain.SendNowOrExit(EvtMsg_t(evtIdHostCmd));
        }
    } // while true
}
#endif // task

#if 1 // ============================
uint8_t rLevel1_t::Init() {
#ifdef DBG_PINS
    PinSetupOut(DBG_GPIO1, DBG_PIN1, omPushPull);
    PinSetupOut(DBG_GPIO2, DBG_PIN2, omPushPull);
#endif

    if(CC.Init() == retvOk) {
        CC.SetTxPower(CC_Pwr0dBm);
        CC.SetPktSize(RPKT_LEN);
        CC.SetChannel(RCHNL);
//        CC.EnterPwrDown();
        // Thread
        chThdCreateStatic(warLvl1Thread, sizeof(warLvl1Thread), HIGHPRIO, (tfunc_t)rLvl1Thread, NULL);
        return retvOk;
    }
    else return retvFail;
}
#endif
