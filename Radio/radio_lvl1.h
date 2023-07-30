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
#define RCYCLE_CNT              5U
#define RSLOT_CNT               54U
#define SCYCLES_TO_KEEP_TIMESRC 4U  // After that amount of supercycles, TimeSrcID become self ID

// Timings: based on (27MHz/192) clock of CC, divided by 4 with prescaler
#define RTIM_PRESCALER          2U  // From excel
#define TIMESLOT_DUR_TICKS      108U // From excel
#define CYCLE_DUR_TICKS         (TIMESLOT_DUR_TICKS * RSLOT_CNT)
#define SUPERCYCLE_DUR_TICKS    (CYCLE_DUR_TICKS * RCYCLE_CNT)
#if SUPERCYCLE_DUR_TICKS >= 0xFFFF
#error "Too long supercycle"
#endif
#define ZERO_ID_INCREMENT       3U // [0;100) -> [3;103) to process start of zero cycle calibration delay
#define HOPS_CNT_MAX            4U // do not adjust time if too many hops. Required to avoid eternal loops adjustment.
// Experimental values
#define TX_DUR_TICS             53U // Experimental value to sync time scale

#endif

#if 1 // ============================= RX Table ================================
struct rPayload_t {
    int32_t Cnt = 0; // Count of packets with this ID
    int32_t Rssi;
} __attribute__ ((__packed__));

#define RXTABLE_SZ              RSLOT_CNT

// RxTable must be cleared during processing
class RxTable_t {
private:
    rPayload_t IBuf[RXTABLE_SZ];
public:
    void AddPktI(rPkt_t *pPkt) {
        if(pPkt->ID < RXTABLE_SZ) {
            IBuf[pPkt->ID].Rssi += pPkt->Rssi;
            IBuf[pPkt->ID].Cnt++;
        }
    }

    void CleanUp() {
        for(auto &v : IBuf) {
            v.Cnt = 0;
            v.Rssi = 0;
        }
    }

    rPayload_t& operator[](const int32_t Indx) {
        return IBuf[Indx];
    }

    void Print() {
        Printf("RxTable\r");
        for(uint32_t i=0; i<RXTABLE_SZ; i++) {
            if(IBuf[i].Cnt) Printf("ID: %u; Cnt: %u; Rssi: %d\r", i, IBuf[i].Cnt, IBuf[i].Rssi);
        }
    }
};
#endif

class rLevel1_t {
private:
    RxTable_t RxTable1, RxTable2, *RxTableW = &RxTable1;
public:
    uint8_t TxPower;

    void AddPktToRxTableI(rPkt_t *pPkt) { RxTableW->AddPktI(pPkt); }

    RxTable_t& GetRxTable() {
        chSysLock();
        RxTable_t* RxTableR;
        // Switch tables
        if(RxTableW == &RxTable1) {
            RxTableW = &RxTable2;
            RxTableR = &RxTable1;
        }
        else {
            RxTableW = &RxTable1;
            RxTableR = &RxTable2;
        }
        chSysUnlock();
        return *RxTableR;
    }

    uint8_t Init();

    // Inner use
    void ITask();
};

extern rLevel1_t Radio;

#endif //RADIO_LVL1_H__
