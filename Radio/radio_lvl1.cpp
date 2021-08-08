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
    Radio.ITask();
}

static uint32_t SleepTime = LONG_SLEEP_TIME_ms, TryCnt = 0;

__noreturn
void rLevel1_t::ITask() {
    while(true) {
        CC.Recalibrate();
        if(CC.Receive(RX_TIME_ms, &rPkt, RPKT_LEN, &Rssi) == retvOk) {
            Printf("%u %u %u; %d\r", rPkt.Clr.R, rPkt.Clr.G, rPkt.Clr.B, Rssi);
            SleepTime = SHORT_SLEEP_TIME_ms;
            TryCnt = 0;
            EvtMsg_t Msg;
            Msg.ID = evtIdRadioCmd;
            Msg.Value = (int32_t)rPkt.Clr.DWord32;
            Msg.ValueID = rPkt.Chnl;
            EvtQMain.SendNowOrExit(Msg);
        }
        else {
            TryCnt++;
            if(TryCnt >= 7) {
                TryCnt = 0;
                SleepTime = LONG_SLEEP_TIME_ms;
            }
        }

        // Sleep
        if(SleepTime > SHORT_SLEEP_TIME_ms) CC.EnterPwrDown();
        chThdSleepMilliseconds(SleepTime);
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
        CC.SetTxPower(CC_TX_PWR);
        CC.SetPktSize(RPKT_LEN);
        CC.SetChannel(0);
        CC.SetBitrate(CCBitrate250k);
        // Thread
        chThdCreateStatic(warLvl1Thread, sizeof(warLvl1Thread), HIGHPRIO, (tfunc_t)rLvl1Thread, NULL);
        return retvOk;
    }
    else return retvFail;
}
#endif
