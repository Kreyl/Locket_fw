#include "App.h"
#include "Sequences.h"
#include "led.h"
#include "vibro.h"
#include "ch.h"
#include "kl_lib.h"
#include "battery_consts.h"

extern LedRGBwPower_t<11> led;
extern Vibro_t<4> vibro;
Config cfg;

static uint32_t iVbat = 0UL;

static bool IsBatteryLow() {
    return iVbat < Battery::kLowVoltageAlkaline3v0_mV;
}

void Config::PrintTxPwr() {
    Printf("TxPwr: %S\r", CC_PwrToString(tx_power));
}

static void ShowSelfTypeWhenIdle() {
    if(cfg.type == DevType::Immortal) {
        if(cfg.VibroEnabled()) led.StartOrAddToQueue(lsqSelfTypeImmortal);
        else led.StartOrAddToQueue(lsqSelfTypeImmortalNoVibro);
    }
    else led.StartOrAddToQueue(lsqSelfTypePreimmortal);
}

static void SetupAndShowBrightness() {
    uint8_t v = Config::kBrtTable[cfg.brt_indx];
    lsqOneImmortal  [0].Color.B = v;
    lsqTwoImmortals [0].Color.B = v;
    lsqManyImmortals[0].Color.B = v;
    lsqPreImmortal  [0].Color.G = v;
    lsqDischarged   [0].Color.R = v;
    led.StartOrAddToQueue(lsqOneImmortal);
    ShowSelfTypeWhenIdle();
    // Printf("Brt: %d\r", v);
}

namespace App {

void SetDevtype(uint32_t type32) {
    if(type32 > 0) cfg.type = DevType::Immortal;
    else cfg.type = DevType::Preimmortal;
    cfg.PrintType();
    SetupAndShowBrightness();
}

void TakeBatteryVoltage(uint32_t vbat) {
    if(iVbat == 0UL) Printf("Battery: %d mV\r", vbat);
    iVbat = vbat;
}

void OnBtnEvt(BtnEvtInfo_t btn_info) {
    switch(btn_info.btn_indx) {
        case 0: // Vibro on/off
            if(cfg.VibroEnabled()) cfg.DisableVibro();
            else cfg.EnableVibro();
            ShowSelfTypeWhenIdle(); // Indicate vibro state
            break;
        case 1: // Increase brt
            if(cfg.brt_indx < Config::kBrtCnt - 1) cfg.brt_indx++;
            SetupAndShowBrightness();
            break;
        case 2: // Decrease brt
            if(cfg.brt_indx > 0) cfg.brt_indx--;
            SetupAndShowBrightness();
            break;
        default: break;
    } // switch
}

void OnSecondEvt() {
    if(cfg.novibro_time_left_s > 0) cfg.novibro_time_left_s--;
}

#pragma region // ==== Radio related ====
static bool rx_pkt_printing = false;
inline constexpr const uint8_t kImmortal = 18, kPreImmortal = 99;
static uint32_t immortals_cnt = 0, preimmortals_cnt = 0;

// RX. Called from main thread by evt which is periodically sent by radio
void ProcessRxTbl(RxTable &tbl) {
    if(cfg.type == DevType::Immortal) { // Only immortals can feel
        // === Analyze table ===
        uint32_t iimmortals_cnt = 0, ipreimmortals_cnt = 0;
        for(uint32_t i=0; i<tbl.cnt; i++) {
            rPkt &pkt = tbl[i];
            if(rx_pkt_printing) pkt.Print();
            if(pkt.IsImmortal == kImmortal) iimmortals_cnt++;
            else if(pkt.IsImmortal == kPreImmortal) ipreimmortals_cnt++;
        }
        immortals_cnt = iimmortals_cnt;
        preimmortals_cnt = ipreimmortals_cnt;
        // ==== Indicate ====
        // Present immortals
        switch(iimmortals_cnt) {
            case 0:  break; // Noone near
            case 1:  led.StartOrAddToQueue(lsqOneImmortal);   break;
            case 2:  led.StartOrAddToQueue(lsqTwoImmortals);  break;
            default: led.StartOrAddToQueue(lsqManyImmortals); break;
        } // switch
        if(cfg.VibroEnabled() and iimmortals_cnt > 0) vibro.StartOrAddToQueue(vsqBrr);
        // Present preimmortals
        if(ipreimmortals_cnt > 0) {
            led.StartOrAddToQueue(lsqPreImmortal);
            if(cfg.VibroEnabled()) vibro.StartOrAddToQueue(vsqBrrBrr);
        }
    }
    // Show discharged
    if(IsBatteryLow()) led.StartOrAddToQueue(lsqDischarged);
    // Present self
    ShowSelfTypeWhenIdle();
}

// Tx. Called from radio level
void PrepareTxPkt(rPkt *ppkt) {
    ppkt->id = cfg.id;
    ppkt->IsImmortal = cfg.type == DevType::Immortal? kImmortal : kPreImmortal;
    ppkt->silt = Random::Generate(0, 0xFFFF);
}
#pragma endregion


void OnCmd(Shell *pshell) {
    Cmd_t *pcmd = &pshell->cmd;
    if(pcmd->NameIs("State")) {
        Printf("Immortals: %d\r", immortals_cnt);
        Printf("Preimmortals: %d\r", preimmortals_cnt);
        Printf("Battery: %d\r", iVbat);
        cfg.PrintTxPwr();
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