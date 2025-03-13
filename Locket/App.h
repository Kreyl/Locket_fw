#pragma once

#include "radio_lvl1.h"


enum class DevType {
    Immortal   = 1,
    Preimmortal = 0,
};

class Config {
public:
    uint32_t id = 0;
    DevType type = DevType::Immortal;
    uint8_t tx_power = 0;
    // Brightness
    static const uint8_t kBrtCnt = 4;
    static constexpr uint8_t kBrtTable[kBrtCnt] = { 4, 37, 115, 255 };
    uint8_t brt_indx = kBrtCnt - 1;
    // Vibro
    static const int32_t kNoVibroTime_s = 20 * 60; // 20 minutes
    int32_t novibro_time_left_s = 0; // Vibro enabled
    bool VibroEnabled() { return novibro_time_left_s == 0; }
    void DisableVibro() { novibro_time_left_s = kNoVibroTime_s; }
    void EnableVibro()  { novibro_time_left_s = 0; }
    void PrintTxPwr();
    void PrintType() {
        if(type == DevType::Immortal) Printf("Immortal\r");
        else Printf("Preimmortal\r");
    }
};

extern Config cfg;

namespace App {

void SetDevtype(uint32_t type32);
void TakeBatteryVoltage(uint32_t vbat);

// Evt processing
void OnBtnEvt(BtnEvtInfo_t btn_info);
void OnSecondEvt();

// Radio
void PrepareTxPkt(rPkt *ppkt);
void ProcessRxTbl(RxTable &tbl);

// App-specific commands parsing
void OnCmd(Shell *pshell);

} // namespace App