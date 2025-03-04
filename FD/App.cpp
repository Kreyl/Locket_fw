#include "App.h"
#include "Sequences.h"
#include "led.h"
#include "vibro.h"
#include "ch.h"
#include "beeper.h"
#include "kl_lib.h"

extern LedRGBwPower_t<11> Led;
extern Vibro_t<4> vibro;
extern Beeper_t<4> beeper;
Config cfg;
static uint32_t time_s = 0;

void Reset();

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
    {DevType::Particle,         "Particle",         lsqParticle },
    {DevType::Path,             "Path",             lsqPath },
    {DevType::Beast,            "Beast",            lsqBeast },
    {DevType::PlaceMinus1Magic, "PlaceMinus1Magic", lsqPlaceMinus1Magic },
    {DevType::PlaceMinus2Magic, "PlaceMinus2Magic", lsqPlaceMinus2Magic },
    {DevType::PlaceMinus3Magic, "PlaceMinus3Magic", lsqPlaceMinus3Magic},
};

static RetvValU32 DevTypeToIndx(DevType type) {
    for(uint32_t i=0; i<kDevTypeCnt; i++) {
        if(dev_id_names[i].type == type) return RetvValU32(retv::Ok, i);
    }
    return RetvValU32(retv::Fail, 0);
}

