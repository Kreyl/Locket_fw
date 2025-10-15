#pragma once

#include "radio_lvl1.h"
#include "app_types.h"

extern Locket lkt;
extern uint8_t tx_power;

namespace App {
extern Locket::Sta prev_state;

void TakeBatteryVoltage(uint32_t vbat);

void PresentSelf();

// Evt processing
void OnBtnEvt(BtnEvtInfo btn_info);
void OnSecondEvt();
void ApplyPill(int32_t pill_id);

// Radio
rPkt* PrepareTxPkt(); // Return null if no tx required
void ProcessRxTbl();

// App-specific commands parsing
void OnCmd(Shell *pshell);

} // namespace App