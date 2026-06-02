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

// #define DBG_PINS

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

extern uint8_t tx_pwr;
uint8_t old_pwr = 0x00;
extern uint32_t be_test_station;
extern LedRGBwPower_t<11> led;

namespace Radio {

static rPkt pkt_rx, pkt_tx;

static THD_WORKING_AREA(warLvl1Thread, 256);
__noreturn
static void rLvl1Thread(void *arg) {
    chRegSetThreadName("rLvl1");
    while(true) {
        CC.Recalibrate();
        if(be_test_station) {
            retv rslt = CC.Receive(270, (uint8_t*)&pkt_rx, kRPktSz, &pkt_tx.rssi);
            if(rslt == retv::Ok) {
                CC.Transmit((uint8_t*)&pkt_tx, kRPktSz);
                Printf("Rssi: our= %d; their=%d\r", pkt_tx.rssi, pkt_rx.rssi);
                led.StartOrRestart(lsqBlink);
            }
        }
        else {
            CC.Transmit((uint8_t*)&pkt_tx, kRPktSz);
            retv rslt = CC.Receive(270, (uint8_t*)&pkt_rx, kRPktSz, &pkt_tx.rssi);
            if(rslt == retv::Ok) {
                Printf("Rssi: our= %d; their=%d\r", pkt_tx.rssi, pkt_rx.rssi);
                led.StartOrRestart(lsqBlink);
            }
            chThdSleepMilliseconds(630);
        }
        if(old_pwr != tx_pwr) {
            old_pwr = tx_pwr;
            CC.SetTxPower(tx_pwr);
            Printf("TxPwr: 0x%02X\n", tx_pwr);
        }
    } // while true
}


retv Init() {
#ifdef DBG_PINS
    PinSetupOut(DBG_GPIO1, DBG_PIN1, omPushPull);
    PinSetupOut(DBG_GPIO2, DBG_PIN2, omPushPull);
#endif

    if(CC.Init() == retv::Ok) {
        CC.SetPktSize(kRPktSz);
        CC.SetChannel(0);
        CC.SetTxPower(CC_Pwr0dBm);
        // CC.SetBitrate(CCBitrate500k);
        // CC.SetBitrate(CCBitrate250k);
        CC.SetBitrate(CCBitrate100k);
        // CC.SetBitrate(CCBitrate38k4);
        // CC.SetBitrate(CCBitrate10k);
        // CC.SetBitrate(CCBitrate2k4);
        // Thread
        chThdCreateStatic(warLvl1Thread, sizeof(warLvl1Thread), HIGHPRIO, (tfunc_t)rLvl1Thread, NULL);
        return retv::Ok;
    }
    else return retv::Fail;
}

} // namespace Radio