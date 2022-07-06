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
    uint32_t DW32[2];
    struct {
        uint8_t ID; // Required to distinct packets from same src
        uint8_t TimeSrcID;
        uint16_t iTime;
        // Payload
        uint8_t Type;
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
#define RPKT_SALT   0xCA11U

#if 1 // =================== Channels, cycles, Rssi  ===========================
#define RCHNL_EACH_OTH  0

// Feel-Each-Other related
#define RCYCLE_CNT              5U
#define RSLOT_CNT               4U
#define SCYCLES_TO_KEEP_TIMESRC 4   // After that amount of supercycles, TimeSrcID become self ID

// Timings: based on (27MHz/192) clock of CC, divided by 4 with prescaler
#define RTIM_PRESCALER          0U
#define TIMESLOT_DUR_TICKS      88U
#define CYCLE_DUR_TICKS         (TIMESLOT_DUR_TICKS * RSLOT_CNT)
#define SUPERCYCLE_DUR_TICKS    (CYCLE_DUR_TICKS * RCYCLE_CNT)

// XXX
#define ADJUST_DELAY_TICS       72

#endif

#if 1 // ============================= RX Table ================================
#define RXTABLE_SZ              RSLOT_CNT
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
            Printf("ID: %u; Type: %u\r", IBuf[i].ID, IBuf[i].Type);
#else
            Printf("ID: %u\r", IdBuf[i]);
#endif
        }
    }
};
#endif

// Message queue
#define R_MSGQ_LEN      9
enum RmsgId_t { rmsgEachOthRx, rmsgEachOthTx, rmsgEachOthSleep, rmsgPktRx };
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
public:
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
    // Inner use
    void ITask();
};

extern rLevel1_t Radio;
