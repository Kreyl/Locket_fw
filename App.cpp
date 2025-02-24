#include "App.h"
#include "Sequences.h"
#include "led.h"
#include "vibro.h"
#include "ch.h"

extern LedRGBwPower_t<7> Led;
extern Vibro_t<4> vibro;
Config cfg;
static uint32_t seconds_passed = 0;

#pragma region // ==== DevType related ====
struct DevIdName {
    DevType type;
    const char* name;
    const LedRGBChunk_t *lsq_self;
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

void Config::PrintTxPwr() {
    Printf("TxPwr: %S\r", CC_PwrToString(tx_power));
}
#pragma endregion

#pragma region // ==== Influence & Modifiers ====
// Radio TX
static struct {
    uint32_t prev_g_en_time = 0; // When goodness was enabled previously
    uint32_t transaction_id = 0;
    int16_t goodness = 0;
    bool green_evil = false;
    bool must_tx = false;
    void EnableGoodness(int16_t agoodness) {
        chSysLock();
        goodness = agoodness;
        green_evil = false;
        // Calculate new transaction ID if enough time has passed since prev time.
        // This is needed to handle situation with short button release and press again
        if(prev_g_en_time == 0 or (seconds_passed - prev_g_en_time) > 4) {
            uint32_t new_id;
            do {
                new_id = Random::Generate(1, 0xFFFFFF);
            } while(new_id == transaction_id);
            transaction_id = new_id;
        }
        prev_g_en_time = seconds_passed;
        must_tx = true;
        chSysUnlock();
    }
    void EnableGreenEvil() {
        chSysLock();
        goodness = 0;
        green_evil = true;
        must_tx = true;
        chSysUnlock();
    }
    void Disable() { must_tx = false; }
    void Reset() {
        prev_g_en_time = 0;
        transaction_id = 0;
        goodness = 0;
        green_evil = false;
        must_tx = false;
    }
} tx_params;

// Radio RX
static struct {
    int32_t goodness = 0;
    int32_t green_evil = 0;
    int32_t artifact = 0;
    int32_t cyan_beast = 0;
    int32_t searcher = 0;
    // For master's indication
    int32_t goodness_plus = 0;
    int32_t goodness_minus = 0;
} influence;

// Modifiers
struct Modifier {
    bool fix_forever = false;
    int32_t fix_timed = 0;
    bool IsFixed() { return fix_forever or fix_timed > 0; }
    void Reset() { fix_forever = false; fix_timed = 0; }
};
static Modifier modifier;
#pragma endregion

#pragma region // ==== Goodness & Beast resource ====
// Goodness
static const int32_t kGoodnessMax = 14400,  kGoodnessDefault = kGoodnessMax;
static int32_t goodness = kGoodnessDefault;
// Beast resource
static const int32_t kBeastMax = 21600, kBeaastMin = -600, kBeastDefault = kBeastMax;
static int32_t beast_resource = kBeastDefault;

void InjectGoodnessUnconditional(int32_t goodness_value) {
    switch(cfg.type) {
        case DevType::Searcher:
        case DevType::Particle:
            goodness += goodness_value;
            if(goodness > kGoodnessMax) goodness = kGoodnessMax;
            else if(goodness < 0) goodness = 0;
            break;
        case DevType::Beast:
            beast_resource -= goodness_value;
            if(beast_resource < 0) beast_resource = 0;
            break;
        default:
            break;
    } // switch(cfg.type)
}

void ProcessGoodnessForParticle() {
    if(!modifier.IsFixed()) { // do not change if fixed
        // Apply influence if any
        if(influence.goodness != 0) goodness += influence.goodness;
        else goodness--; // No influence, just decrease
        // Keep goodness in range
        if(goodness > kGoodnessMax) goodness = kGoodnessMax;
        else if(goodness < 0) goodness = 0;
    }
    // Process modifiers
    if(modifier.fix_timed > 0) modifier.fix_timed--;
}

void ProcessGoodnessForBeast() {
    if(beast_resource > 0) {
        if(!modifier.IsFixed()) { // do not change if fixed
            // Positive goodness decreases resource
            if(influence.goodness > 0) beast_resource -= (1L + influence.goodness * 5L);
            else beast_resource--; // No influence, just decrease
            // Keep resource in range
            if(beast_resource > kBeastMax) beast_resource = kBeastMax;
            else if(beast_resource < 0) beast_resource = 0;
        }
        // Process modifiers
        if(modifier.fix_timed > 0) modifier.fix_timed--;
    }
    else { // beast_resource<=0 => no influence/modifiers are applicable
        if(beast_resource > kBeaastMin) beast_resource--;
        modifier.fix_timed = 0;
    }
}
#pragma endregion

// ==== Indication ====
void IndicateGoodness() {
    bool is_frozen = modifier.fix_forever or modifier.fix_timed > 0;
    if(goodness >= 9599)
        Led.StartOrAddToQueue(is_frozen? lsqGoodnessBlueFrozen : lsqGoodnessBlue);
    else if(goodness >= 4800)
        Led.StartOrAddToQueue(is_frozen? lsqGoodnessYellowFrozen : lsqGoodnessYellow);
    else
        Led.StartOrAddToQueue(is_frozen? lsqGoodnessRedFrozen : lsqGoodnessRed);
}

void Indicate() {
    switch(cfg.type) {
        case DevType::Searcher:
            IndicateGoodness();
            // Indicate magic
            if(influence.green_evil > 0) vibro.StartOrAddToQueue(vsqBrr);
            if(influence.artifact   > 0) vibro.StartOrAddToQueue(vsqBrrBrr);
            if(influence.cyan_beast > 0) vibro.StartOrAddToQueue(vsqBrrBrrBrr);
            break;

        case DevType::PlacePlus1: Led.StartOrAddToQueue(lsqPlacePlus1_inwork); break;
        case DevType::PlacePlus2: Led.StartOrAddToQueue(lsqPlacePlus2_inwork); break;
        case DevType::PlacePlus3: Led.StartOrAddToQueue(lsqPlacePlus3_inwork); break;

        case DevType::Master:
            Led.StartOrAddToQueue(lsqMaster_inwork);
            switch(influence.goodness_plus) {
                case 1: Led.StartOrAddToQueue(lsqPlacePlus1); break;
                case 2: Led.StartOrAddToQueue(lsqPlacePlus2); break;
                case 3: Led.StartOrAddToQueue(lsqPlacePlus3); break;
                default: break;
            }
            switch(influence.goodness_minus) {
                case -1: Led.StartOrAddToQueue(lsqPlaceMinus1); break;
                case -2: Led.StartOrAddToQueue(lsqPlaceMinus2); break;
                case -3: Led.StartOrAddToQueue(lsqPlaceMinus3); break;
                default: break;
            }
            switch(influence.green_evil) {
                case 1: Led.StartOrAddToQueue(lsqGreenEvil1); break;
                case 2: Led.StartOrAddToQueue(lsqGreenEvil2); break;
                case 3: Led.StartOrAddToQueue(lsqGreenEvil3); break;
                default: break;
            }
            switch(influence.artifact) {
                case 1: Led.StartOrAddToQueue(lsqArtifact1); break;
                case 2: Led.StartOrAddToQueue(lsqArtifact2); break;
                case 3: Led.StartOrAddToQueue(lsqArtifact3); break;
                default: break;
            }
            switch(influence.cyan_beast) {
                case 1: Led.StartOrAddToQueue(lsqCyanBeast1); break;
                case 2: Led.StartOrAddToQueue(lsqCyanBeast2); break;
                case 3: Led.StartOrAddToQueue(lsqCyanBeast3); break;
                default: break;
            }
            switch(influence.searcher) {
                case 1: Led.StartOrAddToQueue(lsqSearcher1); break;
                case 2: Led.StartOrAddToQueue(lsqSearcher2); break;
                case 3: Led.StartOrAddToQueue(lsqSearcher3); break;
                default: break;
            }
            break;

        case DevType::PlaceMinus1: Led.StartOrAddToQueue(lsqPlaceMinus1_inwork); break;
        case DevType::PlaceMinus2: Led.StartOrAddToQueue(lsqPlaceMinus2_inwork); break;
        case DevType::PlaceMinus3: Led.StartOrAddToQueue(lsqPlaceMinus3_inwork); break;

        case DevType::Artifact: Led.StartOrAddToQueue(lsqArtifact_inwork); break;

        case DevType::Beast:
            if(beast_resource > 10800) {
                Led.StartOrAddToQueue(lsqBeast1_inwork);
            }
            else if(beast_resource > 3600) {
                Led.StartOrAddToQueue(lsqBeast2_inwork);
                if(seconds_passed % 60 == 0) vibro.StartOrAddToQueue(vsqBrr); // Every minute
            }
            else if(beast_resource > 0) {
                Led.StartOrAddToQueue(lsqBeast3_inwork);
                if(seconds_passed % 30 == 0) vibro.StartOrAddToQueue(vsqBrrBrr); // Every 30 seconds
            }
            else if(beast_resource > -kBeaastMin) { // Out of Control
                Led.StartOrAddToQueue(lsqBeast4_inwork);
                vibro.StartOrAddToQueue(vsqBrrBrrBrr);
            }
            else { // Madness
                Led.StartOrAddToQueue(lsqBeastMadness);
            }
            break;

        case DevType::PlaceMinus1Magic: Led.StartOrAddToQueue(lsqPlaceMinus1Magic_inwork); break;
        case DevType::PlaceMinus2Magic: Led.StartOrAddToQueue(lsqPlaceMinus2Magic_inwork); break;
        case DevType::PlaceMinus3Magic: Led.StartOrAddToQueue(lsqPlaceMinus3Magic_inwork); break;

        case DevType::Particle: IndicateGoodness(); break;

        case DevType::Path:
            if(influence.searcher > 0) Led.StartOrAddToQueue(lsqPathFadeIn);
            else Led.StartOrAddToQueue(lsqPathFadeOut);
            break;
    } // switch
}


void ProcessRxTbl(RxTable *ptbl) {
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

void OnSecond() {
    seconds_passed++;
    bool time_to_act = (seconds_passed % 4 == 0);
    // Process goodness
    switch(cfg.type) {
        case DevType::Searcher:
        case DevType::Particle:
            ProcessGoodnessForParticle();
            break;
        case DevType::Beast:
            ProcessGoodnessForBeast();
            break;
        default: // Places, master, artifact, path
            break;
    } // switch
    // Indicate if needed
    if(time_to_act) Indicate();
}

void Reset() {
    seconds_passed = 0;
    goodness = kGoodnessDefault;
    beast_resource = kBeastDefault;
    modifier.Reset();
    tx_params.Reset();
    ShowSelfType();
}

void ApplyPill(int32_t pill_id) {
    switch(pill_id) {
        case  1: InjectGoodnessUnconditional(+1200); break;
        case  2: InjectGoodnessUnconditional(-1200); break;
        case  3: modifier.fix_forever = true; break;
        case  4: modifier.fix_timed = 3600;   break;
        case  5: modifier.Reset(); break;
        case  6: Reset(); break;
        // Type switch
        case  7: cfg.type = DevType::Particle; break;
        case  8: cfg.type = DevType::Searcher; break;
        case  9: cfg.type = DevType::Beast; break;
        case 10: cfg.type = DevType::Path; break;

        default: Printf("Invalid pill: %d\r", pill_id); break;
    }
}

bool CheckIfTxAndPrepareRPkt(rPkt *ppkt) {
    // Particle & Path do not transmit
    if(cfg.type == DevType::Particle or cfg.type == DevType::Path) return false;
    // Others save master must transmit all the time
    ppkt->Reset(cfg.id); // Zero all
    switch(cfg.type) {
        case DevType::Searcher:   ppkt->searcher = 1; break;

        case DevType::PlacePlus1: ppkt->goodness = 1; break;
        case DevType::PlacePlus2: ppkt->goodness = 2; break;
        case DevType::PlacePlus3: ppkt->goodness = 3; break;

        case DevType::Master:
            if(!tx_params.must_tx) return false;
            chSysLock();
            if(tx_params.goodness != 0) {
                ppkt->transaction_id = tx_params.transaction_id;
                ppkt->goodness = tx_params.goodness;
            }
            else if(tx_params.green_evil) ppkt->green_evil = 1;
            chSysUnlock();
            break;

        case DevType::PlaceMinus1: ppkt->goodness = -1; break;
        case DevType::PlaceMinus2: ppkt->goodness = -2; break;
        case DevType::PlaceMinus3: ppkt->goodness = -3; break;

        case DevType::Artifact: ppkt->artifact = 1; break;
        case DevType::Beast: ppkt->cyan_beast = 1; break;

        case DevType::PlaceMinus1Magic: ppkt->goodness = -1; ppkt->green_evil = 1; break;
        case DevType::PlaceMinus2Magic: ppkt->goodness = -2; ppkt->green_evil = 1; break;
        case DevType::PlaceMinus3Magic: ppkt->goodness = -3; ppkt->green_evil = 1; break;

        default: return false; // Impossible to get here, but just in case
    } // switch
    return true;
}

void OnBtnPress(BtnEvtInfo_t btn_info) {
    if(cfg.type == DevType::Master) {
        switch(btn_info.btn_indx) {
            case 0:  // Top button
                if(btn_info.type == beShortPress) tx_params.EnableGoodness(+1200);
                else tx_params.Disable(); // Release
                break;
            case 1:  // Middle button
                if(btn_info.type == beShortPress) tx_params.EnableGreenEvil();
                else tx_params.Disable(); // Release
                break;
            case 2:  // Bottom button
                if(btn_info.type == beShortPress) tx_params.EnableGoodness(-1200);
                else tx_params.Disable(); // Release
                break;
            default: break;
        } // switch
    }
}

void SetDevtype(uint32_t id) {
    if(IsDevTypeValid(id)) {
        cfg.type = static_cast<DevType>(id);
        Reset(); // Show self type inside
    }
    else Printf("Invalid dev type: %u\r", id);
}
