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
#include "led.h"

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
//static THD_WORKING_AREA(warLvl1Thread, 256);
//__NORETURN
//static void rLvl1Thread(void *arg) {
//    chRegSetThreadName("rLvl1");
//    Radio.ITask();
//}

__noreturn
void rLevel1_t::ITask() {
    __unused uint8_t OldID = 0;
    while(true) {
        __unused eventmask_t Evt = chEvtWaitAny(ALL_EVENTS);
//        if(Evt & EVT_NEW_9D) {
//            Pkt.Time = chVTGetSystemTime();

//            Pkt.AccData[0] = Acc.IPRead->Gyro.x;
//            Pkt.AccData[1] = Acc.IPRead->Gyro.y;
//            Pkt.AccData[2] = Acc.IPRead->Gyro.z;
//
//            Pkt.AccData[3] = Acc.IPRead->Acc.x;
//            Pkt.AccData[4] = Acc.IPRead->Acc.y;
//            Pkt.AccData[5] = Acc.IPRead->Acc.z;
//
//            Pkt.AccData[6] = Acc.IPRead->Magnet.x;
//            Pkt.AccData[7] = Acc.IPRead->Magnet.y;
//            Pkt.AccData[8] = Acc.IPRead->Magnet.z;

            DBG1_SET();
            CC.TransmitSync(&Pkt);
            DBG1_CLR();
//        }

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
        // ==== Transmitter ====
        if(App.MustTransmit) {
            if(App.ID != OldID) {
                OldID = App.ID;
                CC.SetChannel(ID2RCHNL(App.ID));
                Pkt.DWord = App.ID;
            }
            DBG1_SET();
            CC.TransmitSync(&Pkt);
            DBG1_CLR();
        }

        // ==== Receiver ====
        else {
            DBG2_SET();
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
        }
#endif
    } // while true
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
        CC.EnterPwrDown();
        // Thread
//        PThd = chThdCreateStatic(warLvl1Thread, sizeof(warLvl1Thread), HIGHPRIO, (tfunc_t)rLvl1Thread, NULL);
        return OK;
    }
    else return FAILURE;
}
#endif