static bool IsDevTypeValid(uint32_t type32) {
    for(uint32_t i=0; i<kDevTypeCnt; i++) {
        if(static_cast<uint32_t>(dev_id_names[i].type) == type32) return true;
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
    int16_t goodness = 0;
    bool green_evil = false;
    bool must_tx = false;

    void EnableGoodness(int16_t agoodness) {
        chSysLock();
        goodness = agoodness;
        green_evil = false;
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
        goodness = 0;
        green_evil = false;
        must_tx = false;
    }
} tx_params;

// Radio RX
struct Influence {
    int32_t goodness_delta;
    int32_t green_evil;
    int32_t artifact;
    int32_t cyan_beast;
    int32_t searcher;
    // For master's indication
    int32_t goodness_plus;
    int32_t goodness_minus;
    void Reset() {
        goodness_delta = 0;
        green_evil = 0;
        artifact = 0;
        cyan_beast = 0;
        searcher = 0;
        goodness_plus = 0;
        goodness_minus = 0;
    }
    void Print() {
        Printf("goodness_delta %d\rgreen_evil %d\rartifact %d\rcyan_beast %d\rsearcher %d\rgoodness_plus %d\rgoodness_minus %d\r",
            goodness_delta, green_evil, artifact, cyan_beast, searcher, goodness_plus, goodness_minus);
    }
    Influence& operator = (const Influence &right) {
        chSysLock();
        goodness_delta = right.goodness_delta;
        green_evil = right.green_evil;
        artifact = right.artifact;
        cyan_beast = right.cyan_beast;
        searcher = right.searcher;
        goodness_plus = right.goodness_plus;
        goodness_minus = right.goodness_minus;
        chSysUnlock();
        return *this;
    }
};
static Influence influence, new_influence;

// ==== Single goodness injection by master ====
inline constexpr const uint32_t kMinDelayBetweenInjs_s = 18;
static class GTransaction {
private:
    static const uint32_t kCntMax = 9;
    struct GTransItem {
        uint32_t id;
        uint32_t time_last_rx = 0;
        bool IsExpired() { return (time_s - time_last_rx) > kMinDelayBetweenInjs_s; }
    };
    GTransItem arr[kCntMax];
public:
    retv ProcessId(uint32_t id) {
        retv rslt = retv::Same;
        uint32_t last_empty_indx = 0;
        for(uint32_t i=0; i<kCntMax; i++) {
            if(arr[i].id == 0) last_empty_indx = i;
            else {
                if(arr[i].id == id) { // ID presents, check if still fresh
                    if(arr[i].IsExpired()) rslt = retv::New;
                    arr[i].time_last_rx = time_s; // Renew last rx time, anyway
                    return rslt; // New if expired, Same otherwise
                }
                else { // Some other id, check if time to empty it
                    if(arr[i].IsExpired()) {
                        arr[i].id = 0;
                        last_empty_indx = i;
                    }
                } // some other id
            } // not zero
        } // for
        // ID not present, insert it
        arr[last_empty_indx].id = id;
        arr[last_empty_indx].time_last_rx = time_s;
        return retv::New;
    }
    void Reset() {
        for(uint32_t i=0; i<kCntMax; i++) arr[i].id = 0;
    }
    void Print() {
        Printf("GTransList:\r");
        bool is_empty = true;
        for(uint32_t i=0; i<kCntMax; i++) {
            if(arr[i].id == 0) continue;
            Printf("  * id %X; rcvd %u s ago\r", arr[i].id, time_s - arr[i].time_last_rx);
            is_empty = false;
        }
        if(is_empty) Printf("  empty\r");
    }
} g_trans_list;

// Modifiers
inline constexpr const int32_t kFixTimedVulueMax = 7200;
struct Modifier {
    int32_t fix_timed = 0;
    bool fix_forever = false;
    bool IsFixed() { return fix_forever or fix_timed > 0; }
    void Reset() { fix_forever = false; fix_timed = 0; }
    void Print() { Printf("FixForever: %d; FixTimed: %d\r", fix_forever, fix_timed); }
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

#pragma region // ======== EEPROM ========
inline constexpr const uint32_t kEEAddrType = 36, kEEAddrGoodness = 40, kEEAddrBeastRsrc = 44;
inline constexpr const uint32_t kEEAddrFixTimed = 48, kEEAddrFixForever = 52;

void EESaveGoodness()   { EE::WriteI32(kEEAddrGoodness, goodness); }
void EESaveBeastRsrc()  { EE::WriteI32(kEEAddrBeastRsrc, beast_resource); }
void EESaveFixTimed()   { EE::WriteI32(kEEAddrFixTimed, modifier.fix_timed); }
void EESaveFixForever() { EE::WriteI32(kEEAddrFixForever, modifier.fix_forever); }

void EESaveState() {
    EESaveGoodness();
    EESaveBeastRsrc();
    EESaveFixTimed();
    EESaveFixForever();
}

void EELoadState() {
    int32_t v = EE::ReadI32(kEEAddrGoodness);
    if(v >= 0 and v <= kGoodnessMax) goodness = v;
    else goodness = kGoodnessDefault;
    v = EE::ReadI32(kEEAddrBeastRsrc);
    if(v >= kBeaastMin and v <= kBeastMax) beast_resource = v;
    else beast_resource = kBeastDefault;
    v = EE::ReadI32(kEEAddrFixTimed);
    if(v >= 0 and v <= kFixTimedVulueMax) modifier.fix_timed = v;
    else modifier.fix_timed = 0;
    v = EE::ReadI32(kEEAddrFixForever);
    if(v == 1) modifier.fix_forever = true;
    else modifier.fix_forever = false;
}
#pragma endregion

void InjectGoodnessUnconditional(int32_t goodness_value) {
    switch(cfg.type) {
        case DevType::Searcher:
        case DevType::Particle:
            goodness += goodness_value;
            if(goodness > kGoodnessMax) goodness = kGoodnessMax;
            else if(goodness < 0) goodness = 0;
            EESaveGoodness();
            break;
        case DevType::Beast:
            beast_resource -= goodness_value;
            if(beast_resource < 0) beast_resource = 0;
            EESaveBeastRsrc();
            break;
        default:
            break;
    } // switch(cfg.type)
}

void InjectGoodnessConditional(int32_t goodness_value) {
    if(!modifier.IsFixed()) {
        InjectGoodnessUnconditional(goodness_value);
    }
}

void ProcessGoodnessForParticle() {
    if(!modifier.IsFixed()) { // do not change if fixed
        // Add influence if > 0: Lustra(+1) must increment goodness by 1, Lustra(+2) - by 2 etc.
        if(influence.goodness_delta > 0) goodness += influence.goodness_delta;
        else goodness = goodness - 1L + influence.goodness_delta;
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
            beast_resource--; // Always decrease
            // Positive influence decreases resource, negative one does nothing
            if(influence.goodness_delta > 0) beast_resource -= influence.goodness_delta * 5L;
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
            if(influence.green_evil > 0) vibro.StartOrAddToQueue(vsqBrrBrr);
            if(influence.artifact   > 0) vibro.StartOrAddToQueue(vsqBrrBrrBrr);
            if(influence.cyan_beast > 0) vibro.StartOrAddToQueue(vsqBrr);
            break;

        case DevType::PlacePlus1: Led.StartOrAddToQueue(lsqPlacePlus1_inwork); break;
        case DevType::PlacePlus2: Led.StartOrAddToQueue(lsqPlacePlus2_inwork); break;
        case DevType::PlacePlus3: Led.StartOrAddToQueue(lsqPlacePlus3_inwork); break;

        case DevType::Master:
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
            Led.StartOrAddToQueue(lsqMaster_inwork);
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
                if(time_s % 60 == 0) vibro.StartOrAddToQueue(vsqBrr); // Every minute
            }
            else if(beast_resource > 0) {
                Led.StartOrAddToQueue(lsqBeast3_inwork);
                if(time_s % 30 == 0) vibro.StartOrAddToQueue(vsqBrrBrr); // Every 30 seconds
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


void Reset() {
    time_s = 0;
    goodness = kGoodnessDefault;
    beast_resource = kBeastDefault;
    modifier.Reset();
    tx_params.Reset();
    influence.Reset();
    g_trans_list.Reset();
    ShowSelfType();
}

namespace App {

// Set type, reset and LOAD params
void SetDevtypeResetLoadState(uint32_t type32) {
    if(IsDevTypeValid(type32)) {
        cfg.type = static_cast<DevType>(type32);
        Reset();
        EELoadState();
    }
    else Printf("Invalid dev type: %u\r", type32);
}

void SetDevtypeResetSaveState(uint32_t type32) {
    if(IsDevTypeValid(type32)) {
        cfg.type = static_cast<DevType>(type32);
        Reset();
        EESaveState();
    }
    else Printf("Invalid dev type: %u\r", type32);
}

void SetDevtypeResetSaveState(DevType type) {
    cfg.type = type;
    Reset();
    EESaveState();
}


void OnSecond() {
    time_s++;
    // Process goodness
    switch(cfg.type) {
        case DevType::Searcher:
        case DevType::Particle:
            ProcessGoodnessForParticle();
            if(time_s % 64 == 0) EESaveGoodness();
            break;
        case DevType::Beast:
            ProcessGoodnessForBeast();
            if(time_s % 64 == 0) EESaveBeastRsrc();
            break;
        default: // Places, master, artifact, path
            break;
    } // switch
    // Indicate every 4 seconds
    if(time_s % 4 == 0) Indicate();
}

void ApplyPill(int32_t pill_id) {
    Printf("Pill");
    switch(pill_id) {
        case 1:
            Led.StartOrAddToQueue(lsqPillReset);
            Printf("Reset\r");
            Reset();
            EESaveState();
            break;
        case 2:
            Led.StartOrAddToQueue(lsqPillGoodnessPlus);
            Printf("GPlus\r");
            InjectGoodnessUnconditional(+1200); // Saved inside
            break;
        case 3:
            Led.StartOrAddToQueue(lsqPillGoodnessMinus);
            Printf("GMinus\r");
            InjectGoodnessUnconditional(-1200); // Saved inside
            break;
        case 4:
            Led.StartOrAddToQueue(lsqPillFixForever);
            Printf("FixForever\r");
            modifier.fix_forever = true;
            EESaveFixForever();
            break;
        case 5:
            Led.StartOrAddToQueue(lsqPillFixTimed);
            Printf("FixTimed\r");
            modifier.fix_timed = 3600;
            EESaveFixTimed();
            break;
        case 6:
            Led.StartOrAddToQueue(lsqPillDisableFix);
            Printf("DisableFix\r");
            modifier.Reset();
            EESaveFixTimed();
            EESaveFixForever();
            break;

        // Type switch
        case  7: Printf("SetType: Particle\r"); SetDevtypeResetSaveState(DevType::Particle); break;
        case  8: Printf("SetType: Searcher\r"); SetDevtypeResetSaveState(DevType::Searcher); break;
        case  9: Printf("SetType: Beast\r");    SetDevtypeResetSaveState(DevType::Beast);    break;
        case 10: Printf("SetType: Path\r");     SetDevtypeResetSaveState(DevType::Path);     break;

        default:
            Printf("Bad: %d\r", pill_id);
            Led.StartOrAddToQueue(lsqPillBad);
            beeper.StartOrRestart(bsqBeepPillBad);
            return; // Get out before switch ends
    } // switch
    // Will be here if pill is good
    beeper.StartOrRestart(bsqBeepPillOk);
}

#if BUTTONS_ENABLED
void OnBtnEvt(BtnEvtInfo_t btn_info) {
    if(cfg.type == DevType::Master) {
        // Indicate by vibro
        if(btn_info.type == beShortPress) vibro.StartOrRestart(vsqBrrForever);
        else vibro.Stop();
        // Get the job done
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
    } // If master
}
#endif

#pragma region // ==== Radio related ====
// RX. Called from radio lvl
bool CheckIfRx() {
    switch(cfg.type) {
        case DevType::Searcher: return true;

        case DevType::PlacePlus1:
        case DevType::PlacePlus2:
        case DevType::PlacePlus3:
            return false;

        case DevType::Master: return true;

        case DevType::PlaceMinus1:
        case DevType::PlaceMinus2:
        case DevType::PlaceMinus3:
            return false;

        case DevType::Artifact: return false;
        case DevType::Beast: return true;

        case DevType::PlaceMinus1Magic:
        case DevType::PlaceMinus2Magic:
        case DevType::PlaceMinus3Magic:
            return false;

        case DevType::Particle: return true;
        case DevType::Path: return true;
    } // switch
    return false; // Will newer be here
}

// RX. Called from main thread by evt which is periodically sent by radio
void ProcessRxTbl(RxTable &tbl) {
    new_influence.Reset();
    for(uint32_t i=0; i<tbl.cnt; i++) {
        rPkt &pkt = tbl[i]; // Single pkt from one ID
        pkt.Print();
        // Goodness: add it even if its value is zero, because who cares?
        if(pkt.single_transaction) { // Master's whim
            if(g_trans_list.ProcessId(pkt.id) == retv::New) {
                InjectGoodnessConditional(pkt.goodness);
            }
        }
        else { // Not a single ransaction, just field
            new_influence.goodness_delta += pkt.goodness;
            // For master's indication
            switch(pkt.goodness) {
                case  1: if(new_influence.goodness_plus  <  1) { new_influence.goodness_plus  =  1; } break;
                case  2: if(new_influence.goodness_plus  <  2) { new_influence.goodness_plus  =  2; } break;
                case  3: if(new_influence.goodness_plus  <  3) { new_influence.goodness_plus  =  3; } break;
                case -1: if(new_influence.goodness_minus > -1) { new_influence.goodness_minus = -1; } break;
                case -2: if(new_influence.goodness_minus > -2) { new_influence.goodness_minus = -2; } break;
                case -3: if(new_influence.goodness_minus > -3) { new_influence.goodness_minus = -3; } break;
                default: break;
            }
        }
        // Green Evil, Artifact, Beast, Searcher. Value >1 is useful for debug / master's indication
        if(pkt.green_evil) new_influence.green_evil++;
        if(pkt.artifact)   new_influence.artifact++;
        if(pkt.cyan_beast) new_influence.cyan_beast++;
        if(pkt.searcher)   new_influence.searcher++;
    }
    influence = new_influence; // Apply what received
}

// Tx. Called from radio level
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
            if(tx_params.goodness != 0) { // Transmit goodness with single transaction flag
                ppkt->goodness = tx_params.goodness;
                ppkt->single_transaction = 1;
            }
            else if(tx_params.green_evil) ppkt->green_evil = 1;
            chSysUnlock();
            break;

        case DevType::PlaceMinus1: ppkt->goodness = -1; break;
        case DevType::PlaceMinus2: ppkt->goodness = -2; break;
        case DevType::PlaceMinus3: ppkt->goodness = -3; break;

        case DevType::Artifact:    ppkt->artifact = 1; break;
        case DevType::Beast:       ppkt->cyan_beast = 1; break;

        case DevType::PlaceMinus1Magic: ppkt->goodness = -1; ppkt->green_evil = 1; break;
        case DevType::PlaceMinus2Magic: ppkt->goodness = -2; ppkt->green_evil = 1; break;
        case DevType::PlaceMinus3Magic: ppkt->goodness = -3; ppkt->green_evil = 1; break;

        default: return false; // Impossible to get here, but just in case
    } // switch
    return true;
}
#pragma endregion

void GetState() {
    RetvValU32 r = DevTypeToIndx(cfg.type);
    if(r.NotOk()) { Printf("Bad Type: %u\r", cfg.type); return; }
    Printf("DevType: %s\r", dev_id_names[*r].name);
    Printf("Goodness: %d\r", goodness);
    Printf("BeastRsrc: %d\r", beast_resource);
    modifier.Print();
    influence.Print();
    g_trans_list.Print();
}

void SetGoodness(int32_t agoodness) { goodness = agoodness; }
void SetBeastRsrc(int32_t rsrc) { beast_resource = rsrc; }

} // namespace App