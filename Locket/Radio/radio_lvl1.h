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
struct rPkt {
    uint32_t id; // Required to distinct packets from same src.
    union {
        uint32_t dw32;
        struct {
            uint8_t IsImmortal;
            uint16_t silt;
            int8_t rssi; // Will be set after RX. Transmitting is useless, but who cares.
        };
    };
    void Reset(uint32_t aid) {
        id = aid;
        dw32 = 0;
    }
    rPkt& operator = (const rPkt &right) {
        id = right.id;
        dw32 = right.dw32;
        return *this;
    }
    void Print() {
        Printf("id: %X; Immortal=%d rssi=%d\r", id, IsImmortal, rssi);
    }
};
#pragma pack(pop)
#endif

inline constexpr const uint8_t kRPktSz = sizeof(rPkt);


#if 1 // ============================= RX Table ================================
#define RXT_PKT_REQUIRED        TRUE
class RxTable {
public:
    static const uint32_t kSize = 54;
    uint32_t cnt = 0;
#if RXT_PKT_REQUIRED
    void AddOrReplaceExistingPkt(rPkt &apkt) {
        rPkt *ppkt = &ibuf[0], *pend = &ibuf[cnt];
        while(ppkt < pend) {
            if(ppkt->id == apkt.id) {
                *ppkt = apkt; // Replace with newer pkt
                return;
            }
            ppkt++;
        }
        // Empty or not found
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

#pragma region // ==== Constants ====
inline constexpr const uint32_t kCycleCnt = 4U, kSlotCnt = 72U;
inline constexpr const uint32_t kSlotDuration_ms = 3U; // for 500kBit/s
// inline constexpr const uint32_t kSlotDuration_ms = 5U; // for 100 & 250kBit/s
inline constexpr const uint32_t kCycleDuration_ms = kSlotDuration_ms * kSlotCnt;
// inline constexpr const uint32_t kSuperCycleDuration_ms = kCycleDuration_ms * kCycleCnt;
inline constexpr const uint32_t kMinSleepDuration_ms = 18UL;
inline constexpr const uint32_t kCheckRxTablePeriod_sc = 4UL; // Check RxTable every N SuperCycles
/* Example:
* CYCLE_DUR = kSlotDuration_ms(3ms) * kSlotCnt(72) = 216ms
* SUPERCYCLE_DUR = CYCLE_DUR * kCycleCnt(4) = 864ms
* CHECK_PERIOD = SUPERCYCLE_DUR * kCheckRxTablePeriod_sc(4) = 3456ms
*/
#pragma endregion

retv Init();

} // namespace Radio

#endif //RADIO_LVL1_H__
