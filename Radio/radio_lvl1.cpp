/*
 * radio_lvl1.cpp
 *
 *  Created on: Nov 17, 2013
 *      Author: kreyl
 */

#include "radio_lvl1.h"
#include "cc1101.h"
#include "uart.h"

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

static rPkt_t pkt_rx;
static RxTable tbl1, tbl2, *curr_tbl = &tbl1;


static THD_WORKING_AREA(warLvl1Thread, 256);
__noreturn
static void rLvl1Thread(void *arg) {
    chRegSetThreadName("rLvl1");
    while(true) {
        for(int i=0; i<4; i++) {
            chThdSleepMilliseconds(720);
            sysinterval_t total_duration_st = TIME_MS2I(450);
            sysinterval_t start_time_st = chVTGetSystemTimeX();
            sysinterval_t time_left_st = total_duration_st;
            int8_t rssi;
            CC.Recalibrate();
            while(true) {
                DBG2_SET();
                retv rx_rslt = CC.Receive_st(time_left_st, (uint8_t*)&pkt_rx, RPKT_LEN, &rssi);
                DBG2_CLR();
                if(rx_rslt == retv::Ok and pkt_rx.the_word == 0xCa110fEa) {
//                    Printf("%u %d\r", pkt_rx.id, rssi);
                    curr_tbl->AddOrReplaceExistingPkt(pkt_rx);
                }
                // Check if rx more or get out
                systime_t elapsed_st = chVTTimeElapsedSinceX(start_time_st);
                if(elapsed_st >= total_duration_st) break;
                else time_left_st = total_duration_st - elapsed_st;
            }
            CC.EnterIdle();
        } // for
        // Send rxtable if not empty
        if(curr_tbl->cnt != 0) { // Report and switch table if not empty
            chSysLock();
            EvtQMain.SendNowOrExitI(EvtMsg_t(EvtId::CheckRxTable, (void*)curr_tbl));
            curr_tbl = (curr_tbl == &tbl1)? &tbl2 : &tbl1;
            curr_tbl->Clear();
            chSysUnlock();
        }
    } // while true
}

retv RadioInit() {
#ifdef DBG_PINS
    PinSetupOut(DBG_GPIO1, DBG_PIN1, omPushPull);
    PinSetupOut(DBG_GPIO2, DBG_PIN2, omPushPull);
#endif

    if(CC.Init() == retv::Ok) {
        CC.SetPktSize(RPKT_LEN);
        CC.DoIdleAfterTx();
        CC.SetChannel(7);
        CC.SetBitrate(CCBitrate100k);
        CC.SetTxPower(CC_Pwr0dBm);
        // Thread
        chThdCreateStatic(warLvl1Thread, sizeof(warLvl1Thread), HIGHPRIO, (tfunc_t)rLvl1Thread, NULL);
        return retv::Ok;
    }
    else return retv::Fail;
}
