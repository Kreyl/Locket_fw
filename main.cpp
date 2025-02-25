#include "board.h"
#include "led.h"
#include "vibro.h"
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
static void OnCmd(Shell_t *pshell);
static retv ReadAndSetupMode();
// EEAddresses
#define EE_ADDR_DEVICE_ID       0
static const PinInputSetup_t DipSwPin[DIP_SW_CNT] = { DIP_SW8, DIP_SW7, DIP_SW6, DIP_SW5, DIP_SW4, DIP_SW3, DIP_SW2, DIP_SW1 };
static uint8_t GetDipSwitch();

LedRGBwPower_t<7> Led { LED_R_PIN, LED_G_PIN, LED_B_PIN, LED_EN_PIN };
Vibro_t<4> vibro { VIBRO_SETUP };

static TmrKL_t tmr_every_second {TIME_MS2I(1000), EvtId::EverySecond, tktPeriodic};

void SleepNow(uint32_t Delay) {
    chSysLock();
    Iwdg::InitAndStart(Delay);
    Sleep::EnterStandby();
    chSysUnlock();
}

static const LedRGBChunk_t lsqFailure[] = {
    {csSetup, 0, clRed},
    {csWait, 45},
    {csSetup, 0, clBlack},
    {csWait, 45},
    {csRepeat, 1},
    {csEnd}
};
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
    cfg.id = GetUniqID32();
    Printf("\r%S %S; ID: 0x%08X\r", APP_NAME, kBuildTime, cfg.id);
    Clk.PrintFreqs();

    Printf("rpkt sz: %u\r", kRPktSz);


    Random::SeedWithUniqID();
    Led.Init();
    vibro.Init();

    if(Radio::Init().NotOk()) {
        Led.StartOrRestart(lsqFailure);
        chThdSleepMilliseconds(1008);
    }

    ReadAndSetupMode();
    tmr_every_second.StartOrRestart();
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
                OnSecond();
                break;

            case EvtId::Buttons:
                Printf("Btn %u %u\r", msg.btn_info.btn_indx, msg.btn_info.type);
                OnBtnPress(msg.btn_info);
                break;

            case EvtId::CheckRxTable:
                ProcessRxTbl(static_cast<RxTable*>(msg.ptr));
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
void OnCmd(Shell_t *pshell) {
    Cmd_t *pcmd = &pshell->Cmd;
    // Handle command
    if(pcmd->NameIs("Ping"))
        pshell->Ok();
    else if(pcmd->NameIs("Version"))
        pshell->Print("%S %S\r", APP_NAME, kBuildTime);

    else if(pcmd->NameIs("GetID")) {
        // uint32_t x, y, z;
        uint32_t seed;
        if(pcmd->GetNext<uint32_t>(&seed).IsOk()) {
            char* S = pcmd->GetNextString();
            uint32_t h = HashMurmur3_32(S, strlen(S), seed);
            pshell->Print("ID: 0x%08X\r", h);
        }
        // if(pcmd->GetNext<uint32_t>(&x).IsOk() && pcmd->GetNext<uint32_t>(&y).IsOk() && pcmd->GetNext<uint32_t>(&z).IsOk())
        //     pshell->Print("ID: 0x%08X\r", GetUniqID32(x, y, z));
        else pshell->BadParam();
        // pshell->Print("ID: %u\r", Cfg.ID);
    }



#if ADC_REQUIRED
else if(pcmd->NameIs("GetBat")) Adc.StartMeasurement();
#endif

    else if(pcmd->NameIs("SetType")) {
        uint32_t new_type = 0;
        if(pcmd->GetNext<uint32_t>(&new_type).IsOk()) {
            SetDevtype(new_type);
        }
        else pshell->BadParam();
    }

    // else if(pcmd->NameIs("SetID")) {
    //     int32_t new_id = 0;
    //     if(pcmd->GetNext<int32_t>(&new_id) != retv::Ok) {
    //         pshell->CmdError();
    //         return;
    //     }
    //     if(ISetID(new_id) == retv::Ok) pshell->Ok();
    //     else pshell->Failure();
    // }

#if PILL_ENABLED // ==== Pills ====
else if(pcmd->NameIs("PillRead32")) {
    int32_t Cnt = 0;
    if(pcmd->GetNextInt32(&Cnt) != OK) { pshell->Ack(CMD_ERROR); return; }
    uint8_t MemAddr = 0, b = OK;
    pshell->Printf("#PillData32 ");
    for(int32_t i=0; i<Cnt; i++) {
        b = PillMgr.Read(MemAddr, &dw32, 4);
        if(b != OK) break;
        pshell->Printf("%d ", dw32);
        MemAddr += 4;
    }
    Uart.Printf("\r\n");
    pshell->Ack(b);
}

else if(pcmd->NameIs("PillWrite32")) {
    uint8_t b = CMD_ERROR;
    uint8_t MemAddr = 0;
    // Iterate data
    while(true) {
        if(pcmd->GetNextInt32(&dw32) != OK) break;
//            Uart.Printf("%X ", Data);
        b = PillMgr.Write(MemAddr, &dw32, 4);
        if(b != OK) break;
        MemAddr += 4;
    } // while
    Uart.Ack(b);
}
else if(pcmd->NameIs("Pill")) {
    if(pcmd->GetNextInt32(&dw32) != OK) { pshell->Ack(CMD_ERROR); return; }
    PillType = (PillType_t)dw32;
    App.SignalEvt(EVT_PILL_CHECK);
}
#endif

    else pshell->CmdUnknown();
}
#endif

#if 1 // =========================== ID management =============================
// void ReadIDfromEE() {
    // cfg.id = EE::Read32(EE_ADDR_DEVICE_ID);  // Read device ID
    // if(cfg.id < Config::kIdMin or cfg.id > Config::kIdMax) {
    //     Printf("\rUsing default ID\r");
    //     cfg.id = Config::kIdDefault;
    // }
// }

// retv ISetID(int32_t new_id) {
//     if(new_id < Config::kIdMin or new_id > Config::kIdMax) return retv::BadValue;
//     retv rslt = EE::Write32(EE_ADDR_DEVICE_ID, new_id);
//     if(rslt == retv::Ok) {
//         cfg.id = new_id;
//         Printf("New ID: %u\r", new_id);
//         return retv::Ok;
//     }
//     else {
//         Printf("EE error: %u\r", rslt);
//         return retv::Fail;
//     }
// }
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
