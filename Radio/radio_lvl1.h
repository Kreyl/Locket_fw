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
#include "types.h"
#include "app_types.h"

enum class WormholeCmd : uint8_t { None=0, KillThemAll=4, VoteAccepted=18 };

#pragma region // =========================== Radio Packet ===============================
#pragma pack(push, 1)
inline constexpr const int32_t kRpktLktsCnt = 10;
union rPkt {
    uint32_t dw32[3];
    struct {
        uint8_t id;  // 1 byte
        union {
            struct { // 4 bytes
                uint8_t state;
                uint8_t btnA_pressed, btn_middle_pressed, btnB_pressed;
            } locket;
            struct { // 11 bytes
                WormholeCmd cmd;
                uint8_t ids[kRpktLktsCnt];
            } wormhole;
            struct { // 1 byte
                uint8_t value;
            } mengir;
        };
    };
    rPkt& operator = (const rPkt &right) {
        dw32[0] = right.dw32[0];
        dw32[1] = right.dw32[1];
        dw32[2] = right.dw32[2];
        return *this;
    }
    void PrintLocket(const char* S) {
        Printf("%Sid=%u sta=%u; %u %u %u\r", S, id, locket.state, locket.btnA_pressed, locket.btn_middle_pressed, locket.btnB_pressed);
    }
    void PrintWormhole(const char* S) {
        Printf("%Sid=%u cmd=%u L:", S, id, wormhole.cmd);
        for(uint32_t i=0; i<kRpktLktsCnt; i++) Printf(" %u", wormhole.ids[i]);
        PrintfEOL();
    }
    void PrintMengir(const char* S) {
        Printf("%Sid=%u value=%u\r", S, id, mengir.value);
    }

    DevType GetType() {
        if     (id >= IDs::HostMin and id <=IDs::HostMax) return DevType::Host;
        else if(id >= IDs::WormholeMin and id <= IDs::WormholeMax) return DevType::Wormhole;
        else if(id >= IDs::MengirMin and id <= IDs::MengirMax) return DevType::Mengir;
        else if(id >= IDs::LocketMin and id <= IDs::LocketMax) return DevType::Locket;
        else return DevType::None;
    }

};
#pragma pack(pop)
inline constexpr const uint8_t kRPktSz = sizeof(rPkt);
#pragma endregion


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

// #define RADAPTIVE_CYCLE_CNT     TRUE

#pragma region // ==== Constants ====
/* Measured durations for 8 bytes pkt (recalibrate + transmit):
500k => 1.8mS; 250k => 2.3mS; 100k => 3.8mS; 38k4 => 8mS; 10k => 27mS; 2k4 => 109mS
*/
inline constexpr const uint32_t kCycleCnt = 4UL;
inline constexpr const uint32_t kSlotCnt = 108UL;
// inline constexpr const uint32_t kSlotDuration_ms = 4UL; // for 100kBit/s
inline constexpr const uint32_t kSlotDuration_ms = 2UL; // for 12 bytes @ 500kBit/s
inline constexpr const uint32_t kCycleDuration_ms = kSlotDuration_ms * kSlotCnt;
// inline constexpr const uint32_t kSuperCycleDuration_ms = kCycleDuration_ms * kCycleCnt;
inline constexpr const uint32_t kMinSleepDuration_ms = 18UL;
inline constexpr const uint32_t kCheckRxTablePeriod_sc = 4UL; // Check RxTable every N SuperCycles
/* Examples:
CYCLE_DUR = kSlotDuration_ms(4ms) * kSlotCnt(63) = 252ms
SUPERCYCLE_DUR = CYCLE_DUR * kCycleCnt(4) = 1008ms
CHECK_PERIOD = SUPERCYCLE_DUR * kCheckRxTablePeriod_sc(4) = 4032ms

CYCLE_DUR = kSlotDuration_ms(2ms) * kSlotCnt(99) = 198ms
SUPERCYCLE_DUR = CYCLE_DUR * kCycleCnt(5) = 990ms
CHECK_PERIOD = SUPERCYCLE_DUR * kCheckRxTablePeriod_sc(4) = 3960ms
*/
#pragma endregion

retv Init();

} // namespace Radio
