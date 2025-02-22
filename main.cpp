#include "board.h"
#include "led.h"
#include "vibro.h"
#include "Sequences.h"
#include "kl_lib.h"
#include "radio_lvl1.h"
#include "App.h"

#pragma region // ======================== Variables and defines ========================
// Forever
extern const char *kBuildTime, *kBuildCfgName;
EvtMsgQ_t<EvtMsg_t, MAIN_EVT_Q_LEN> evt_q_main;
static const UartParams_t CmdUartParams(115200, CMD_UART_PARAMS);
CmdUart_t Uart { &CmdUartParams };
static void ITask();
static void OnCmd(Shell_t *PShell);
static retv ReadAndSetupMode();
// EEAddresses
#define EE_ADDR_DEVICE_ID       0
static const PinInputSetup_t DipSwPin[DIP_SW_CNT] = { DIP_SW8, DIP_SW7, DIP_SW6, DIP_SW5, DIP_SW4, DIP_SW3, DIP_SW2, DIP_SW1 };
static uint8_t GetDipSwitch();
static retv ISetID(int32_t NewID);
static void ReadIDfromEE();

LedRGBwPower_t<3> Led { LED_R_PIN, LED_G_PIN, LED_B_PIN, LED_EN_PIN };
Vibro_t<3> Vibro { VIBRO_SETUP };

static TmrKL_t TmrEverySecond {TIME_MS2I(1000), EvtId::EverySecond, tktPeriodic};

void SleepNow(uint32_t Delay) {
    chSysLock();
    Iwdg::InitAndStart(Delay);
    Sleep::EnterStandby();
    chSysUnlock();
}
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
    Uart.Init();
    ReadIDfromEE();
    Printf("\r%S %S; ID: %u\r", APP_NAME, kBuildTime, cfg.id);
    Clk.PrintFreqs();

    Led.Init();
    Vibro.Init();

    if(RadioInit() == retv::Ok) Vibro.StartOrRestart(vsqBrrBrr);
    else {
        Led.StartOrRestart(lsqFailure);
        chThdSleepMilliseconds(1008);
    }

    ReadAndSetupMode();
    TmrEverySecond.StartOrRestart();
    SimpleSensors::Init();

    // Main cycle
    ITask();
}

__noreturn
void ITask() {
    while(true) {
        EvtMsg_t msg = evt_q_main.Fetch(TIME_INFINITE);
        switch(msg.id) {
            case EvtId::EverySecond:
                if(ReadAndSetupMode() == retv::New) chThdSleepMilliseconds(810);
                break;

            case EvtId::CheckRxTable: ProcessRxTbl(*(RxTable*)msg.ptr); break;

#if BUTTONS_ENABLED
        case EvtId::Buttons:
            Printf("Btn %u %u\r", msg.btn_info.btn_indx, msg.btn_info.type);
            OnBtnPress(msg.btn_info);
            break;
#endif
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

__unused
retv ReadAndSetupMode() {
    static uint32_t OldDipSettings = 0xFFFF;
    uint32_t dw = GetDipSwitch();
    if(dw == OldDipSettings) return retv::NoChanges;
    // Something has changed
    Printf("Dip: 0x%02X; ", dw);
    OldDipSettings = dw;
    // Select power
    uint32_t bits = dw & 0b1111; // Remove high bits = group 5678
    cfg.tx_power = (bits > 11) ? CC_PwrPlus12dBm : PwrTable[bits];
    // Select dev type: group 5678
    SetDevtype((dw >> 4) & 0b1111UL);
    cfg.PrintTxPwr();
    return retv::New;
}

#if 1 // ================= Command processing ====================
void OnCmd(Shell_t *PShell) {
    Cmd_t *PCmd = &PShell->Cmd;
// Handle command
    if(PCmd->NameIs("Ping"))
        PShell->Ok();
    else if(PCmd->NameIs("Version"))
        PShell->Print("%S %S\r", APP_NAME, XSTRINGIFY(BUILD_TIME));
//    else if(PCmd->NameIs("GetID"))
//        PShell->Print("ID: %u\r", Cfg.ID);
#if ADC_REQUIRED
else if(PCmd->NameIs("GetBat")) Adc.StartMeasurement();
#endif

    else if(PCmd->NameIs("SetID")) {
        int32_t new_id = 0;
        if(PCmd->GetNext<int32_t>(&new_id) != retv::Ok) {
            PShell->CmdError();
            return;
        }
        if(ISetID(new_id) == retv::Ok) PShell->Ok();
        else PShell->Failure();
    }

#if PILL_ENABLED // ==== Pills ====
else if(PCmd->NameIs("PillRead32")) {
    int32_t Cnt = 0;
    if(PCmd->GetNextInt32(&Cnt) != OK) { PShell->Ack(CMD_ERROR); return; }
    uint8_t MemAddr = 0, b = OK;
    PShell->Printf("#PillData32 ");
    for(int32_t i=0; i<Cnt; i++) {
        b = PillMgr.Read(MemAddr, &dw32, 4);
        if(b != OK) break;
        PShell->Printf("%d ", dw32);
        MemAddr += 4;
    }
    Uart.Printf("\r\n");
    PShell->Ack(b);
}

else if(PCmd->NameIs("PillWrite32")) {
    uint8_t b = CMD_ERROR;
    uint8_t MemAddr = 0;
    // Iterate data
    while(true) {
        if(PCmd->GetNextInt32(&dw32) != OK) break;
//            Uart.Printf("%X ", Data);
        b = PillMgr.Write(MemAddr, &dw32, 4);
        if(b != OK) break;
        MemAddr += 4;
    } // while
    Uart.Ack(b);
}
else if(PCmd->NameIs("Pill")) {
    if(PCmd->GetNextInt32(&dw32) != OK) { PShell->Ack(CMD_ERROR); return; }
    PillType = (PillType_t)dw32;
    App.SignalEvt(EVT_PILL_CHECK);
}
#endif

    else PShell->CmdUnknown();
}
#endif

#if 1 // =========================== ID management =============================
void ReadIDfromEE() {
    cfg.id = EE::Read32(EE_ADDR_DEVICE_ID);  // Read device ID
    if(cfg.id < Config::kIdMin or cfg.id > Config::kIdMax) {
        Printf("\rUsing default ID\r");
        cfg.id = Config::kIdDefault;
    }
}

retv ISetID(int32_t new_id) {
    if(new_id < Config::kIdMin or new_id > Config::kIdMax) return retv::BadValue;
    retv rslt = EE::Write32(EE_ADDR_DEVICE_ID, new_id);
    if(rslt == retv::Ok) {
        cfg.id = new_id;
        Printf("New ID: %u\r", new_id);
        return retv::Ok;
    }
    else {
        Printf("EE error: %u\r", rslt);
        return retv::Fail;
    }
}
#endif

// ====== DIP switch ======
uint8_t GetDipSwitch() {
    uint8_t Rslt = 0;
    for(int i = 0; i < DIP_SW_CNT; i++)
        PinSetupInput(DipSwPin[i].PGpio, DipSwPin[i].Pin,
                DipSwPin[i].PullUpDown);
    for(int i = 0; i < DIP_SW_CNT; i++) {
        if(!PinIsHi(DipSwPin[i].PGpio, DipSwPin[i].Pin)) Rslt |= (1 << i);
        PinSetupAnalog(DipSwPin[i].PGpio, DipSwPin[i].Pin);
    }
    return Rslt;
}
