#include "App.h"
#include "Sequences.h"
#include "led.h"
#include "vibro.h"
#include "ch.h"
#include "kl_lib.h"
#include "battery_consts.h"
#include <unordered_map>
#include <array>
#include "pill_mgr.h"


extern LedRGBwPower_t<11> led;
extern Vibro_t<4> vibro;
Locket lkt;
static rPkt pkt_tx;
uint8_t tx_power;
static uint32_t iVbat = 0UL;

// Implemented in main.cpp
void WriteStateToEE();
void ReadStateFromEE();

namespace Near {
    class AlienExists {
        private:
            int32_t counter_ = 0;
        public:
            static const int32_t kCntToReset = 5L;
            bool Exists() { return counter_ > 0; }
            void Reset() { counter_ = 0; }
            void MarkAsExisting() { counter_ = kCntToReset; }
            void Tick() { if(counter_ > 0) --counter_; }
    };

    template<int32_t kCnt>
    class AlienCnt {
        private:
            std::array<AlienExists, kCnt> aliens_;
        public:
            uint32_t GetCnt() {
                uint32_t cnt = 0;
                for(auto& alien : aliens_) {
                    if(alien.Exists()) ++cnt;
                }
                return cnt;
            }
            void MarkAsExisting(uint32_t indx) {
                if(indx < 0 or indx >= kCnt) return;
                aliens_[indx].MarkAsExisting();
            }
            void Tick() {
                for(auto& alien : aliens_) alien.Tick();
            }
            void Reset() {
                for(auto& alien : aliens_) alien.Reset();
            }
    };


    AlienExists locket;
    AlienCnt<IDs::WormholeCnt> wormholes;
    AlienCnt<IDs::MengirCnt> mengirs;
    AlienExists vote_accepted;

    bool speaks_with_wormhole = false, time_to_die = false;

    // void Reset() {
    //     locket.Reset();
    //     mengirs.Reset();
    //     wormholes = 0;
    //     speaks_with_wormhole = false;
    //     vote_accepted = false;
    //     time_to_die = false;
    // }

    void Tick() {
        locket.Tick();
        mengirs.Tick();
        wormholes.Tick();
        vote_accepted.Tick();
    }

    void Print() {
        Printf("Near: locket=%d mengirs=%d wormholes=%d speaksww=%u vote_accepted=%d time_to_die=%d\r",
            locket.Exists(), mengirs.GetCnt(), wormholes.GetCnt(), speaks_with_wormhole, vote_accepted, time_to_die);
    }
} // namespace Near

enum Btn { A=0, Mid=1, B=2 };

static bool IsBatteryLow() {
    return iVbat < Battery::kLowVoltageAlkaline3v0_mV;
}

