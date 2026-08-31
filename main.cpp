#include "board.h"
#include "led.h"
#include "vibro.h"
#include "kl_lib.h"
#include "beeper.h"
#include "adcL151.h"
#include "Sequences.h"
#include "radio_lvl1.h"
#include "app_types.h"


#pragma region // ======================== Variables and defines ========================
// Forever
extern const char *kBuildTime, *kBuildCfgName;
EvtMsgQ_t<EvtMsg_t, MAIN_EVT_Q_LEN> evt_q_main;
static const UartParams_t kCmdUartParams(115200, CMD_UART_PARAMS);
CmdUart uart { &kCmdUartParams };
[[noreturn]] static void ITask();
static void OnCmd(Shell *pshell);

uint32_t self_id;
DevType self_type;
uint8_t tx_pwr = CC_PwrMinus15dBm;
RxTable rx_table;
rPkt pkt_tx_;

// DIP switch
static const PinInputSetup_t dip_sw_pin[DIP_SW_CNT] = { DIP_SW8, DIP_SW7, DIP_SW6, DIP_SW5, DIP_SW4, DIP_SW3, DIP_SW2, DIP_SW1 };
static uint8_t GetDipSwitch();
static retv ReadModeFromDip();

LedRGBwPower_t<4> led { LED_R_PIN, LED_G_PIN, LED_B_PIN, LED_EN_PIN };
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

// Use watchdog to reset
void Reboot() {
    Iwdg::InitAndStart(270);
    __disable_irq();
    while(true);
}
#pragma endregion

void main(void) {
    Iwdg::InitAndStart(2700);
    // ==== Init Vcore & clock system ====
    SetupVCore(vcore1V2);
    Clk.SetMSI4MHz();
    Clk.EnableHSI(); // For ADC
    Clk.UpdateFreqValues();

    // === Init OS ===
    halInit();
    chSysInit();
    evt_q_main.Init();

    self_id = GetUniqID32(); // Calculate self uniq ID

    // ==== Init hardware ====
    uart.Init();
    Printf("\r%S %S ID=%08X\r", APP_NAME, kBuildTime, self_id);
    Clk.PrintFreqs();

    Random::Seed(self_id);
    led.Init();
    vibro.Init();
    Adc::Init(); // Battery measurement
    // beeper.Init();

    if(Radio::Init().IsOk()) {
        led.StartOrRestart(lsqStart);
        vibro.StartOrRestart(vsqBrrBrr);
    }
    else led.StartOrRestart(lsqFailure);
    chThdSleepMilliseconds(1008);

    // Read dev type and tx pwr from dip, and load state
    ReadModeFromDip();
    tmr_every_second.StartOrRestart();
    tmr_check_uart.StartOrRestart();
    // SimpleSensors::Init();

    // Main cycle
    ITask();
}

[[noreturn]]
void ITask() {
    while(true) {
        EvtMsg_t msg = evt_q_main.Fetch(TIME_INFINITE);
        switch(msg.id) {
            case EvtId::UartCheckTime:
                Iwdg::Reload();
                while(uart.TryParseRxBuff() == retv::Ok) { OnCmd((Shell*)&uart); }
                break;

            case EvtId::EverySecond:
                if(ReadModeFromDip() == retv::New) chThdSleepMilliseconds(810);
                Adc::StartMeasurement();
                break;

            case EvtId::Buttons:
                break;

            case EvtId::CheckRxTable:
                // Printf("RxTable: 0x%X\r", msg.ptr);
                ProcessRxTable();
                break;

#if ADC_REQUIRED
            case EvtId::AdcRslt: {
                uint32_t vd = Adc::GetResultMedian(0);
                uint32_t vbat = Adc::GetVDAmV(vd);
                Printf("Battery: %u mV\r", vbat);
            }
            break;
#endif

            default:
                Printf("Unhandled msg %u\r", msg.id);
                break;
        } // Switch
    } // while true
} // ITask()

#pragma region // ================= App =================
bool GetPktToTx(rPkt &apkt) {
    pkt_tx_.sender_id = self_id;
    pkt_tx_.sender_type = self_type;
    apkt = pkt_tx_;
    return true; // Always
}

void ProcessRxTable() {
    bool do_light = false;
    bool do_vibro = false;
    // Iterate received packets
    for(auto& item : rx_table) {
        const rPkt &pkt = item.pkt;
        // Player must react on places
        if(self_type == DevType::Player and pkt.sender_type == DevType::Place) {
            Printf("Place: 0x%X\r", pkt.sender_id);
            do_light = true;
            do_vibro = true;
            break; // no need to iterate more
        }
        // Place must react on players
        if(self_type == DevType::Place and pkt.sender_type == DevType::Player) {
            Printf("Player: 0x%X\r", pkt.sender_id);
            do_light = true;
            break; // no need to iterate more
        }
    } // for
    rx_table.Tick();
    // React if needed
    if(do_light) led.StartOrContinue(lsqRed);
    if(do_vibro) vibro.StartOrContinue(vsqBrr);
}
#pragma endregion

#if 1 // ================= Command processing ====================
void OnCmd(Shell *pshell) {
    Cmd_t *pcmd = &pshell->cmd;
    // Handle command
    if(pcmd->NameIs("Ping")) pshell->Ok();
    else if(pcmd->NameIs("Version")) pshell->Print("%S %S\r", APP_NAME, kBuildTime);
    else if(pcmd->NameIs("Reboot")) Reboot();
}
#endif

retv ReadModeFromDip() {
    static uint32_t old_dip = 0xFFFF;
    uint32_t new_dip = GetDipSwitch();
    if(new_dip == old_dip) return retv::NoChanges;
    // Something has changed
    Printf("Dip: 0x%02X; ", new_dip);
    old_dip = new_dip;
    // Select device type
    self_type = (new_dip & 0x80)? DevType::Player : DevType::Place;
    if(self_type == DevType::Player) led.StartOrRestart(lsqBlue);
    else led.StartOrRestart(lsqGreen);
    // Select power
    uint32_t bits = new_dip & 0b1111; // Remove high bits = group 5678
    tx_pwr = (bits > 11) ? CC_PwrPlus12dBm : kPwrTable[bits];
    Printf("TxPwr: %S\r", CC_PwrToString(tx_pwr));
    return retv::New;
}

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
