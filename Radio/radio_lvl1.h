#pragma once

#include "kl_lib.h"
#include "ch.h"
#include "cc1101.h"
#include "kl_buf.h"
#include "uart.h"
#include "MsgQ.h"

#if 1 // =========================== Pkt_t =====================================
struct rPkt_t {
    union {
        uint32_t DWord32;
        uint32_t ID;
    };
    uint32_t AppMode;
} __attribute__ ((__packed__));
#define RPKT_LEN    sizeof(rPkt_t)

#define THE_WORD        0xCA115EA1
#endif

// Message queue
#define R_MSGQ_LEN      4
#define R_MSG_SET_PWR   1
#define R_MSG_SET_CHNL  2
struct RMsg_t {
    uint8_t Cmd;
    uint8_t Value;
    RMsg_t() : Cmd(0), Value(0) {}
    RMsg_t(uint8_t ACmd, uint8_t AValue) : Cmd(ACmd), Value(AValue) {}
} __attribute__((packed));

#if 1 // =================== Channels, cycles, Rssi  ===========================
#define RCHNL_SERVICE   0
#define RCHNL_COMMON    1
#define RCHNL_EACH_OTH  7
#define RCHNL_MIN       10
#define RCHNL_MAX       30
#define ID2RCHNL(ID)    (RCHNL_MIN + ID)

#define RSSI_MIN        -81

// Feel-Each-Other related
#define CYCLE_CNT           4
#define SLOT_CNT            81
#define SLOT_DURATION_MS    5

// Timings
#define RX_T_MS                 180      // pkt duration at 10k is around 12 ms
#define RX_SLEEP_T_MS           810
#define MIN_SLEEP_DURATION_MS   18
#endif

#if 1 // ============================= RX Table ================================
#define RXTABLE_SZ              4
#define RXT_PKT_REQUIRED        FALSE
class RxTable_t {
private:
#if RXT_PKT_REQUIRED
    rPkt_t IBuf[RXTABLE_SZ];
#else
    uint8_t IdBuf[RXTABLE_SZ];
#endif
    uint32_t Cnt = 0;
public:
#if RXT_PKT_REQUIRED
    void AddOrReplaceExistingPkt(rPkt_t &APkt) {
        if(Cnt >= RXTABLE_SZ) return;   // Buffer is full, nothing to do here
        for(uint32_t i=0; i<Cnt; i++) {
            if(IBuf[i].ID == APkt.ID) {
                IBuf[i] = APkt; // Replace with newer pkt
                return;
            }
        }
        IBuf[Cnt] = APkt;
        Cnt++;
    }

    uint8_t GetPktByID(uint8_t ID, rPkt_t **ptr) {
        for(uint32_t i=0; i<Cnt; i++) {
            if(IBuf[i].ID == ID) {
                *ptr = &IBuf[i];
                return OK;
            }
        }
        return FAILURE;
    }

    bool IDPresents(uint8_t ID) {
        for(uint32_t i=0; i<Cnt; i++) {
            if(IBuf[i].ID == ID) return true;
        }
        return false;
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
    uint32_t GetCount() { return Cnt; }
    void Clear() { Cnt = 0; }

    void Print() {
        Printf("RxTable Cnt: %u\r", Cnt);
        for(uint32_t i=0; i<Cnt; i++) {
#if RXT_PKT_REQUIRED
            Printf("ID: %u; State: %u\r", IBuf[i].ID, IBuf[i].State);
#else
            Printf("ID: %u\r", IdBuf[i]);
#endif
        }
    }
};
#endif

class rLevel1_t {
public:
    EvtMsgQ_t<RMsg_t, R_MSGQ_LEN> RMsgQ;
    rPkt_t PktRx, PktTx;
//    bool MustTx = false;
    int8_t Rssi;
    RxTable_t RxTable;
    uint8_t Init();
    // Inner use
    void TryToSleep(uint32_t SleepDuration);
    void TryToReceive(uint32_t TotalDuration_ms);
    // Different modes of operation
    void TaskTransmitter();
    void TaskReceiverManyByID();
    void TaskReceiverManyByChannel();
    void TaskReceiverSingle();
    void TaskFeelEachOtherSingle();
    void TaskFeelEachOtherMany();
};

extern rLevel1_t Radio;
