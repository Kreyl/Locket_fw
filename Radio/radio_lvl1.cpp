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
int8_t Rssi;
extern LedRGBwPower_t Led;

#if 1 // ================================ Task =================================
static THD_WORKING_AREA(warLvl1Thread, 256);
__noreturn
static void rLvl1Thread(void *arg) {
    chRegSetThreadName("rLvl1");
    Radio.ITask();
}

//#define TEST_STATION

__noreturn
void rLevel1_t::ITask() {
    while(true) {
        CC.Recalibrate();
#ifdef TEST_STATION
        uint8_t Rslt = CC.Receive(270, &PktRx, RPKT_LEN, &Rssi);
        if(Rslt == retvOk) {
            PktTx.Rssi = Rssi;
            CC.Transmit(&PktTx, RPKT_LEN);
            Printf("Rssi: our= %d; their=%d\r", Rssi, PktRx.Rssi);
            Led.StartOrRestart(lsqBlink);
        }
#else
        CC.Transmit(&PktTx, RPKT_LEN);
        uint8_t Rslt = CC.Receive(270, &PktRx, RPKT_LEN, &Rssi);
        if(Rslt == retvOk) {
            Printf("Rssi: our= %d; their=%d\r", Rssi, PktRx.Rssi);
            Led.StartOrRestart(lsqBlink);
        }
        chThdSleepMilliseconds(630);
#endif
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
        CC.SetTxPower(CC_Pwr0dBm);
        CC.SetBitrate(CCBitrate100k);
//        CC.EnterPwrDown();

        // Thread
        chThdCreateStatic(warLvl1Thread, sizeof(warLvl1Thread), HIGHPRIO, (tfunc_t)rLvl1Thread, NULL);
        return retvOk;
    }
    else return retvFail;
}
#endif
