#include "board.h"
#include "led.h"
#include "vibro.h"
#include "kl_lib.h"
#include "radio_lvl1.h"
#include "App.h"
#include "beeper.h"
#include "pill_mgr.h"
#include "adcL151.h"

#include "Sequences.h"

#pragma region // ======================== Variables and defines ========================
// Forever
extern const char *kBuildTime, *kBuildCfgName;
EvtMsgQ_t<EvtMsg_t, MAIN_EVT_Q_LEN> evt_q_main;
static const UartParams_t kCmdUartParams(115200, CMD_UART_PARAMS);
CmdUart uart { &kCmdUartParams };
static void ITask();
static void OnCmd(Shell *pshell);

static retv ReadModeFromDip();

static const PinInputSetup_t dip_sw_pin[DIP_SW_CNT] = { DIP_SW8, DIP_SW7, DIP_SW6, DIP_SW5, DIP_SW4, DIP_SW3, DIP_SW2, DIP_SW1 };
static uint8_t GetDipSwitch();

LedRGBwPower_t<11> led { LED_R_PIN, LED_G_PIN, LED_B_PIN, LED_EN_PIN };
Vibro_t<4> vibro { VIBRO_SETUP };
// Beeper_t<4> beeper { BEEPER_PIN };

static TmrKL_t tmr_every_second {TIME_MS2I(1000), EvtId::EverySecond, tktPeriodic};
static TmrKL_t tmr_check_uart {TIME_MS2I(UART_RX_POLLING_MS), EvtId::UartCheckTime, tktPeriodic};

void SleepNow(uint32_t delay) {
    chSysLock();
    Iwdg::InitAndStart(delay);
    Sleep::EnterStandby();
    chSysUnlock();
}
#pragma endregion

void main(void) {
    // ==== Init Vcore & clock system ====
    SetupVCore(vcore1V2);
    Clk.SetMSI4MHz();
    Clk.EnableHSI(); // For ADC
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
    Adc::Init(); // Battery measurement
    // beeper.Init();
    // PillMgr::Init();

    if(Radio::Init().IsOk()) led.StartOrRestart(lsqStart);
    else led.StartOrRestart(lsqFailure);
    chThdSleepMilliseconds(1008);

    // Read dev type and tx pwr from dip, and load state
    ReadModeFromDip();
    App::ShowSelfType();
    tmr_every_second.StartOrRestart();
    tmr_check_uart.StartOrRestart();
    SimpleSensors::Init();

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
                if(ReadModeFromDip() == retv::New) chThdSleepMilliseconds(810);
                Adc::StartMeasurement();
                App::OnSecondEvt();
                break;

            case EvtId::Buttons:
                // Printf("Btn %u %u\r", msg.btn_info.btn_indx, msg.btn_info.type);
                App::OnBtnEvt(msg.btn_info);
                break;

            case EvtId::CheckRxTable:
                // Printf("RxTable: 0x%X\r", msg.ptr);
                App::ProcessRxTbl(*static_cast<RxTable*>(msg.ptr));
                break;

#if ADC_REQUIRED
            case EvtId::AdcRslt: {
                uint32_t vd = Adc::GetResultMedian(0);
                uint32_t vbat = Adc::GetVDAmV(vd);
                // Printf("Battery: %u mV\r", vbat);
                App::TakeBatteryVoltage(vbat);
            }
            break;
#endif

            default:
                Printf("Unhandled msg %u\r", msg.id);
                break;
        } // Switch
    } // while true
} // ITask()


retv ReadModeFromDip() {
    static uint32_t old_dip_settings = 0xFFFF;
    uint32_t dw32 = GetDipSwitch();
    if(dw32 == old_dip_settings) return retv::NoChanges;
    // Something has changed
    Printf("Dip: 0x%02X; ", dw32);
    old_dip_settings = dw32;
    // Select power
    uint32_t bits = dw32 & 0b1111; // Remove high bits = group 5678
    cfg.tx_power = (bits > 11) ? CC_PwrPlus12dBm : kPwrTable[bits];
    // Is it master?
    cfg.is_master = (dw32 & 0x80UL) != 0;
    cfg.PrintType();
    cfg.PrintTxPwr();
    return retv::New;
}

#if 1 // ================= Command processing ====================
void OnCmd(Shell *pshell) {
    Cmd_t *pcmd = &pshell->cmd;
    // Handle command
    if(pcmd->NameIs("Ping")) pshell->Ok();
    else if(pcmd->NameIs("Version")) pshell->Print("%S %S\r", APP_NAME, kBuildTime);

    else App::OnCmd(pshell);
}
#endif


// ====== DIP switch ======
uint8_t GetDipSwitch() {
    uint8_t rslt = 0;
    for(int i = 0; i < DIP_SW_CNT; i++)
        PinSetupInput(dip_sw_pin[i].PGpio, dip_sw_pin[i].Pin, dip_sw_pin[i].PullUpDown);
    for(int i = 0; i < DIP_SW_CNT; i++) {
        if(!PinIsHi(dip_sw_pin[i].PGpio, dip_sw_pin[i].Pin)) rslt |= (1 << i);
        PinSetupAnalog(dip_sw_pin[i].PGpio, dip_sw_pin[i].Pin);
    }
    return rslt;
}
