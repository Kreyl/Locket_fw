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
    uint32_t dw32[2];
    struct {
        uint32_t id;
        int8_t Rssi; // Will be set after RX. Transmitting is useless, but who cares.
    } __attribute__((__packed__));
    rPkt_t& operator = (const rPkt_t &Right) {
        dw32[0] = Right.dw32[0];
        dw32[1] = Right.dw32[1];
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

#if 0 // ============================= RX Table ================================
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
            if((IBuf[i].ID == APkt.ID) and (IBuf[i].RCmd == APkt.RCmd)) {
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
public:
    rPkt_t PktRx, PktTx;
    EvtMsgQ_t<RMsg_t, R_MSGQ_LEN> RMsgQ;
    uint8_t Init();
    // Inner use
    void ITask();
};

extern rLevel1_t Radio;

#endif //RADIO_LVL1_H__