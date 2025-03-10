#include "App.h"
#include "Sequences.h"
#include "led.h"
#include "vibro.h"
#include "ch.h"
#include "beeper.h"
#include "kl_lib.h"

#ifdef LED_EN_PIN // Locket
extern LedRGBwPower_t<11> led;
#else // FD
extern LedRGB_t<11> led;
#endif
extern Vibro_t<4> vibro;
extern Beeper_t<4> beeper;
Config cfg;
static uint32_t time_s = 0;

void Reset();
void EESaveGoodness();
void EESaveBeastRsrc();
void EESaveFixTimed();

#pragma region // ==== DevType related ====
struct DevIdName {
    DevType type;
    const char* name;
    const LedRGBChunk *lsq_self;
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
        led.StartOrAddToQueue(dev_id_names[*r].lsq_self);
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
inline constexpr const uint32_t kMinDelayBetweenInjs_s = 18; // TODO Set 60s here
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
    void DecreaseFixTimedAndSaveIfZero() {
        if(fix_timed > 0) {
            fix_timed--;
            if(fix_timed == 0) EESaveFixTimed();
        }
    }
};
static Modifier modifier;
#pragma endregion

#pragma region // ==== Goodness & Beast resource ====
namespace Particle { // And searcher is Particle, too
    static const int32_t kMax = 14400,  kDefault = kMax;
    static int32_t goodness = kDefault;

    void InjectGoodness(int32_t delta) {
        int32_t new_goodness = goodness;
        new_goodness += delta;
        // Keep goodness in range [0..kMax]
        if(new_goodness > kMax) new_goodness = kMax;
        else if(new_goodness < 0) new_goodness = 0;
        if(goodness == new_goodness) return; // No changes
        goodness = new_goodness;
        EESaveGoodness();
        vibro.StartOrRestart(vsqBrrBrr);
    }

    void OnSecond() {
        if(!modifier.IsFixed()) { // do not change if fixed
            // Add influence if positive: Lustra(+1) must increment goodness by 1, Lustra(+2) - by 2 etc.
            if(influence.goodness_delta > 0) goodness += influence.goodness_delta;
            // Otherwise, decrement goodness by 1 AND influence delta
            else goodness = goodness - 1L + influence.goodness_delta;
            // Keep goodness in range
            if(goodness > kMax) goodness = kMax;
            else if(goodness < 0) goodness = 0;
        }
        // Process modifiers
        modifier.DecreaseFixTimedAndSaveIfZero();
        // Save goodness every 64 seconds
        if(time_s % 64 == 0)  {
            EESaveGoodness();
            if(modifier.fix_timed != 0) EESaveFixTimed();
        }
    }
} // namespace Goodness

// Beast resource
namespace Beast {
    static const int32_t kMax = 21600L, kHangerMin = -600L, kMadness = kHangerMin - 1L, kDefault = kMax;
    static int32_t resource = kDefault;
    enum class State { Calm, Worry, Thrill, Hunger, Madness };

    void InjectGoodness(int32_t delta) {
        int32_t new_resource = resource;
        new_resource -= delta; // Positive goodness decreases resource, negative one increases it
        // Keep resource in range [0..kMax]
        if(new_resource > kMax) new_resource = kMax;
        else if(new_resource < 0) new_resource = 0; // Injection must not put to Hunger or Madness
        if(resource == new_resource) return; // No changes
        resource = new_resource;
        EESaveBeastRsrc();
        vibro.StartOrRestart(vsqBrrBrr);
    }

    State GetState() {
        if     (resource >= 10800L)     return State::Calm;
        else if(resource >= 3600L)      return State::Worry;
        else if(resource >= 0L)         return State::Thrill;
        else if(resource >= kHangerMin) return State::Hunger;
        else return State::Madness;
    }

    void OnSecond() {
        if(Beast::resource > 0) {
            if(!modifier.IsFixed()) { // do not change if fixed
                resource--; // Always decrease
                // Positive influence decreases resource, negative one does nothing
                if(influence.goodness_delta > 0) resource -= influence.goodness_delta * 5L;
                // Keep resource in range
                if(resource > kMax) resource = kMax;
                else if(resource < 0) resource = 0; // Do not decrease under 0 too fast, do it slowly ignoring influence
            }
            // Process modifiers
            modifier.DecreaseFixTimedAndSaveIfZero();
        }
        else { // beast_resource <= 0 => no influence/modifiers are applicable
            if(resource > kMadness) resource--;
            modifier.fix_timed = 0;
        }
        // Save resource every 64 seconds
        if(time_s % 64 == 0) {
            EESaveBeastRsrc();
            if(modifier.fix_timed != 0) EESaveFixTimed();
        }
    }
};
#pragma endregion

