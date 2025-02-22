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
    Beast       = 0b1100,
    PlaceMinus1Magic = 0b1101,
    PlaceMinus2Magic = 0b1110,
    PlaceMinus3Magic = 0b1111,
    Particle = 27,
    Path = 36
};

inline constexpr const uint32_t kDevTypeCnt = 15;

class Config {
public:
    static const int32_t kIdMin=0, kIdMax=50, kIdBad=-1, kIdDefault = kIdMin;
    DevType type = DevType::Searcher;
    int32_t id = kIdMin;
    uint8_t tx_power = 0;
    void PrintTxPwr();
};

extern Config cfg;

void SetDevtype(uint32_t id);

// Evt processing
void OnBtnPress(BtnEvtInfo_t btn_info);
void ProcessRxTbl(RxTable &tbl);
