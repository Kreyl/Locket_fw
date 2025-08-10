#include "App.h"
#include "Sequences.h"
#include "led.h"
#include "ch.h"
#include "kl_lib.h"
#include "battery_consts.h"
#include <vector>
#include "AuPlayer.h"

extern LedRGB_t led;
Config cfg;
static rPkt pkt_tx;

static uint32_t iVbat = 0UL;

void Resume();

// static bool IsBatteryLow() {
//     return iVbat < Battery::kLowVoltageAlkaline3v0_mV;
// }

void Config::PrintType() {
    switch(type) {
        // Locket
        case DevType::Idle:     Printf("Idle\r"); break;
        case DevType::Opener:   Printf("Opener\r"); break;
        case DevType::Restorer: Printf("Restorer\r"); break;
        case DevType::Closer:   Printf("Closer\r"); break;
        // Point
        case DevType::Active:   Printf("Active\r"); break;
        case DevType::Opened:   Printf("Opened\r"); break;
        case DevType::Closed:   Printf("Closed\r"); break;
    }
    if(is_master) Printf("Master\r");
}


void Config::PrintTxPwr() {
    Printf("TxPwr: %S\r", CC_PwrToString(tx_power));
}


namespace App {

uint32_t quorum_sz = 10UL;

void TakeBatteryVoltage(uint32_t vbat) {
    if(iVbat == 0UL) Printf("Battery: %d mV\r", vbat);
    iVbat = vbat;
}


void OnSecondEvt() {
    // Nothing here
}

#pragma region // ==== Radio related ====
static bool rx_pkt_printing = false;

class Point {
public:
    uint32_t id = 0;
    DevType type = DevType::Active;
    DevType prev_type = DevType::Active;
    Point(uint32_t id) : id(id) {}
};

static std::vector<Point> points;

static void SetState(DevType commander_type) {
    DevType new_type = cfg.type;
    switch(commander_type) {
        case DevType::Opener:   new_type = DevType::Opened; break;
        case DevType::Restorer: new_type = DevType::Active; break;
        case DevType::Closer:   new_type = DevType::Closed; break;
        default: break;
    }
    if(cfg.type != new_type) {
        cfg.type = new_type;
        Resume(); // Prepare for playing
        switch(new_type) {
            case DevType::Opened:
                led.StartOrRestart(lsqOpened);
                AuPlayer.Play("opened.wav", spmSingle);
                break;
            case DevType::Active:
                led.StartOrRestart(lsqActive);
                AuPlayer.Play("alive.wav", spmSingle);
                break;
            case DevType::Closed:
                led.StartOrRestart(lsqClosed);
                AuPlayer.Play("closed.wav", spmSingle);
                break;
            default: break;
        }
    }
}

// RX. Called from main thread by evt which is periodically sent by radio
void ProcessRxTbl(RxTable &tbl) {
    // === Analyze table ===
    uint32_t opener_cnt = 0, closer_cnt = 0;
    for(uint32_t i=0; i<tbl.cnt; i++) {
        rPkt &pkt = tbl[i];
        if(rx_pkt_printing) pkt.Print();
        DevType type = static_cast<DevType>(pkt.type);
        // Obey your master
        if(pkt.is_master) {
            SetState(type);
            return;
        }
        // Proceed when no master nearby
        if(type == DevType::Opener) opener_cnt++;
        if(type == DevType::Closer) closer_cnt++;
    }
    // ==== Act ====
    if(opener_cnt >= quorum_sz and opener_cnt >= closer_cnt) SetState(DevType::Opener);
    else if(closer_cnt >= quorum_sz and closer_cnt > opener_cnt) SetState(DevType::Closer);

    // Show discharged
    // if(IsBatteryLow()) led.StartOrAddToQueue(lsqDischarged);
}

// Tx. Called from radio level
rPkt* PrepareTxPkt() {
    pkt_tx.id = cfg.id;
    pkt_tx.type = static_cast<uint8_t>(cfg.type);
    pkt_tx.is_master = 0;
    pkt_tx.silt = Random::Generate(0, 0xFF);
    return &pkt_tx;
}
#pragma endregion


void OnCmd(Shell *pshell) {
    Cmd_t *pcmd = &pshell->cmd;
    if(pcmd->NameIs("State")) {
        Printf("Battery: %d\r", iVbat);
        cfg.PrintTxPwr();
        cfg.PrintType();
    }

    else if(pcmd->NameIs("SetTxPwr")) {
        uint8_t tx_pwr_indx = 0;
        if(pcmd->GetNext<uint8_t>(&tx_pwr_indx).IsOk() and tx_pwr_indx <= 11) {
            cfg.tx_power = kPwrTable[tx_pwr_indx];
            cfg.PrintTxPwr();
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