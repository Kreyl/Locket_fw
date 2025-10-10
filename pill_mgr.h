/*
 * pill.h
 *
 *  Created on: Apr 17, 2013
 *      Author: g.kruglov
 */

#ifndef PILL_MGR_H__
#define PILL_MGR_H__

#include "uart.h"
#include "ch.h"
#include "kl_lib.h"


namespace PillMgr {

inline constexpr const uint32_t kCheckPeriod_ms = 540;
inline constexpr const uint8_t  kPillI2CAddr = 0x50;
inline constexpr const uint32_t kPillPageSize = 8; // IC dependant, see datasheet.

// Data related
inline constexpr const uint32_t kPillDataAddr = 0x00;

struct PillData {
    uint32_t type;
};
inline constexpr const uint32_t kPillDataSz = sizeof(PillData);
inline constexpr const uint32_t kPillDataSz32 = kPillDataSz / 4UL;
extern PillData pill_data;

void Init();
void Check();

retv WritePill(PillData &pill_data);
retv Read32(uint32_t addr, uint32_t *pdata, uint32_t len);
retv Write32(uint32_t addr, uint32_t *pdata, uint32_t len);

} // namespace PillMgr

#endif //PILL_MGR_H__