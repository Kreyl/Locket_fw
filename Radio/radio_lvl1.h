/*
 * radio_lvl1.h
 *
 *  Created on: Nov 17, 2013
 *      Author: kreyl
 */

#ifndef RADIO_LVL1_H__
#define RADIO_LVL1_H__

#include "kl_lib.h"
#include "ch.h"
#include "cc1101.h"
#include "kl_buf.h"
#include "uart.h"
#include "MsgQ.h"

#if 1 // =========================== Pkt_t =====================================
union rPkt_t {
    uint32_t DW32[2];
    struct {
        uint8_t ID; // Required to distinct packets from same src
        uint8_t TimeSrc;
        uint8_t HopCnt;
        uint16_t iTime;
        // Payload
        int8_t Rssi; // Will be set after RX. Transmitting is useless, but who cares.
        uint16_t Salt;
    } __attribute__((__packed__));
    rPkt_t& operator = (const rPkt_t &Right) {
        DW32[0] = Right.DW32[0];
        DW32[1] = Right.DW32[1];
        return *this;
    }
} __attribute__ ((__packed__));
#endif

#define RPKT_LEN    sizeof(rPkt_t)
#define RPKT_SALT   0xCA11

#if 1 // =================== Channels, cycles, Rssi  ===========================
#define RCHNL_EACH_OTH          2

// Feel-Each-Other related
#define CYCLE_CNT               5U
#define SLOT_CNT                36U
#define SLOT_DURATION_MS        4U
#define MIN_SLEEP_DURATION_MS   18UL
#define CHECK_RXTABLE_PERIOD_SC 3UL // Check RxTable every 3 SuperCycle

#endif

#define RXTABLE_SZ          4UL

class rLevel1_t {
private:
    void TryToReceive(uint32_t RxDuration);
    void TryToSleep(uint32_t SleepDuration);
    void AddPktToRxTableI(rPkt_t *pPkt) { IdBuf.Add(pPkt->ID); }
public:
    CountingBuf_t<uint8_t, RXTABLE_SZ> IdBuf;
    uint8_t Init();
    // Inner use
    void TaskFeelEachOther();
};

extern rLevel1_t Radio;

#endif //RADIO_LVL1_H__
