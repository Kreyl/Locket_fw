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
#include "types.h"

#if 1 // =========================== Pkt_t =====================================
#pragma pack(push, 1)
struct rPkt_t {
    uint32_t salt = 0xCa110fEa;
    uint16_t id; // Required to distinct packets from same src
    uint8_t type;
    int8_t rssi; // Will be set after RX. Transmitting is useless, but who cares.
};
#pragma pack(pop)
#endif

#define RPKT_LEN    sizeof(rPkt_t)


#if 1 // =================== Channels, cycles, Rssi  ===========================
#define RCHNL_EACH_OTH          0

// Feel-Each-Other related
#define CYCLE_CNT               4U
#define SLOT_CNT                54U
// #define SLOT_DURATION_MS        3U // for 500kBit/s
// #define SLOT_DURATION_MS        5U // for 250kBit/s
#define SLOT_DURATION_MS        5U // for 100kBit/s
#define MIN_SLEEP_DURATION_MS   18UL
#define CHECK_RXTABLE_PERIOD_SC 4UL // Check RxTable every N SuperCycles
/*
 * CYCLE_DUR = SLOT_DUR(3ms) * SLOT_CNT(54) = 162ms
 * SUPERCYCLE_DUR = CYCLE_DUR * CYCLE_CNT(5) = 810ms
 * CHECK_PERIOD = SUPERCYCLE_DUR * CHECK_RXTABLE_PERIOD_SC(4) = 3240ms
 */
#endif

#if 1 // ============================= RX Table ================================
#define RXTABLE_SZ              50U // 50 devices total
#define RXT_PKT_REQUIRED        TRUE
class RxTable {
private:
#if RXT_PKT_REQUIRED
    rPkt_t ibuf[RXTABLE_SZ];
#else
    uint8_t IdBuf[RXTABLE_SZ];
#endif
public:
    uint32_t cnt = 0;
#if RXT_PKT_REQUIRED
    void AddOrReplaceExistingPkt(rPkt_t &apkt) {
        for(uint32_t i=0; i<cnt; i++) {
            if(ibuf[i].id == apkt.id) {
                ibuf[i] = apkt; // Replace with newer pkt
                return;
            }
        }
        ibuf[cnt] = apkt;
        if(cnt < (RXTABLE_SZ-1)) cnt++;
    }

    // StatusOr<rPkt_t> GetPktByID(uint16_t id) {
    //     for(uint32_t i=0; i<cnt; i++) {
    //         if(ibuf[i].id == id) {
    //             return StatusOr<rPkt_t>(retv::Ok, ibuf[i]);
    //         }
    //     }
    //     return StatusOr<rPkt_t>(retv::Fail);
    // }

    bool IDPresents(uint16_t id) {
        for(uint32_t i=0; i<cnt; i++) {
            if(ibuf[i].id == id) return true;
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
    void Clear() { cnt = 0; }

#if RXT_PKT_REQUIRED
    rPkt_t& operator [](uint32_t indx) { return ibuf[indx]; }
#endif

    void Print() {
        Printf("RxTable cnt: %u\r", cnt);
        for(uint32_t i=0; i<cnt; i++) {
#if RXT_PKT_REQUIRED
            Printf("ID: %u; type: %u\r", ibuf[i].id, ibuf[i].type);
#else
            Printf("ID: %u\r", IdBuf[i]);
#endif
        }
    }
};
#endif

retv RadioInit();

#endif //RADIO_LVL1_H__
