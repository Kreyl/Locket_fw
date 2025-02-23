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
union rPkt {
    uint32_t dw32[2];
    uint8_t bytes[8];
    struct {
        uint8_t id; // Required to distinct packets from same src.
        uint32_t transaction_id: 24;
        int16_t goodness;
        uint8_t green_evil : 1;
        uint8_t artifact: 1;
        uint8_t cyan_beast: 1;
        uint8_t searcher: 1;
        uint8_t : 4;
        int8_t rssi; // Will be set after RX. Transmitting is useless, but who cares.
    };
    void Reset(uint8_t aid) {
        dw32[0] = 0;
        dw32[1] = 0;
        id = aid;
    }
};
#pragma pack(pop)
#endif

inline constexpr const uint8_t kRPktSz = sizeof(rPkt);

#if 1 // =================== Channels, cycles, Rssi  ===========================
// Feel-Each-Other related
inline constexpr const uint32_t kCycleCnt = 4U, kSlotCnt = 54U;
inline constexpr const uint32_t kSlotDuration_ms = 3U; // for 500kBit/s
// inline constexpr const uint32_t kSlotDuration_ms = 5U; // for 100 & 2500kBit/s
inline constexpr const uint32_t kCycleDuration_ms = kSlotDuration_ms * kSlotCnt;
inline constexpr const uint32_t kMinSleepDuration_ms = 18UL;
// #define CHECK_RXTABLE_PERIOD_SC 4UL // Check RxTable every N SuperCycles
/*
 * CYCLE_DUR = SLOT_DUR(3ms) * SLOT_CNT(54) = 162ms
 * SUPERCYCLE_DUR = CYCLE_DUR * CYCLE_CNT(5) = 810ms
 * CHECK_PERIOD = SUPERCYCLE_DUR * CHECK_RXTABLE_PERIOD_SC(4) = 3240ms
 */
#endif

#if 1 // ============================= RX Table ================================
#define RXT_PKT_REQUIRED        TRUE
class RxTable {
public:
    static const uint32_t kSize = 36;
    uint32_t cnt = 0;
#if RXT_PKT_REQUIRED
    void AddOrReplaceExistingPkt(rPkt &apkt) {
        for(uint32_t i=0; i<cnt; i++) {
            if(ibuf[i].id == apkt.id) {
                ibuf[i] = apkt; // Replace with newer pkt
                return;
            }
        }
        ibuf[cnt] = apkt;
        if(cnt < (kSize-1)) cnt++;
    }

    // StatusOr<rPkt> GetPktByID(uint16_t id) {
    //     for(uint32_t i=0; i<cnt; i++) {
    //         if(ibuf[i].id == id) {
    //             return StatusOr<rPkt>(retv::Ok, ibuf[i]);
    //         }
    //     }
    //     return StatusOr<rPkt>(retv::Fail);
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
    rPkt& operator [](uint32_t indx) { return ibuf[indx]; }
#endif

    void Print() {
        Printf("RxTable cnt: %u\r", cnt);
        for(uint32_t i=0; i<cnt; i++) {
#if RXT_PKT_REQUIRED
            // Printf("ID: %u; type: %u\r", ibuf[i].id, ibuf[i].type);
#else
            Printf("ID: %u\r", IdBuf[i]);
#endif
        }
    }
private:
#if RXT_PKT_REQUIRED
    rPkt ibuf[kSize];
#else
    uint8_t IdBuf[RXTABLE_SZ];
#endif
};
#endif

namespace Radio {

retv Init();

extern rPkt pkt_tx;

} // namespace Radio

#endif //RADIO_LVL1_H__
