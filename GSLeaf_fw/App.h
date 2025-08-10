#pragma once

#include "radio_lvl1.h"

enum class DevType {
    // Locket
    Idle = 0,
    Opener = 1,
    Restorer = 2,
    Closer = 3,
    // Point
    Active = 10,
    Opened = 11,
    Closed = 12,
};

class Config {
public:
    uint32_t id = 0;
    DevType type = DevType::Idle;
    bool is_master = false;
    uint8_t tx_power = 0;
    uint32_t quorum_sz = 3UL;
    void PrintType() const;
    void PrintTxPwr() const;
};

extern Config cfg;

namespace App {

void TakeBatteryVoltage(uint32_t vbat);

// Evt processing
// void OnSecondEvt();

// Radio
rPkt* PrepareTxPkt(); // Return null if no tx required
void ProcessRxTbl(RxTable &tbl);

// App-specific commands parsing
void OnCmd(Shell *pshell);

} // namespace App