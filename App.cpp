#include "App.h"
#include "Sequences.h"
#include "led.h"

extern LedRGBwPower_t<3> Led;
Config cfg;

struct DevIdName {
    DevType type;
    const char* name;
    LedRGBChunk_t *lsq_self;
};



static const DevIdName dev_id_names[kDevTypeCnt] = {
    {DevType::Searcher,         "Searcher",         lsqSearcher },
    {DevType::PlacePlus1,       "PlacePlus1",       lsqPlacePlus1 },
    {DevType::PlacePlus2,       "PlacePlus2",       lsqPlacePlus2 },
    {DevType::PlacePlus3,       "PlacePlus3",       lsqPlacePlus3 },
    {DevType::Master,           "Master",           lsqMaster },
    {DevType::PlaceMinus1,      "PlaceMinus1",      lsqPlaceMinus1 },
    {DevType::PlaceMinus2,      "PlaceMinus2",      lsqPlaceMinus2 },
    {DevType::PlaceMinus3,      "PlaceMinus3",      lsqPlaceMinus3 },
    {DevType::Artifact,         "Artifact",         lsqArtifact },
    {DevType::Beast,            "Beast",            lsqBeast },
    {DevType::PlaceMinus1Magic, "PlaceMinus1Magic", lsqPlaceMinus1Magic },
    {DevType::PlaceMinus2Magic, "PlaceMinus2Magic", lsqPlaceMinus2Magic },
    {DevType::PlaceMinus3Magic, "PlaceMinus3Magic", lsqPlaceMinus3Magic},
    {DevType::Particle,         "Particle",         lsqParticle },
    {DevType::Path,             "Path",             lsqPath }
};

static RetvValU32 DevTypeToIndx(DevType type) {
    for(uint32_t i=0; i<kDevTypeCnt; i++) {
        if(dev_id_names[i].type == type) return RetvValU32(retv::Ok, i);
    }
    return RetvValU32(retv::Fail, 0);
}

static bool IsDevTypeValid(uint32_t id) {
    for(uint32_t i=0; i<kDevTypeCnt; i++) {
        if(static_cast<uint32_t>(dev_id_names[i].type) == id) return true;
    }
    return false;
}

static void ShowSelfType() {
    RetvValU32 r = DevTypeToIndx(cfg.type);
    if(r.IsOk()) {
        Printf("DevType: %s\r", dev_id_names[*r].name);
        Led.StartOrAddToQueue(dev_id_names[*r].lsq_self);
    }
    else Printf("DevType: Unknown\r");
}

// RetvValPConstChar r = DevTypeToString(type);
// if(r.IsOk()) Printf("DevType: %s, Pwr: %S\r", *r, CC_PwrToString(tx_power));
// else Printf("DevType: Unknown, Pwr: %S\r", CC_PwrToString(tx_power));


void Config::PrintTxPwr() {
    Printf("TxPwr: %S\r", CC_PwrToString(tx_power));
}





void ProcessRxTbl(RxTable &tbl) {
    /*
    if(cfg.type != DevType::Witch) return; // Only witches can feel
    // === Analyze table ===
    uint32_t witch_cnt = 0;
    bool saint_place_is_near = false, witch_place_is_near = false;
    for(uint32_t i=0; i<tbl.cnt; i++) {
        // If Saint Place is near - indicate it and go out
        if(tbl[i].type == (uint8_t)DevType::SaintPlace) {
            saint_place_is_near = true;
            break;
        }
        else if(tbl[i].type == (uint8_t)DevType::WitchPlace) witch_place_is_near = true;
        else witch_cnt++;  // witch is here!
    } // for
    // === Indicate ===
    if(saint_place_is_near) Led.StartOrRestart(lsqSaintPlace); // ...and do no more
    else {
        // Present witches
        switch(witch_cnt) {
            case 0:  break; // Noone near
            case 1:  Led.StartOrRestart(lsqWitch1); break;
            case 2:  Led.StartOrRestart(lsqWitch2); break;
            default: Led.StartOrRestart(lsqWitchMany); break;
        } // switch
        if(cfg.VibroEnabled()) {
            switch(witch_cnt) {
                case 0:  break; // Noone near
                case 1:  Vibro.StartOrContinue(vsqBrr); break;
                case 2:  Vibro.StartOrContinue(vsqBrrBrr); break;
                default: Vibro.StartOrContinue(vsqBrrBrrBrr); break;
            } // switch
        }
        // Present witch place if any
        if(witch_place_is_near) {
            Led.StartOrAddToQueue(lsqWitchPlace);
        }
    } // else
    // Present self
    ShowSelfType();
    */
}

void OnBtnPress(BtnEvtInfo_t btn_info) {

}

void SetDevtype(uint32_t id) {
    if(IsDevTypeValid(id)) {
        cfg.type = static_cast<DevType>(id);
        ShowSelfType();
    }
    else Printf("Invalid dev type: %u\r");
}
