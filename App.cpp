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

// static bool IsBatteryLow() {
//     return iVbat < Battery::kLowVoltageAlkaline3v0_mV;
// }

namespace App {

void TakeBatteryVoltage(uint32_t vbat) {
    if(iVbat == 0UL) Printf("Battery: %d mV\r", vbat);
    iVbat = vbat;
}

void ShowSelfState() {
    // if(cfg.is_master) led.StartOrAddToQueue(lsqSelfTypeMaster);
    // else led.StartOrAddToQueue(lsqSelfTypePlayer);
}

// Just indicate btnpress
void OnBtnEvt(BtnEvtInfo btn_info) {
    // switch(btn_info.btn_indx) {
    //     case 0: // Open
    //         vibro.StartOrAddToQueue(vsqBrrBrr);
    //         break;
    //     case 1: // Restore
    //         if(cfg.is_master) vibro.StartOrAddToQueue(vsqBrr);
    //         break;
    //     case 2: // Close
    //         vibro.StartOrAddToQueue(vsqBrrBrrBrr);
    //         break;
    //     default: break;
    // } // switch
}

void OnSecondEvt() {
    // Nothing here
}

#pragma region // ==== Radio related ====
static bool rx_pkt_printing = false;

// RX. Called from main thread by evt which is periodically sent by radio
void ProcessRxTbl(RxTable &tbl) {
    // === Analyze table ===
    // for(uint32_t i=0; i<tbl.cnt; i++) {
    //     rPkt &pkt = tbl[i];
    //     // if(rx_pkt_printing) pkt.Print();
    //     DevType type = pkt.GetType();
    // }
    // ==== Indicate depending on self type ====
    // Show changed points only when the button is pressed
    // if(cfg.type != DevType::Idle) {
    //     if(!cfg.is_master) {
    //         switch(changed_cnt) {
    //             case 0: break;
    //             case 1:  led.StartOrAddToQueue(lsqChangedOne);  break;
    //             case 2:  led.StartOrAddToQueue(lsqChangedTwo);  break;
    //             default: led.StartOrAddToQueue(lsqChangedMany); break;
    //         }
    //     }
    //     if(changed_cnt != 0) vibro.StartOrAddToQueue(vsqLongBrr);
    // }

    // Always show active points
    // switch(active_cnt) {
    //     case 0: break;
    //     case 1: led.StartOrAddToQueue(lsqActiveOne); break;
    //     case 2: led.StartOrAddToQueue(lsqActiveTwo); break;
    //     default: led.StartOrAddToQueue(lsqActiveMany); break;
    // }

    // Vibrate if idle
    // if(active_cnt > 0 and cfg.type == DevType::Idle) vibro.StartOrRestart(vsqBrr);


    // Show discharged
    // if(IsBatteryLow()) led.StartOrAddToQueue(lsqDischarged);
    // Present self
    // ShowSelfType();
}

// Tx. Called from radio level
rPkt* PrepareTxPkt() {
    // Check which btn is pressed. If no btn is pressed, no need to transmit.
    // if     (GetBtnState(0) == BTN_HOLDDOWN_STATE) cfg.type = DevType::Opener;
    // else if(GetBtnState(1) == BTN_HOLDDOWN_STATE and cfg.is_master) cfg.type = DevType::Restorer;
    // else if(GetBtnState(2) == BTN_HOLDDOWN_STATE) cfg.type = DevType::Closer;
    // else {
    //     cfg.type = DevType::Idle;
    //     return nullptr;
    // }
    // Something is pressed
    pkt_tx.id = lkt.id;
    // pkt_tx.type = static_cast<uint8_t>(cfg.type);
    // pkt_tx.is_master = cfg.is_master? 1 : 0;
    // pkt_tx.silt = Random::Generate(0, 0xFF);
    return &pkt_tx;
}
#pragma endregion


void OnCmd(Shell *pshell) {
    Cmd_t *pcmd = &pshell->cmd;
    if(pcmd->NameIs("State")) {
        // Printf("Immortals: %d\r", immortals_cnt);
        // Printf("Preimmortals: %d\r", preimmortals_cnt);
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