#pragma region // ======== EEPROM ========
inline constexpr const uint32_t kEEAddrType = 36, kEEAddrGoodness = 40, kEEAddrBeastRsrc = 44;
inline constexpr const uint32_t kEEAddrFixTimed = 48, kEEAddrFixForever = 52;
inline constexpr const uint32_t kEEAddrTxPwr = 54;

void EESaveType()       { EE::WriteI32(kEEAddrType,       static_cast<int32_t>(cfg.type)); Printf("# %S\r", __FUNCTION__); }
void EESaveGoodness()   { EE::WriteI32(kEEAddrGoodness,   Particle::goodness);             Printf("# %S\r", __FUNCTION__); }
void EESaveBeastRsrc()  { EE::WriteI32(kEEAddrBeastRsrc,  Beast::resource);                Printf("# %S\r", __FUNCTION__); }
void EESaveFixTimed()   { EE::WriteI32(kEEAddrFixTimed,   modifier.fix_timed);             Printf("# %S\r", __FUNCTION__); }
void EESaveFixForever() { EE::WriteI32(kEEAddrFixForever, modifier.fix_forever);           Printf("# %S\r", __FUNCTION__); }
void EESaveTxPwr()      { EE::WriteI32(kEEAddrTxPwr,      cfg.tx_power);                   Printf("# %S\r", __FUNCTION__); }

void EESaveState() {
    EESaveGoodness();
    EESaveBeastRsrc();
    EESaveFixTimed();
    EESaveFixForever();
}

void EELoadState() {
    int32_t v = EE::ReadI32(kEEAddrGoodness);
    if(v >= 0 and v <= Particle::kMax) Particle::goodness = v;
    else Particle::goodness = Particle::kDefault;
    v = EE::ReadI32(kEEAddrBeastRsrc);
    if(v >= Beast::kMadness and v <= Beast::kMax) Beast::resource = v;
    else Beast::resource = Beast::kDefault;
    v = EE::ReadI32(kEEAddrFixTimed);
    if(v >= 0 and v <= kFixTimedVulueMax) modifier.fix_timed = v;
    else modifier.fix_timed = 0;
    v = EE::ReadI32(kEEAddrFixForever);
    if(v == 1) modifier.fix_forever = true;
    else modifier.fix_forever = false;
}
#pragma endregion

