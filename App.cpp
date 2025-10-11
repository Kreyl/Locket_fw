#include "App.h"
#include "Sequences.h"
#include "led.h"
#include "vibro.h"
#include "ch.h"
#include "kl_lib.h"
#include "battery_consts.h"
#include <unordered_map>
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
    bool locket = false;
    uint32_t mengirs = 0, wormholes = 0;
    bool speaks_with_wormhole = false, vote_accepted = false, time_to_die = false;

    void Reset() {
        locket = false;
        mengirs = 0;
        wormholes = 0;
        speaks_with_wormhole = false;
        vote_accepted = false;
        time_to_die = false;
    }

    void Print() {
        Printf("Near: locket=%d mengirs=%d wormholes=%d speaksww=%u vote_accepted=%d time_to_die=%d\r",
            locket, mengirs, wormholes, speaks_with_wormhole, vote_accepted, time_to_die);
    }
} // namespace Near

enum Btn { A=0, Mid=1, B=2 };

static bool IsBatteryLow() {
    return iVbat < Battery::kLowVoltageAlkaline3v0_mV;
}

namespace App {

void TakeBatteryVoltage(uint32_t vbat) {
    if(iVbat == 0UL) Printf("Battery: %d mV\r", vbat);
    iVbat = vbat;
}

void PresentSelf() {
    if(lkt.state == Locket::Sta::Level1) led.StartOrAddToQueue(lsqLvl1);
    else if(lkt.state == Locket::Sta::Level2) led.StartOrAddToQueue(lsqLvl2);
}


// Just indicate btnpress
void OnBtnEvt(BtnEvtInfo btn_info) {
    if(lkt.state == Locket::Sta::Dead) return;
    if(btn_info.btn_indx == Btn::Mid) {
        if(Near::locket) led.StartOrAddToQueue(lsqLocketIsNear);
    }
    PresentSelf();
}

void OnSecondEvt() {
    // Nothing here
}

void ApplyPill(int32_t pill_id) {
    Printf("Pill %u\n", pill_id);
    switch(pill_id) {
        case 0:
            led.StartOrRestart(lsqDieNow);
            lkt.state = Locket::Sta::Dead;
            WriteStateToEE();
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

// RX. Called from main thread by evt which is periodically sent by radio
void ProcessRxTbl(RxTable &tbl) {
    Near::Reset();
    if(lkt.state == Locket::Sta::Dead) {
        led.StartOrRestart(lsqDead);
        return;
    }
    // === Analyze table ===
    for(uint32_t i=0; i<tbl.cnt; i++) {
        rPkt &pkt = tbl[i];
        // if(rx_pkt_printing) pkt.Print();
        Printf("id=%u\n", pkt.id);

        DevType type = pkt.GetType();
        if(type == DevType::Locket and pkt.locket.state != Locket::Sta::Dead) Near::locket = true;
        else if(type == DevType::Mengir) Near::mengirs++;
        else if(type == DevType::Wormhole) {
            Near::wormholes++;
            pkt.PrintWormhole("WH ");
            InListVoted r = pkt.wormhole.CheckID(lkt.id);
            if(r.is_in_list) {
                Near::speaks_with_wormhole = true; // Registered or voting or dying
                Near::vote_accepted = r.is_voted;
                Near::time_to_die = pkt.wormhole.cmd == WormholeCmd::KillThemAll;
            };
        }
    }
    // ==== Indicate ====
    if(Near::time_to_die) {
        lkt.state = Locket::Sta::Dead;
        led.StartOrRestart(lsqDieNow);
        vibro.StartOrRestart(vsqDieNow);
    }
    else { // It's good to be alive
        // Ignore wormholes and mengirs if speaking with a wormhole
        if(!Near::speaks_with_wormhole) {
            for(uint32_t i=0; i<Near::mengirs;   i++) vibro.StartOrAddToQueue(vsqBrr);
            for(uint32_t i=0; i<Near::wormholes; i++) vibro.StartOrAddToQueue(vsqBrrBrr);
        }
        if(Near::vote_accepted) led.StartOrAddToQueue(lsqVoteAccepted);
        // Show discharged
        if(IsBatteryLow()) led.StartOrAddToQueue(lsqDischarged);
        PresentSelf();
    }
}

// Tx. Called from radio level
rPkt* PrepareTxPkt() {
    pkt_tx.id = lkt.id;
    pkt_tx.locket.state = lkt.state;
    // Check which btn is pressed
    pkt_tx.locket.btnA_pressed = GetBtnState(Btn::A) == BTN_HOLDDOWN_STATE;
    pkt_tx.locket.btnB_pressed = GetBtnState(Btn::B) == BTN_HOLDDOWN_STATE;
    pkt_tx.locket.btn_middle_pressed = GetBtnState(Btn::Mid) == BTN_HOLDDOWN_STATE;
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