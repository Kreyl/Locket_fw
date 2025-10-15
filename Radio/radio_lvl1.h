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
#include <array>


#if 1 // ============================= RX Table ================================
class RxTable {
private:
    struct Item {
        rPkt pkt;
        int32_t counter = 0;
    };
    static constexpr const uint32_t kSize = 256; // To index with 8-bit ids
    std::array<Item, kSize> iarr;
    // Iterator implementation
    class iterator {
        const RxTable* table;
        size_t indx;
        void AdvanceToNextValid() {
            while(indx < RxTable::kSize and table->iarr[indx].counter <= 0) ++indx;
        }
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = rPkt;
        using difference_type = std::ptrdiff_t;
        using pointer = const rPkt*;
        using reference = const rPkt&;
        iterator(const RxTable* t, size_t i) : table(t), indx(i) { AdvanceToNextValid(); }
        reference operator*() { return table->iarr[indx].pkt; }
        pointer operator->()  { return &table->iarr[indx].pkt; }
        iterator& operator++() {
            ++indx;
            AdvanceToNextValid();
            return *this;
        }
        bool operator==(const iterator& other) const { return indx == other.indx; }
        bool operator!=(const iterator& other) const { return !(*this == other);  }
    };
public:
    static constexpr const int32_t kTimeout_tics = 4L;
    void AddPkt(rPkt &apkt) {
        iarr[apkt.id].pkt = apkt;
        iarr[apkt.id].counter = kTimeout_tics;
    }
    void Tick() {
        for(auto &itm : iarr) { if(itm.counter > 0) itm.counter--; }
    }

    rPkt& operator [](uint32_t indx) { return iarr[indx].pkt; }
    // Range-based for support
    iterator begin() const { return iterator(this, 0); }
    iterator end()   const { return iterator(this, kSize); }
};
#endif

    // void Print() {
    //     Printf("RxTable cnt: %u\r", cnt);
    //     for(uint32_t i=0; i<cnt; i++) {
    //         // Printf("ID: %u; type: %u\r", ibuf[i].id, ibuf[i].type);
    //         // Printf("ID: %u\r", IdBuf[i]);
    //     }
    // }

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
// inline constexpr const uint32_t kCheckRxTablePeriod_sc = 4UL; // Check RxTable every N SuperCycles
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
extern RxTable rx_table;

} // namespace Radio
