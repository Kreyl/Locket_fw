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
#include "MsgQ.h"

#if 0 // ========================= Signal levels ===============================
// Python translation for db
#define RX_LVL_TOP      1000
// Jolaf: str(tuple(1 + int(sqrt(float(i) / 65) * 99) for i in xrange(0, 65 + 1)))
//const int32_t dBm2Percent1000Tbl[66] = {10, 130, 180, 220, 250, 280, 310, 330, 350, 370, 390, 410, 430, 450, 460, 480, 500, 510, 530, 540, 550, 570, 580, 590, 610, 620, 630, 640, 650, 670, 680, 690, 700, 710, 720, 730, 740, 750, 760, 770, 780, 790, 800, 810, 820, 830, 840, 850, 860, 860, 870, 880, 890, 900, 910, 920, 920, 930, 940, 950, 960, 960, 970, 980, 990, 1000};
const int32_t dBm2Percent1000Tbl[86] = {
         10, 117, 162, 196, 225, 250, 273, 294, 314, 332,
        350, 366, 382, 397, 412, 426, 440, 453, 466, 478,
        490, 502, 514, 525, 536, 547, 558, 568, 578, 588,
        598, 608, 617, 627, 636, 645, 654, 663, 672, 681,
        689, 698, 706, 714, 722, 730, 738, 746, 754, 762,
        769, 777, 784, 792, 799, 806, 814, 821, 828, 835,
        842, 849, 856, 862, 869, 876, 882, 889, 895, 902,
        908, 915, 921, 927, 934, 940, 946, 952, 958, 964,
        970, 976, 982, 988, 994, 1000
};

static inline int32_t dBm2Percent(int32_t Rssi) {
    if(Rssi < -100) Rssi = -100;
    else if(Rssi > -15) Rssi = -15;
    Rssi += 100;    // 0...85
    return dBm2Percent1000Tbl[Rssi];
}

// Conversion Lvl1000 <=> Lvl250
#define Lvl1000ToLvl250(Lvl1000) ((uint8_t)((Lvl1000 + 3) / 4))

static inline void Lvl250ToLvl1000(uint16_t *PLvl) {
    *PLvl = (*PLvl) * 4;
}

// Sensitivity Constants, percent [1...1000]. Feel if RxLevel > SnsConst.
#define RLVL_NEVER              10000
#define RLVL_2M                 800     // 0...4m
#define RLVL_4M                 700     // 1...20m
#define RLVL_10M                600
#define RLVL_50M                1

#endif

__unused
static const uint8_t PwrTable[12] = {
        CC_PwrMinus30dBm, // 0
        CC_PwrMinus27dBm, // 1
        CC_PwrMinus25dBm, // 2
        CC_PwrMinus20dBm, // 3
        CC_PwrMinus15dBm, // 4
        CC_PwrMinus10dBm, // 5
        CC_PwrMinus6dBm,  // 6
        CC_Pwr0dBm,       // 7
        CC_PwrPlus5dBm,   // 8
        CC_PwrPlus7dBm,   // 9
        CC_PwrPlus10dBm,  // 10
        CC_PwrPlus12dBm   // 11
};

#if 1 // =========================== Pkt_t =====================================
union rPkt_t {
    uint32_t DW32;
    struct {
        uint16_t ID : 6;
        uint16_t Cycle : 4;
        uint16_t TimeSrcID : 6;
        // Payload
        uint16_t Type : 4;
        uint16_t RCmd : 4;
        int8_t Rssi; // Will be set after RX. Trnasmitting is useless, but who cares.
    } __attribute__((__packed__));
    rPkt_t& operator = (const rPkt_t &Right) {
        DW32 = Right.DW32;
        return *this;
    }
} __attribute__ ((__packed__));
#endif

#define RPKT_LEN    sizeof(rPkt_t)

#if 1 // =================== Channels, cycles, Rssi  ===========================
#define RCHNL_EACH_OTH  7
#define RCHNL_FAR       0

#define TX_PWR_FAR      CC_PwrPlus10dBm

