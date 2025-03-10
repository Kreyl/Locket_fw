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
CmdUart uart { &kCmdUartParams };
static void ITask();
static void OnCmd(Shell *pshell);

LedRGB_t<11> led { LED_R_PIN, LED_G_PIN, LED_B_PIN };
Vibro_t<4> vibro { VIBRO_SETUP };
Beeper_t<4> beeper { BEEPER_PIN };

static TmrKL_t tmr_every_second {TIME_MS2I(1000), EvtId::EverySecond, tktPeriodic};
static TmrKL_t tmr_check_uart {TIME_MS2I(UART_RX_POLLING_MS), EvtId::UartCheckTime, tktPeriodic};
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
    vibro.Init();
    beeper.Init();
    PillMgr::Init();

    if(Radio::Init().IsOk()) {
        led.StartOrRestart(lsqStart);
        vibro.StartOrRestart(vsqBrrBrr);
        beeper.StartOrRestart(bsqBeepBeep);
    }
    else led.StartOrRestart(lsqFailure);
    chThdSleepMilliseconds(1008);

    // Read dev type and tx pwr from EE, and load state
    App::LoadDevtypeAndStateFromEE();

    tmr_every_second.StartOrRestart();
    tmr_check_uart.StartOrRestart();
    // Main cycle
    ITask();
}

__noreturn
void ITask() {
    while(true) {
        EvtMsg_t msg = evt_q_main.Fetch(TIME_INFINITE);
        switch(msg.id) {
            case EvtId::UartCheckTime:
                while(uart.TryParseRxBuff() == retv::Ok) { OnCmd((Shell*)&uart); }
            break;

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

            default:
                Printf("Unhandled msg %u\r", msg.id);
                break;
        } // Switch
    } // while true
} // ITask()

#if 1 // ================= Command processing ====================
void OnCmd(Shell *pshell) {
    Cmd_t *pcmd = &pshell->cmd;
    // Handle command
    if(pcmd->NameIs("Ping"))
        pshell->Ok();
    else if(pcmd->NameIs("Version"))
        pshell->Print("%S %S\r", APP_NAME, kBuildTime);

#if ADC_REQUIRED
else if(pcmd->NameIs("GetBat")) Adc.StartMeasurement();
#endif

    else App::OnCmd(pshell);
}
#endif
