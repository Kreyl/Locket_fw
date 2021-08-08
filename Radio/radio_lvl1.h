/*
 * radio_lvl1.h
 *
 *  Created on: Nov 17, 2013
 *      Author: kreyl
 */

#pragma once

#include "kl_lib.h"
#include "ch.h"
#include "cc1101.h"
#include "shell.h"
#include "MsgQ.h"
#include "color.h"

#define CC_TX_PWR   CC_PwrPlus5dBm

#if 1 // =========================== Pkt_t =====================================
struct rPkt_t  {
    Color_t Clr;
    uint8_t Chnl;
    rPkt_t& operator = (const rPkt_t &Right) {
        Clr = Right.Clr;
        Chnl = Right.Chnl;
        return *this;
    }
//    void Print() { Printf("%d %d %d %d %d %d; %X\r", Ch[0],Ch[1],Ch[2],Ch[3],R1, R2, Btns); }
} __packed;

#define RPKT_LEN    sizeof(rPkt_t)
#endif

#define LONG_SLEEP_TIME_ms  450
#define RX_TIME_ms          27
#define SHORT_SLEEP_TIME_ms 36

class rLevel1_t {
private:
public:
    int8_t Rssi;
    rPkt_t rPkt;
    uint8_t Init();
    // Inner use
    void ITask();
};

extern rLevel1_t Radio;