// Feel-Each-Other related
#define RCYCLE_CNT              5
#define FAR_CYCLE_INDX          (RCYCLE_CNT - 1)
#define FAR_CYCLE_DURATION_MS   42
#define RSLOT_CNT               50
#define RSLOT_DURATION_ST       36
#define CYCLE_DURATION_ST       (RSLOT_DURATION_ST * RSLOT_CNT)
//#define MAX_RANDOM_DURATION_MS  18


#define SCYCLES_TO_KEEP_TIMESRC 4   // After that amount of supercycles, TimeSrcID become self ID

#endif

#if 1 // ============================= RX Table ================================
#define RXTABLE_SZ              50
#define RXT_PKT_REQUIRED        TRUE
class RxTable_t {
private:
#if RXT_PKT_REQUIRED
    rPkt_t IBuf[RXTABLE_SZ];
#else
    uint8_t IdBuf[RXTABLE_SZ];
#endif
public:
    uint32_t Cnt = 0;
#if RXT_PKT_REQUIRED
    void AddOrReplaceExistingPkt(rPkt_t &APkt) {
        chSysLock();
        for(uint32_t i=0; i<Cnt; i++) {
            if(IBuf[i].ID == APkt.ID) {
                if(IBuf[i].Rssi < APkt.Rssi) IBuf[i] = APkt; // Replace with newer pkt if RSSI is stronger
                chSysUnlock();
                return;
            }
        }
        // Same ID not found
        if(Cnt < RXTABLE_SZ) {
            IBuf[Cnt] = APkt;
            Cnt++;
        }
        chSysUnlock();
    }

    uint8_t GetPktByID(uint8_t ID, rPkt_t *ptr) {
        for(uint32_t i=0; i<Cnt; i++) {
            if(IBuf[i].ID == ID) {
                *ptr = IBuf[i];
                return retvOk;
            }
        }
        return retvFail;
    }

    bool IDPresents(uint8_t ID) {
        for(uint32_t i=0; i<Cnt; i++) {
            if(IBuf[i].ID == ID) return true;
        }
        return false;
    }

    rPkt_t& operator[](const int32_t Indx) {
        return IBuf[Indx];
    }
#else
    void AddId(uint8_t ID) {
        if(Cnt >= RXTABLE_SZ) return;   // Buffer is full, nothing to do here
        for(uint32_t i=0; i<Cnt; i++) {
            if(IdBuf[i] == ID) return;
        }
        IdBuf[Cnt] = ID;
        Cnt++;
    }

#endif

    void Print() {
        Printf("RxTable Cnt: %u\r", Cnt);
        for(uint32_t i=0; i<Cnt; i++) {
#if RXT_PKT_REQUIRED
//            Printf("ID: %u; State: %u\r", IBuf[i].ID, IBuf[i].State);
#else
            Printf("ID: %u\r", IdBuf[i]);
#endif
        }
    }
};
#endif

// Message queue
#define R_MSGQ_LEN      9
enum RmsgId_t { rmsgEachOthRx, rmsgEachOthTx, rmsgEachOthSleep, rmsgPktRx, rmsgFar };
struct RMsg_t {
    RmsgId_t Cmd;
    uint8_t Value;
    RMsg_t() : Cmd(rmsgEachOthSleep), Value(0) {}
    RMsg_t(RmsgId_t ACmd) : Cmd(ACmd), Value(0) {}
    RMsg_t(RmsgId_t ACmd, uint8_t AValue) : Cmd(ACmd), Value(AValue) {}
} __attribute__((packed));

class rLevel1_t {
private:
    RxTable_t RxTable1, RxTable2, *RxTableW = &RxTable1;
    int32_t CntTxFar = 0;
    // Different modes of operation
    void TaskFeelFar();
public:
    rPkt_t PktRx, PktTx, PktTxFar;
    EvtMsgQ_t<RMsg_t, R_MSGQ_LEN> RMsgQ;
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
        RxTableW->Cnt = 0; // Clear it
        chSysUnlock();
        return *RxTableR;
    }
    uint8_t Init();
    void DoTransmitFar(uint32_t TxCnt)  { CntTxFar = TxCnt; }
    // Inner use
    void ITask();
};

extern rLevel1_t Radio;
