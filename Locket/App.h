#pragma once

#include "radio_lvl1.h"


enum class DevType {
    Searcher   = 0b0000,
    PlacePlus1 = 0b0001,
    PlacePlus2 = 0b0010,
    PlacePlus3 = 0b0011,
    Master     = 0b0100,
    PlaceMinus1 = 0b0101,
    PlaceMinus2 = 0b0110,
    PlaceMinus3 = 0b0111,
    Artifact    = 0b1000,
    Particle    = 0b1001,
    Path        = 0b1010,
    // 1011 is reserved
    Beast       = 0b1100,
    PlaceMinus1Magic = 0b1101,
    PlaceMinus2Magic = 0b1110,
    PlaceMinus3Magic = 0b1111,
};

inline constexpr const uint32_t kDevTypeCnt = 15;

class Config {
public:
    uint32_t id = 0;
    DevType type = DevType::Searcher;
    uint8_t tx_power = 0;
    void PrintTxPwr();
};

extern Config cfg;

namespace App {

// Used for FD
void LoadDevtypeAndStateFromEE();
void SetAndSaveTxPwr(uint8_t tx_pwr);
// With loading state: by dip or by saved type for FD
void SetDevtypeResetLoadState(uint32_t type32);
// With saving resetted state: by pill or cmd
void SetDevtypeResetSaveState(uint32_t type32);
void SetDevtypeResetSaveState(DevType type);

// Evt processing
#if BUTTONS_ENABLED
void OnBtnEvt(BtnEvtInfo_t btn_info);
#endif
void OnSecond();
void ApplyPill(int32_t pill_id);

// Radio
bool CheckIfTxAndPrepareRPkt(rPkt *ppkt);
bool CheckIfRx();
void ProcessRxTbl(RxTable &tbl);

// Dbg
void PrintState();
void SetGoodness(int32_t goodness);
void SetBeastRsrc(int32_t goodness);

} // namespace App