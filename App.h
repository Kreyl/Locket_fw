#pragma once

#include "radio_lvl1.h"

class Config {
public:
    uint32_t id = 0;
    uint32_t level = 1;
    bool is_alive = true;
    uint8_t tx_power = 0;
    void PrintTxPwr() const;
};
extern Config cfg;

namespace App {

void TakeBatteryVoltage(uint32_t vbat);

void ShowSelfState();

// Evt processing
void OnBtnEvt(BtnEvtInfo btn_info);
void OnSecondEvt();

// Radio
rPkt* PrepareTxPkt(); // Return null if no tx required
void ProcessRxTbl(RxTable &tbl);

// App-specific commands parsing
void OnCmd(Shell *pshell);

} // namespace App