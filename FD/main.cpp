#include "board.h"
#include "led.h"
#include "vibro.h"
#include "kl_lib.h"
#include "radio_lvl1.h"
#include "App.h"
#include "beeper.h"
#include "pill_mgr.h"

#include "Sequences.h"

#pragma region // ======================== Variables and defines ========================
// Forever
extern const char *kBuildTime, *kBuildCfgName;
EvtMsgQ_t<EvtMsg_t, MAIN_EVT_Q_LEN> evt_q_main;
static const UartParams_t kCmdUartParams(115200, CMD_UART_PARAMS);
CmdUart_t uart { &kCmdUartParams };
static void ITask();
static void OnCmd(Shell_t *pshell);

LedRGB_t<11> led { LED_R_PIN, LED_G_PIN, LED_B_PIN };
Vibro_t<4> vibro { VIBRO_SETUP };
Beeper_t<4> beeper { BEEPER_PIN };

static TmrKL_t tmr_every_second {TIME_MS2I(1000), EvtId::EverySecond, tktPeriodic};
#pragma endregion

void main(void) {
    // ==== Init Vcore & clock system ====
    SetupVCore(vcore1V2);
    Clk.SetMSI4MHz();
    Clk.UpdateFreqValues();
    // === Init OS ===
    halInit();
    chSysInit();
    evt_q_main.Init();

    // ==== Init hardware ====
    uart.Init();
    cfg.id = GetUniqID32();
    Printf("\r%S %S; ID: 0x%08X\r", APP_NAME, kBuildTime, cfg.id);
    Clk.PrintFreqs();

    Random::SeedWithUniqID();
    led.Init();
    // vibro.Init();
    beeper.Init();
    // PillMgr::Init();

    // if(Radio::Init().IsOk())
    led.StartOrRestart(lsqStart);
    // else led.StartOrRestart(lsqFailure);
    // chThdSleepMilliseconds(1008);

    // Read dev type and tx pwr from EE, and load state
    // App::LoadDevtypeAndStateFromEE();
    // tmr_every_second.StartOrRestart();

    // Main cycle
    ITask();
}

__noreturn
void ITask() {
    while(true) {
        EvtMsg_t msg = evt_q_main.Fetch(TIME_INFINITE);
        switch(msg.id) {
            case EvtId::EverySecond:
                App::OnSecond();
                break;

            case EvtId::CheckRxTable:
                App::ProcessRxTbl(*static_cast<RxTable*>(msg.ptr));
                break;

            // Pill
            case EvtId::CheckPill: PillMgr::Check(); break;
            case EvtId::PillConnected:
                Printf("Pill connected: %u\r", PillMgr::pill_data.type);
                App::ApplyPill(PillMgr::pill_data.type);
                break;
            case EvtId::PillDisconnected:
                Printf("Pill disconnected\r");
                break;

#if ADC_REQUIRED
            case evtIdAdcRslt: Printf("Battery: %u mV\r", Adc.GetVDAmV(Adc.GetResultMedian(0))); break;
#endif
            case EvtId::ShellCmd:
                OnCmd((Shell_t*) msg.ptr);
                ((Shell_t*)msg.ptr)->SignalCmdProcessed();
                break;
            default:
                Printf("Unhandled msg %u\r", msg.id);
                break;
        } // Switch
    } // while true
} // ITask()

#if 1 // ================= Command processing ====================
void OnCmd(Shell_t *pshell) {
    Cmd_t *pcmd = &pshell->Cmd;
    // Handle command
    if(pcmd->NameIs("Ping"))
        pshell->Ok();
    else if(pcmd->NameIs("Version"))
        pshell->Print("%S %S\r", APP_NAME, kBuildTime);

#if ADC_REQUIRED
else if(pcmd->NameIs("GetBat")) Adc.StartMeasurement();
#endif

    else if(pcmd->NameIs("SetType")) {
        uint32_t new_type = 0;
        if(pcmd->GetNext<uint32_t>(&new_type).IsOk()) {
            App::SetDevtypeResetSaveState(new_type);
        }
        else pshell->BadParam();
    }

    else if(pcmd->NameIs("SetTxPwr")) {
        uint8_t tx_pwr_indx = 0;
        if(pcmd->GetNext<uint8_t>(&tx_pwr_indx).IsOk()) {
            if(tx_pwr_indx <= 11) App::SetAndSaveTxPwr(kPwrTable[tx_pwr_indx]);
            else pshell->BadParam();
        }
        else pshell->BadParam();
    }

#if PILL_ENABLED // ==== Pills ====
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
    pshell->EOL();
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
        App::ApplyPill(dw32);
    }
    else pshell->BadParam();
}
#endif

    else if(pcmd->NameIs("State")) App::PrintState();

    else pshell->CmdUnknown();
}
#endif