// ==== Indication ====
void IndicateGoodness() {
    bool is_frozen = modifier.fix_forever or modifier.fix_timed > 0;
    if(Particle::goodness >= 9599)
        led.StartOrAddToQueue(is_frozen? lsqGoodnessBlueFrozen : lsqGoodnessBlue);
    else if(Particle::goodness >= 4800)
        led.StartOrAddToQueue(is_frozen? lsqGoodnessYellowFrozen : lsqGoodnessYellow);
    else
        led.StartOrAddToQueue(is_frozen? lsqGoodnessRedFrozen : lsqGoodnessRed);
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

        case DevType::PlacePlus1: led.StartOrAddToQueue(lsqPlacePlus1_inwork); break;
        case DevType::PlacePlus2: led.StartOrAddToQueue(lsqPlacePlus2_inwork); break;
        case DevType::PlacePlus3: led.StartOrAddToQueue(lsqPlacePlus3_inwork); break;

        case DevType::Master:
            switch(influence.goodness_plus) {
                case 1: led.StartOrAddToQueue(lsqPlacePlus1); break;
                case 2: led.StartOrAddToQueue(lsqPlacePlus2); break;
                case 3: led.StartOrAddToQueue(lsqPlacePlus3); break;
                default: break;
            }
            switch(influence.goodness_minus) {
                case -1: led.StartOrAddToQueue(lsqPlaceMinus1); break;
                case -2: led.StartOrAddToQueue(lsqPlaceMinus2); break;
                case -3: led.StartOrAddToQueue(lsqPlaceMinus3); break;
                default: break;
            }
            switch(influence.green_evil) {
                case 1: led.StartOrAddToQueue(lsqGreenEvil1); break;
                case 2: led.StartOrAddToQueue(lsqGreenEvil2); break;
                case 3: led.StartOrAddToQueue(lsqGreenEvil3); break;
                default: break;
            }
            switch(influence.artifact) {
                case 1: led.StartOrAddToQueue(lsqArtifact1); break;
                case 2: led.StartOrAddToQueue(lsqArtifact2); break;
                case 3: led.StartOrAddToQueue(lsqArtifact3); break;
                default: break;
            }
            switch(influence.cyan_beast) {
                case 1: led.StartOrAddToQueue(lsqCyanBeast1); break;
                case 2: led.StartOrAddToQueue(lsqCyanBeast2); break;
                case 3: led.StartOrAddToQueue(lsqCyanBeast3); break;
                default: break;
            }
            switch(influence.searcher) {
                case 1: led.StartOrAddToQueue(lsqSearcher1); break;
                case 2: led.StartOrAddToQueue(lsqSearcher2); break;
                case 3: led.StartOrAddToQueue(lsqSearcher3); break;
                default: break;
            }
            led.StartOrAddToQueue(lsqMaster_inwork);
            break;

        case DevType::PlaceMinus1: led.StartOrAddToQueue(lsqPlaceMinus1_inwork); break;
        case DevType::PlaceMinus2: led.StartOrAddToQueue(lsqPlaceMinus2_inwork); break;
        case DevType::PlaceMinus3: led.StartOrAddToQueue(lsqPlaceMinus3_inwork); break;

        case DevType::Artifact: led.StartOrAddToQueue(lsqArtifact_inwork); break;

        case DevType::Beast: {
            Beast::State state = Beast::GetState();
            switch(state) {
                case Beast::State::Calm:
                    led.StartOrAddToQueue(lsqBeast1_inwork);
                    break;
                case Beast::State::Worry:
                    led.StartOrAddToQueue(lsqBeast2_inwork);
                    if(time_s % 60 == 0) vibro.StartOrAddToQueue(vsqBrr); // Every minute
                    break;
                case Beast::State::Thrill:
                    led.StartOrAddToQueue(lsqBeast3_inwork);
                    if(time_s % 30 == 0) vibro.StartOrAddToQueue(vsqBrrBrr); // Every 30 seconds
                    break;
                case Beast::State::Hunger:
                    led.StartOrAddToQueue(lsqBeast4_inwork);
                    vibro.StartOrAddToQueue(vsqBrrBrrBrr);
                    break;
                case Beast::State::Madness:
                    led.StartOrAddToQueue(lsqBeastMadness);
                    break;
            } // switch state
        } break;

        case DevType::PlaceMinus1Magic: led.StartOrAddToQueue(lsqPlaceMinus1Magic_inwork); break;
        case DevType::PlaceMinus2Magic: led.StartOrAddToQueue(lsqPlaceMinus2Magic_inwork); break;
        case DevType::PlaceMinus3Magic: led.StartOrAddToQueue(lsqPlaceMinus3Magic_inwork); break;

        case DevType::Particle: IndicateGoodness(); break;

        case DevType::Path:
            if(influence.searcher > 0) led.StartOrAddToQueue(lsqPathFadeIn);
            else led.StartOrAddToQueue(lsqPathFadeOut);
            break;
    } // switch
}


void Reset() {
    time_s = 0;
    Particle::goodness = Particle::kDefault;
    Beast::resource = Beast::kDefault;
    modifier.Reset();
    tx_params.Reset();
    influence.Reset();
    g_trans_list.Reset();
    ShowSelfType();
}

