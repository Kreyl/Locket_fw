#include "board.h"
#include "led.h"
#include "vibro.h"
#include "kl_lib.h"
#include "radio_lvl1.h"
#include "App.h"
#include "beeper.h"
#include "pill_mgr.h"
#include "adcL151.h"
#include "app_types.h"
#include "Sequences.h"

#pragma region // ======================== Variables and defines ========================
// Forever
extern const char *kBuildTime, *kBuildCfgName;
EvtMsgQ_t<EvtMsg_t, MAIN_EVT_Q_LEN> evt_q_main;
static const UartParams_t kCmdUartParams(115200, CMD_UART_PARAMS);
CmdUart uart { &kCmdUartParams };
static void ITask();
static void OnCmd(Shell *pshell);

// DIP switch
static const PinInputSetup_t dip_sw_pin[DIP_SW_CNT] = { DIP_SW8, DIP_SW7, DIP_SW6, DIP_SW5, DIP_SW4, DIP_SW3, DIP_SW2, DIP_SW1 };
static uint8_t GetDipSwitch();
static retv ReadModeFromDip();

// EE
#define EE_ADDR_DEVICE_ID       0
#define EE_ADDR_STATE           4
static retv ISetID(uint32_t new_id);
static void ReadIDfromEE();
void WriteStateToEE();
void ReadStateFromEE();

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
    ReadIDfromEE();
    ReadStateFromEE();
    Printf("\r%S %S; State=%u; ID: %u\r", APP_NAME, kBuildTime, lkt.state, lkt.id);
    Clk.PrintFreqs();

    Random::SeedWithUniqID();
    led.Init();
    vibro.Init();
    Adc::Init(); // Battery measurement
    // beeper.Init();
    PillMgr::Init();

    if(Radio::Init().IsOk()) {
        led.StartOrRestart(lsqStart);
        vibro.StartOrRestart(vsqBrrBrr);
    }
    else led.StartOrRestart(lsqFailure);
    chThdSleepMilliseconds(1008);

    // Read dev type and tx pwr from dip, and load state
    ReadModeFromDip();
    App::PresentSelf();
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

            // ==== Pill ====
            case EvtId::CheckPill: PillMgr::Check(); break;
            case EvtId::PillConnected: App::ApplyPill(PillMgr::pill_data.type); break;
            case EvtId::PillDisconnected: Printf("Pill disconnected\r"); break;

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
    tx_power = (bits > 11) ? CC_PwrPlus12dBm : kPwrTable[bits];
    Printf("TxPwr: %S\r", CC_PwrToString(tx_power));
    return retv::New;
}

#if 1 // ================= Command processing ====================
void OnCmd(Shell *pshell) {
    Cmd_t *pcmd = &pshell->cmd;
    // Handle command
    if(pcmd->NameIs("Ping")) pshell->Ok();
    else if(pcmd->NameIs("Version")) pshell->Print("%S %S\r", APP_NAME, kBuildTime);
    else if(pcmd->NameIs("SetID")) {
        uint32_t new_id = 0;
        if(pcmd->GetNext<uint32_t>(&new_id) != retv::Ok) {
            pshell->CmdError();
            return;
        }
        if(ISetID(new_id) == retv::Ok) pshell->Ok();
        else pshell->Failure();
    }

    else App::OnCmd(pshell);
}
#endif

#if 1 // =========================== EE management =============================
void ReadIDfromEE() {
    lkt.id = EE::ReadU32(EE_ADDR_DEVICE_ID);  // Read device ID
    if(lkt.id < IDs::LocketMin or lkt.id > IDs::LocketMax) {
        Printf("\rUsing default ID\r");
        lkt.id = IDs::LocketMin;
    }
}

retv ISetID(uint32_t new_id) {
    if(new_id < IDs::LocketMin or new_id > IDs::LocketMax) return retv::BadValue;
    retv rslt = EE::WriteU32(EE_ADDR_DEVICE_ID, new_id);
    if(rslt == retv::Ok) {
        lkt.id = new_id;
        Printf("New ID: %u\r", new_id);
        return retv::Ok;
    }
    else {
        Printf("EE error: %u\r", rslt);
        return retv::Fail;
    }
}

void ReadStateFromEE() {
    uint32_t state = EE::ReadU32(EE_ADDR_STATE);
    if(state > Locket::Sta::Level2) state = Locket::Sta::Dead;
    else lkt.state = (Locket::Sta)state;
}

void WriteStateToEE() {
    EE::WriteU32(EE_ADDR_STATE, lkt.state);
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