namespace App {
Locket::Sta prev_state;
// static bool btnA_pressed = false, btn_mid_pressed = false, btnB_pressed = false;


void TakeBatteryVoltage(uint32_t vbat) {
    if(iVbat == 0UL) Printf("Battery: %d mV\r", vbat);
    iVbat = vbat;
}

void PresentSelf() {
    if(lkt.state == Locket::Sta::Level1) led.StartOrAddToQueue(lsqLvl1);
    else if(lkt.state == Locket::Sta::Level2) led.StartOrAddToQueue(lsqLvl2);
    else { // Dead
        if(prev_state == Locket::Sta::Level2) led.StartOrAddToQueue(lsqDeadLvl2);
        else led.StartOrAddToQueue(lsqDeadLvl1);
    }
}

static void DieNow() {
    prev_state = lkt.state;
    lkt.state = Locket::Sta::Dead;
    WriteStateToEE();
    led.StartOrRestart(lsqDieNow);
    vibro.StartOrRestart(vsqDieNow);
    PresentSelf();
}

void Indicate() {
    static int32_t vote_indi_cnt = 0, whm_indi_cnt = 0;
    if(Near::time_to_die) DieNow();
    else { // It's good to be alive
        // Indicate vote acceptance every 4 seconds
        if(vote_indi_cnt <= 0) {
            if(Near::vote_accepted.Exists()) {
                led.StartOrAddToQueue(lsqVoteAccepted);
                vote_indi_cnt = 3;
            }
        }
        else --vote_indi_cnt;
        // Ignore wormholes and mengirs if speaking with a wormhole
        if(!Near::speaks_with_wormhole) {
            if(whm_indi_cnt <= 0) { // indicate once a 4 ticks
                whm_indi_cnt = 3;
                int32_t wh_cnt = Near::wormholes.GetCnt();
                int32_t mengir_cnt = Near::mengirs.GetCnt();
                if(wh_cnt > 0) vibro.StartOrAddToQueue(vsqBrrBrr); // }
                if(wh_cnt > 1) vibro.StartOrAddToQueue(vsqBrrBrr); // } 1 brbr for 1 wh, 2 brbr for 2 or more whs
                if(mengir_cnt > 1) vibro.StartOrAddToQueue(vsqBrr); // }
                if(mengir_cnt > 2) vibro.StartOrAddToQueue(vsqBrr); // } 1 brr for 1 mengir, 2 brr for 2 or more mengirs
            }
            else --whm_indi_cnt;
        }
        // Show discharged
        if(IsBatteryLow()) led.StartOrAddToQueue(lsqDischarged);
        PresentSelf();
    }
}


void OnBtnEvt(BtnEvtInfo btn_info) {
    // btn_info.Print();
    if(lkt.state == Locket::Sta::Dead) return;
    if     (btn_info.btn_indx == 0) lkt.btnA_pressed = (btn_info.type == beLongPress); // else release or shortpress
    else if(btn_info.btn_indx == 2) lkt.btnB_pressed = (btn_info.type == beLongPress); // else release or shortpress
    else { // indx == 1 => btn mid
        if(btn_info.type == beShortPress) {
            lkt.btn_middle_pressed = false;
            if(Near::locket.Exists()) {
                led.StartOrAddToQueue(lsqLocketIsNear);
                PresentSelf();
            }
        }
        else lkt.btn_middle_pressed = (btn_info.type == beLongPress);
    }
}

void OnSecondEvt() {
    // Nothing here
}


void ApplyPill(int32_t pill_id) {
    Printf("Pill %u\n", pill_id);
    switch(pill_id) {
        case 0:
            DieNow();
            break;
        case 1:
            led.StartOrRestart(lsqPillLvl1);
            lkt.state = Locket::Sta::Level1;
            WriteStateToEE();
            break;
        case 2:
            led.StartOrRestart(lsqPillLvl2);
            lkt.state = Locket::Sta::Level2;
            WriteStateToEE();
            break;
        default:
            led.StartOrRestart(lsqPillBad);
            break;
    } // switch
}


#pragma region // ==== Radio related ====
static bool rx_pkt_printing = false;


void AnalyzeRxTable() {
    for(const rPkt& pkt : Radio::rx_table) {
        // Printf("id=%u\n", pkt.id);
        DevType type = pkt.GetType();
        switch(type) {
            case DevType::Locket:
                if(pkt.locket.state != Locket::Sta::Dead) Near::locket.MarkAsExisting();
                break;
            case DevType::Mengir:
                Near::mengirs.MarkAsExisting(IDs::Mengir2indx(pkt.id));
                break;
            case DevType::Wormhole: {
                Near::wormholes.MarkAsExisting(IDs::Wormhole2indx(pkt.id));
                pkt.PrintWormhole("WH ");
                InListVoted r = IDs::CheckID(lkt.id, pkt.wormhole.ids);
                if(r.is_in_list) {
                    Near::speaks_with_wormhole = true; // Registered or voting or dying
                    if(r.is_voted) Near::vote_accepted.MarkAsExisting();
                    Near::time_to_die = pkt.wormhole.cmd == WormholeCmd::KillThemAll;
                };
            } break;
            default: break;
        } // switch
    } // for
}


// RX. Called from main thread by evt which is periodically sent by radio
void ProcessRxTbl() {
    Near::Tick();
    if(lkt.state != Locket::Sta::Dead) {
        AnalyzeRxTable();
        Indicate();
    }
    Radio::rx_table.Tick();
}

// Tx. Called from radio level
rPkt* PrepareTxPkt() {
    if(lkt.state == Locket::Sta::Dead) return nullptr; // Do not make noise
    pkt_tx.id = lkt.id;
    pkt_tx.locket.state = lkt.state;
    // Check which btn is pressed
    pkt_tx.locket.btnA_pressed = lkt.btnA_pressed;
    pkt_tx.locket.btnB_pressed = lkt.btnB_pressed;
    pkt_tx.locket.btn_middle_pressed = lkt.btn_middle_pressed;
    // Printf("btns: %u %u %u\n", pkt_tx.locket.btnA_pressed, pkt_tx.locket.btnB_pressed, pkt_tx.locket.btn_middle_pressed);
    // Say we are busy speaking with wormhole to allow mengir to ignore us
    pkt_tx.locket.speaks_with_wormhole = Near::speaks_with_wormhole? 1 : 0;
    return &pkt_tx;
}
#pragma endregion


void OnCmd(Shell *pshell) {
    Cmd_t *pcmd = &pshell->cmd;
    if(pcmd->NameIs("Sta")) {
        lkt.Print();
        Near::Print();
        Printf("Battery: %d\r", iVbat);
    }

    else if(pcmd->NameIs("SetTxPwr")) {
        uint8_t tx_pwr_indx = 0;
        if(pcmd->GetNext<uint8_t>(&tx_pwr_indx).IsOk() and tx_pwr_indx <= 11) {
            tx_power = kPwrTable[tx_pwr_indx];
            Printf("TxPwr: %S\r", CC_PwrToString(tx_power));
        }
        else pshell->BadParam();
    }

    else if(pcmd->NameIs("RxPktPrinting")) {
        uint32_t v;
        rx_pkt_printing = pcmd->GetNext(&v).IsOk() and v != 0;
        Printf("RxPktPrinting: %s\r", rx_pkt_printing ? "Enabled" : "Disabled");
        pshell->Ok();
    }

    else if(pcmd->NameIs("PillRead32")) {
        uint32_t cnt = 0, dw32 = 0;
        if(pcmd->GetNext(&cnt).NotOk()) { pshell->BadParam(); return; }
        uint8_t mem_addr = 0;
        pshell->Print("#PillData32 ");
        for(uint32_t i=0; i<cnt; i++) {
            if(PillMgr::Read32(mem_addr, &dw32, 1).NotOk()) break;
            pshell->Print("%u ", dw32);
            mem_addr += 4;
        }
        pshell->PrintEOL();
        pshell->Ok();
    }

    else if(pcmd->NameIs("PillWrite32")) {
        uint32_t dw32, mem_addr = 0;
        while(true) {
            if(pcmd->GetNext(&dw32).NotOk()) break;
            Printf("%u ", dw32);
            if(PillMgr::Write32(mem_addr, &dw32, 1).NotOk()) break;
            mem_addr += 4;
        } // while
        pshell->Ok();
    }

    else if(pcmd->NameIs("ApplyPill")) {
        int32_t dw32;
        if(pcmd->GetNext(&dw32).IsOk()) {
            pshell->Ok();
            ApplyPill(dw32);
        }
        else pshell->BadParam();
    }

    else pshell->CmdUnknown();
}

} // namespace App