namespace App {

// Load type, state and tx power from EEPROM. Called at pwr on from main of FD.
void LoadDevtypeAndStateFromEE() {
    int32_t v = EE::ReadI32(kEEAddrType);
    if(IsDevTypeValid(v)) cfg.type = static_cast<DevType>(v);
    else {
        cfg.type = DevType::Particle;
        Printf("Bad EE dev type: %u; using default\r", v);
    }
    Reset();
    EELoadState();
    v = EE::ReadI32(kEEAddrTxPwr);
    if(v >= kPwrTable[0] and v <= kPwrTable[11]) cfg.tx_power = v;
    else cfg.tx_power = CC_PwrMinus10dBm;
    PrintState();
}

void SetAndSaveTxPwr(uint8_t tx_pwr) {
    cfg.tx_power = tx_pwr;
    EESaveTxPwr();
    cfg.PrintTxPwr();
    // Indication
    switch(cfg.tx_power) {
        case CC_PwrMinus15dBm: led.StartOrAddToQueue(lsqTxPwrM15); break;
        case CC_PwrMinus10dBm: led.StartOrAddToQueue(lsqTxPwrM10); break;
        case CC_PwrMinus6dBm:  led.StartOrAddToQueue(lsqTxPwrM6);  break;
        case CC_Pwr0dBm:       led.StartOrAddToQueue(lsqTxPwr0);   break;
        case CC_PwrPlus5dBm:   led.StartOrAddToQueue(lsqTxPwrP5);  break;
        case CC_PwrPlus7dBm:   led.StartOrAddToQueue(lsqTxPwrP7);  break;
        case CC_PwrPlus10dBm:  led.StartOrAddToQueue(lsqTxPwrP10); break;
        case CC_PwrPlus12dBm:  led.StartOrAddToQueue(lsqTxPwrP12); break;
        default: break;
    } // switch
}

// Set type, reset and LOAD params. Called from SetTypeByDIP of Locket.
void SetDevtypeResetLoadState(uint32_t type32) {
    if(IsDevTypeValid(type32)) {
        cfg.type = static_cast<DevType>(type32);
        Reset();
        EELoadState();
    }
    else Printf("Invalid dev type: %u\r", type32);
}

void SetDevtypeResetSaveState(DevType type) {
    cfg.type = type;
    Reset();
    EESaveState();
    EESaveType(); // Makes sense for FD only
}

// Set type, reset and SAVE params. Called from shell.
void SetDevtypeResetSaveStateU32(uint32_t type32) {
    if(IsDevTypeValid(type32)) {
        SetDevtypeResetSaveState(static_cast<DevType>(type32));
    }
    else Printf("Invalid dev type: %u\r", type32);
}

void OnSecond() {
    time_s++;
    // Process goodness
    switch(cfg.type) {
        case DevType::Searcher:
        case DevType::Particle:
            Particle::OnSecond();
            break;
        case DevType::Beast:
            Beast::OnSecond();
            break;
        default: // Places, master, artifact, path
            break;
    } // switch
    // Indicate every 4 seconds
    if(time_s % 4 == 0) Indicate();
}

static void SignalPillIsNotApplicable() {
    Printf("Not applicable\r");
    led.StartOrAddToQueue(lsqPillBad);
    beeper.StartOrRestart(bsqBeepPillBad);
}

void ApplyPill(int32_t pill_id) {
    Printf("Pill");
    switch(pill_id) {
        #pragma region // === Reset, Goodness ===
        case 1:
            led.StartOrAddToQueue(lsqPillReset);
            Printf("Reset\r");
            Reset();
            EESaveState();
            break;
        case 2:
            led.StartOrAddToQueue(lsqPillGoodnessPlus);
            Printf("GPlus\r");
            switch(cfg.type) {
                case DevType::Particle:
                case DevType::Searcher:
                    Particle::InjectGoodness(+1200);
                    break;
                case DevType::Beast:
                    Beast::InjectGoodness(+1200);
                    break;
                default:
                    SignalPillIsNotApplicable();
                    break;
            } // switch cfg.type
            break;
        case 3:
            led.StartOrAddToQueue(lsqPillGoodnessMinus);
            Printf("GMinus\r");
            switch(cfg.type) {
                case DevType::Particle:
                case DevType::Searcher:
                    Particle::InjectGoodness(-1200);
                    break;
                case DevType::Beast:
                    Beast::InjectGoodness(-1200);
                    break;
                default:
                    SignalPillIsNotApplicable();
                    break;
            } // switch cfg.type
            break;
        case 4:
            Printf("FixForever\r");
            if(cfg.type == DevType::Particle or cfg.type == DevType::Searcher or cfg.type == DevType::Beast) {
                led.StartOrAddToQueue(lsqPillFixForever);
                modifier.fix_forever = true;
                EESaveFixForever();
            }
            else SignalPillIsNotApplicable();
            break;
        case 5:
            Printf("FixTimed\r");
            if(cfg.type == DevType::Particle or cfg.type == DevType::Searcher or cfg.type == DevType::Beast) {
                led.StartOrAddToQueue(lsqPillFixTimed);
                modifier.fix_timed = 3600;
                EESaveFixTimed();
            }
            else SignalPillIsNotApplicable();
            break;
        case 6:
            led.StartOrAddToQueue(lsqPillDisableFix);
            Printf("DisableFix\r");
            modifier.Reset();
            EESaveFixTimed();
            EESaveFixForever();
            break;
        #pragma endregion

        #pragma region // === Type switch ===
        case  7: Printf("SetType: Particle\r"); SetDevtypeResetSaveState(DevType::Particle); break;
        case  8: Printf("SetType: Searcher\r"); SetDevtypeResetSaveState(DevType::Searcher); break;
        case  9: Printf("SetType: Beast\r");    SetDevtypeResetSaveState(DevType::Beast);    break;
        case 10: Printf("SetType: Path\r");     SetDevtypeResetSaveState(DevType::Path);     break;
        case 11: Printf("SetType: PlacePlus1\r"); SetDevtypeResetSaveState(DevType::PlacePlus1); break;
        case 12: Printf("SetType: PlacePlus2\r"); SetDevtypeResetSaveState(DevType::PlacePlus2); break;
        case 13: Printf("SetType: PlacePlus3\r"); SetDevtypeResetSaveState(DevType::PlacePlus3); break;
        case 14: Printf("SetType: PlaceMinus1\r"); SetDevtypeResetSaveState(DevType::PlaceMinus1); break;
        case 15: Printf("SetType: PlaceMinus2\r"); SetDevtypeResetSaveState(DevType::PlaceMinus2); break;
        case 16: Printf("SetType: PlaceMinus3\r"); SetDevtypeResetSaveState(DevType::PlaceMinus3); break;
        case 17: Printf("SetType: PlaceMinus1Magic\r"); SetDevtypeResetSaveState(DevType::PlaceMinus1Magic); break;
        case 18: Printf("SetType: PlaceMinus2Magic\r"); SetDevtypeResetSaveState(DevType::PlaceMinus2Magic); break;
        case 19: Printf("SetType: PlaceMinus3Magic\r"); SetDevtypeResetSaveState(DevType::PlaceMinus3Magic); break;
        #pragma endregion

        #pragma region // === Set Tx Pwr === Indication inside.
        case 20: Printf("SetTxPwr: -15dBm\r"); SetAndSaveTxPwr(CC_PwrMinus15dBm); break;
        case 21: Printf("SetTxPwr: -10dBm\r"); SetAndSaveTxPwr(CC_PwrMinus10dBm); break;
        case 22: Printf("SetTxPwr: -6dBm\r");  SetAndSaveTxPwr(CC_PwrMinus6dBm);  break;
        case 23: Printf("SetTxPwr:  0dBm\r");  SetAndSaveTxPwr(CC_Pwr0dBm);       break;
        case 24: Printf("SetTxPwr: +5dBm\r");  SetAndSaveTxPwr(CC_PwrPlus5dBm);   break;
        case 25: Printf("SetTxPwr: +7dBm\r");  SetAndSaveTxPwr(CC_PwrPlus7dBm);   break;
        case 26: Printf("SetTxPwr: +10dBm\r"); SetAndSaveTxPwr(CC_PwrPlus10dBm);  break;
        case 27: Printf("SetTxPwr: +12dBm\r"); SetAndSaveTxPwr(CC_PwrPlus12dBm);  break;
        #pragma endregion
        default:
            Printf("Bad: %d\r", pill_id);
            led.StartOrAddToQueue(lsqPillBad);
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
        // pkt.Print();
        // Goodness: add it even if its value is zero, because who cares?
        if(pkt.single_transaction) { // Master's whim
            if(g_trans_list.ProcessId(pkt.id) == retv::New) { // New whim from this ID in the last minute
                switch(cfg.type) {
                    case DevType::Particle:
                    case DevType::Searcher:
                        if(!modifier.IsFixed()) Particle::InjectGoodness(pkt.goodness);
                        break;
                    case DevType::Beast:
                        // Beast is only affected by positive values of the master's whim, when Beast's rsr is positive
                        if(!modifier.IsFixed() and pkt.goodness > 0 and Beast::resource > 0)
                            Beast::InjectGoodness(pkt.goodness * 2L);
                        break;
                    default: break;
                } // switch
            }
        }
        else { // Not a single transaction, just field
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
        case DevType::Beast: {
            Beast::State state = Beast::GetState();
            if(state == Beast::State::Hunger or state == Beast::State::Madness) ppkt->cyan_beast = 1;
        } break;

        case DevType::PlaceMinus1Magic: ppkt->goodness = -1; ppkt->green_evil = 1; break;
        case DevType::PlaceMinus2Magic: ppkt->goodness = -2; ppkt->green_evil = 1; break;
        case DevType::PlaceMinus3Magic: ppkt->goodness = -3; ppkt->green_evil = 1; break;

        default: return false; // Impossible to get here, but just in case
    } // switch
    return true;
}
#pragma endregion

void PrintState() {
    RetvValU32 r = DevTypeToIndx(cfg.type);
    if(r.NotOk()) { Printf("Bad Type: %u\r", cfg.type); return; }
    Printf("DevType: %s\r", dev_id_names[*r].name);
    Printf("Goodness: %d\r", Particle::goodness);
    Printf("BeastRsrc: %d\r", Beast::resource);
    modifier.Print();
    influence.Print();
    g_trans_list.Print();
    cfg.PrintTxPwr();
}

void SetGoodness(int32_t agoodness) { Particle::goodness = agoodness; }
void SetBeastRsrc(int32_t rsrc) { Beast::resource = rsrc; }

} // namespace App