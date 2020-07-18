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
#include "kl_buf.h"
#include "uart.h"
#include "color.h"

#if 1 // =========================== Pkt_t =====================================
struct rPkt_t {
    union {
        uint32_t DWord[2];
        int32_t TimeAfterPress;
        struct {
            uint8_t R1, G1, B1, R2, G2, B2;
            uint16_t Wait_ms;
        } __attribute__ ((__packed__));
    } __attribute__ ((__packed__));
    uint8_t Cmd;
    uint8_t ID;
//    bool operator == (const rPkt_t &APkt) { return (DWord32 == APkt.DWord32); }
    rPkt_t& operator = (const rPkt_t &Right) {
        chSysLock();
        DWord[0] = Right.DWord[0];
        DWord[1] = Right.DWord[1];
        Cmd = Right.Cmd;
        ID = Right.ID;
        chSysUnlock();
        return *this;
    }
    rPkt_t() : TimeAfterPress(0), Cmd(0) {}
} __attribute__ ((__packed__));
#endif


#if 1 // ============================== Defines ================================
#define RCHNL                   0
#define RPKT_LEN                sizeof(rPkt_t)
#define CC_TX_PWR               CC_Pwr0dBm

#define CMD_GET                 18
#define CMD_SET                 27
#define CMD_FLASH               36
#define CMD_OK                  0

#define RX_T_MS                 99
#define RX_SLEEP_T_MS           810

#endif

class rLevel1_t {
public:
    uint8_t Init();
    rPkt_t PktRx;
    // Inner use
    void TryToSleep(uint32_t SleepDuration);
    void TryToReceive(uint32_t RxDuration);
};

extern rLevel1_t Radio;
