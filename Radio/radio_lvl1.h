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
#include "main.h"

#if 1 // =========================== Pkt_t =====================================
union rPkt_t  {
    uint32_t IDWord;
    struct {
        uint8_t dummy;
        uint8_t ID;
        uint8_t Type;
        int8_t Rssi; // Put here received rssi value
    } __packed;
    rPkt_t& operator = (const rPkt_t &Right) {
        IDWord = Right.IDWord;
        return *this;
    }
    void Print() { Printf("ID: %u; Type: %u\r", ID, Type); }
} __packed;

#define RPKT_LEN    sizeof(rPkt_t)
#endif

#if 1 // =================== Channels, cycles, Rssi  ===========================
#define RCHNL               7
#define RCYCLE_CNT          4
#define TIMESLOT_CNT        99

#define RSSI_LOWEST         -90
#define RSSI_NEAR           -63
#endif

#if 1 // =========================== Timings ===================================
#define TIMESLOT_DURATION_ST  18
#endif

#if 1 // ============================= RX Table ================================
#define RXTABLE_SZ              10
class RxTable_t {
private:
    uint32_t Cnt = 0;
public:
    rPkt_t Buf[RXTABLE_SZ];
    void AddOrReplaceExistingPkt(rPkt_t &APkt) {
        for(uint32_t i=0; i<Cnt; i++) {
            if(Buf[i].ID == APkt.ID) {
                Buf[i] = APkt; // Replace with newer pkt
                return;
            }
        }
        if(Cnt >= RXTABLE_SZ) return;   // Buffer is full, nothing to do here
        Buf[Cnt] = APkt;
        Cnt++;
    }

    uint8_t GetPktByID(uint8_t ID, rPkt_t **ptr) {
        for(uint32_t i=0; i<Cnt; i++) {
            if(Buf[i].ID == ID) {
                *ptr = &Buf[i];
                return retvOk;
            }
        }
        return retvFail;
    }

    bool IDPresents(uint8_t ID) {
        for(uint32_t i=0; i<Cnt; i++) {
            if(Buf[i].ID == ID) return true;
        }
        return false;
    }

    uint32_t GetCount() { return Cnt; }
    void Clear() { Cnt = 0; }

    void Print() {
        Printf("RxTable Cnt: %u\r", Cnt);
        for(uint32_t i=0; i<Cnt; i++) Buf[i].Print();
    }
};
#endif

// Message queue
#define R_MSGQ_LEN      9
enum RmsgId_t { rmsgSetPwr, rmsgSetChnl, rmsgTimeToRx, rmsgTimeToTx, rmsgTimeToSleep, rmsgPktRx };
struct RMsg_t {
    RmsgId_t Cmd;
    uint8_t Value;
    RMsg_t() : Cmd(rmsgSetPwr), Value(0) {}
    RMsg_t(RmsgId_t ACmd) : Cmd(ACmd), Value(0) {}
    RMsg_t(RmsgId_t ACmd, uint8_t AValue) : Cmd(ACmd), Value(AValue) {}
} __attribute__((packed));


class rLevel1_t {
public:
    int8_t Rssi;
    rPkt_t PktTx, PktRx;
    EvtMsgQ_t<RMsg_t, R_MSGQ_LEN> RMsgQ;
    RxTable_t RxTable;
    uint8_t Init();
    // Inner use
    void ITask();
};

extern rLevel1_t Radio;
