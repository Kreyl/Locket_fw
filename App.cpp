#include "App.h"
#include "Sequences.h"
#include "led.h"
#include "vibro.h"
#include "ch.h"
#include "kl_lib.h"
#include "battery_consts.h"
#include <unordered_map>

extern LedRGBwPower_t<11> led;
extern Vibro_t<4> vibro;
Locket lkt;
static rPkt pkt_tx;
uint8_t tx_power;

static uint32_t iVbat = 0UL;

namespace Near {
    bool locket = false;
    uint32_t mengirs = 0, wormholes = 0;
    bool vote_accepted = false, time_to_die = false;

    void Reset() {
        locket = false;
        mengirs = 0;
        wormholes = 0;
        vote_accepted = false;
        time_to_die = false;
    }

    void Print() {
        Printf("Near: locket=%d mengirs=%d wormholes=%d vote_accepted=%d time_to_die=%d\r",
            locket, mengirs, wormholes, vote_accepted, time_to_die);
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
    if(lkt.state == Locket::Sta::Level1) led.StartOrRestart(lsqLvl1);
    else if(lkt.state == Locket::Sta::Level2) led.StartOrRestart(lsqLvl2);
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
        DevType type = pkt.GetType();
        if(type == DevType::Locket and pkt.locket.state != Locket::Sta::Dead) Near::locket = true;
        else if(type == DevType::Mengir) Near::mengirs++;
        else if(type == DevType::Wormhole) {
            Near::wormholes++;
            // Process wormhole cmd
            Near::vote_accepted = (pkt.wormhole.cmd == WormholeCmd::VoteAccepted) and (pkt.wormhole.IdIsInList(lkt.id));
            Near::time_to_die =   (pkt.wormhole.cmd == WormholeCmd::KillThemAll)  and (pkt.wormhole.IdIsInList(lkt.id));
        }
    }
    // ==== Indicate ====
    if(Near::time_to_die) {
        lkt.state = Locket::Sta::Dead;
        led.StartOrRestart(lsqDieNow);
        vibro.StartOrRestart(vsqDieNow);
        return;
    }
    // It's good to be alive
    for(uint32_t i=0; i<Near::mengirs; i++) vibro.StartOrAddToQueue(vsqBrr);
    for(uint32_t i=0; i<Near::wormholes; i++) vibro.StartOrAddToQueue(vsqBrrBrr);
    if(Near::vote_accepted) led.StartOrAddToQueue(lsqVoteAccepted);
    // Show discharged
    if(IsBatteryLow()) led.StartOrAddToQueue(lsqDischarged);
    PresentSelf();
}

// Tx. Called from radio level
rPkt* PrepareTxPkt() {
    pkt_tx.id = lkt.id;
    pkt_tx.locket.state = lkt.state;
    // Check which btn is pressed
    pkt_tx.locket.btnA_pressed = GetBtnState(Btn::A) == BTN_HOLDDOWN_STATE;
    pkt_tx.locket.btnB_pressed = GetBtnState(Btn::B) == BTN_HOLDDOWN_STATE;
    pkt_tx.locket.btn_middle_pressed = GetBtnState(Btn::Mid) == BTN_HOLDDOWN_STATE;
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

    else pshell->CmdUnknown();
}

} // namespace App