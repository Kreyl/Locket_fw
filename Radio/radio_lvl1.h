/*
 * radio_lvl1.h
 *
 *  Created on: Nov 17, 2013
 *      Author: kreyl
 */

#pragma once

#include "app_types.h"
#include "cc1101defins.h"

namespace Radio {

#pragma region // ==== Constants ====
/* Measured durations for 8 bytes pkt (recalibrate + transmit):
500k => 1.8mS; 250k => 2.3mS; 100k => 3.8mS; 38k4 => 8mS; 10k => 27mS; 2k4 => 109mS
Duration of 9-byte pkt measured using system time (10k ticks); recalibration once per supercycle:
500k => 0.8ms; 250k=> 1.3ms; 100k => 2.8ms; 10k => 26ms
*/
inline constexpr const uint32_t kCycleCnt = 4UL;
inline constexpr const int32_t kSlotCnt = 90UL;
inline constexpr const int32_t kSlotDiffMin = (72UL * kSlotCnt) / 99UL; // Next slot = curr_slot + diff; diff = random(kSlotDiffMin, kSlotCnt-1). 72/99 is a good count.
inline constexpr const uint32_t kSlotDuration_ms = 3UL; // for 9 bytes @ 100kBit/s, recalibration once per supercycle
inline constexpr const uint32_t kCycleDuration_ms = kSlotDuration_ms * kSlotCnt;
inline constexpr const uint32_t kMinSleepDuration_ms = 18UL; // For shorter delays, IDLE mode of CC used

// Just for reference, not used anywhere
inline constexpr const uint32_t kSuperCycleDuration_ms = kCycleDuration_ms * kCycleCnt;
/* Examples:
CYCLE_DUR = kSlotDuration_ms(4ms) * kSlotCnt(63) = 252ms
SUPERCYCLE_DUR = CYCLE_DUR * kCycleCnt(4) = 1008ms
CHECK_PERIOD = SUPERCYCLE_DUR * kCheckRxTablePeriod_sc(4) = 4032ms

CYCLE_DUR = kSlotDuration_ms(2ms) * kSlotCnt(99) = 198ms
SUPERCYCLE_DUR = CYCLE_DUR * kCycleCnt(4) = 792ms
CHECK_PERIOD = SUPERCYCLE_DUR * kCheckRxTablePeriod_sc(4) = 3960ms
*/
#pragma endregion

retv Init();

} // namespace Radio
