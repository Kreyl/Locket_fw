#include "board.h"
#include "led.h"
#include "vibro.h"
#include "Sequences.h"
#include "kl_lib.h"
#include "cc1101.h"
#include "app.h"

#if 1 // ======================== Variables and defines ========================
// Forever
EvtMsgQ_t<EvtMsg_t, MAIN_EVT_Q_LEN> EvtQMain;
static const UartParams_t CmdUartParams(115200, CMD_UART_PARAMS);
CmdUart_t Uart { &CmdUartParams };
static void ITask();
static void OnCmd(Shell_t *PShell);
static void ReadAndSetupMode();
// EEAddresses
#define EE_ADDR_DEVICE_ID       0
static const PinInputSetup_t DipSwPin[DIP_SW_CNT] = { DIP_SW8, DIP_SW7, DIP_SW6, DIP_SW5, DIP_SW4, DIP_SW3, DIP_SW2, DIP_SW1 };
static uint8_t GetDipSwitch();
static uint8_t ISetID(int32_t NewID);
void ReadIDfromEE();

LedRGBwPower_t Led { LED_R_PIN, LED_G_PIN, LED_B_PIN, LED_EN_PIN };
Vibro_t Vibro { VIBRO_SETUP };

static TmrKL_t TmrEverySecond {TIME_MS2I(1000), evtIdEverySecond, tktPeriodic};

void SleepNow(uint32_t Delay) {
    chSysLock();
    Iwdg::InitAndStart(Delay);
    Sleep::EnterStandby();
    chSysUnlock();
}

Dev_t Dev;
#endif

#if 0 // =============================== App ===================================
enum class DevType {Player, Master, PlaceOfPower} dev_type = DevType::Player;

class Player_t {
private:

public:
    int32_t goodness;
    const int32_t kgoodness_bottom = 0, kgoodness_red = 7200, kgoodness_yellow = 14400, kgoodness_top = 21600;

    void OnSecond() {
        if(goodness > 0) goodness--;
        if(goodness
    }

    void Reset() {
        goodness = kgoodness_top;
    }
} player;


#endif

int main(void) {
    // ==== Init Vcore & clock system ====
    SetupVCore(vcore1V2);
    Clk.SetMSI4MHz();
    Clk.UpdateFreqValues();
    // === Init OS ===
    halInit();
    chSysInit();
    EvtQMain.Init();
    // ==== Init hardware ====
    Uart.Init();
    Vibro.Init();
    Printf("\r%S %S\r", APP_NAME, XSTRINGIFY(BUILD_TIME));
    Clk.PrintFreqs();

    ReadAndSetupMode();

    TmrEverySecond.StartOrRestart();

    // Main cycle
    ITask();
}

__noreturn
void ITask() {
    while(true) {
        EvtMsg_t Msg = EvtQMain.Fetch(TIME_INFINITE);
        switch(Msg.ID) {
            case evtIdEverySecond:
                ReadAndSetupMode();
//                player.OnSecond();
                break;

            case evtIdCheckRxTable: {
                uint32_t Cnt = Msg.Value;
                switch(Cnt) {
                    case 0:
                        break; // Noone near
                    case 1:
                        Vibro.StartOrContinue(vsqBrr);
                        break;
                    case 2:
                        Vibro.StartOrContinue(vsqBrrBrr);
                        break;
                    default:
                        Vibro.StartOrContinue(vsqBrrBrrBrr);
                        break;
                }
            }
                break;

#if BUTTONS_ENABLED
        case evtIdButtons:
            Printf("Btn %u %u\r", Msg.BtnEvtInfo.BtnID, Msg.BtnEvtInfo.Type);
            if(Cfg.IsAriKaesu()) ProcessButtonsAriKaesu(Msg.BtnEvtInfo.BtnID, Msg.BtnEvtInfo.Type);
            else ProcessButtonsOthers(Msg.BtnEvtInfo.BtnID, Msg.BtnEvtInfo.Type);
            break;
#endif
#if ADC_REQUIRED
        case evtIdAdcRslt: Printf("Battery: %u mV\r", Adc.GetVDAmV(Adc.GetResultMedian(0))); break;
#endif
            case evtIdShellCmd:
                OnCmd((Shell_t*) Msg.Ptr);
                ((Shell_t*) Msg.Ptr)->SignalCmdProcessed();
                break;
            default:
                Printf("Unhandled Msg %u\r", Msg.ID);
                break;
        } // Switch
    } // while true
} // ITask()

__unused
void ReadAndSetupMode() {
    static uint32_t OldDipSettings = 0xFFFF;
    uint8_t b = GetDipSwitch();
    if(b == OldDipSettings) return;
    // Something has changed
    Printf("Dip: 0x%02X; ", b);
    OldDipSettings = b;
    // Select power
    b &= 0b1111; // Remove high bits = group 5678
    Cfg.TxPower = (b > 11) ? CC_PwrPlus12dBm : PwrTable[b];
    Printf("Pwr: %S\r", CC_PwrToString(Cfg.TxPower));
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
        int32_t FID = 0;
        if(PCmd->GetNext<int32_t>(&FID) != retvOk) {
            PShell->CmdError();
            return;
        }
        if(ISetID(FID) == retvOk)
            PShell->Ok();
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
//    Cfg.ID = EE::Read32(EE_ADDR_DEVICE_ID);  // Read device ID
//    if(Cfg.ID < ID_MIN or Cfg.ID > ID_MAX) {
//        Printf("\rUsing default ID\r");
//        Cfg.ID = ID_DEFAULT;
//    }
}

uint8_t ISetID(int32_t NewID) {
//    if(NewID < ID_MIN or NewID > ID_MAX) return retvFail;
//    uint8_t rslt = EE::Write32(EE_ADDR_DEVICE_ID, NewID);
//    if(rslt == retvOk) {
//        Cfg.ID = NewID;
//        Printf("New ID: %u\r", Cfg.ID);
//        return retvOk;
//    }
//    else {
//        Printf("EE error: %u\r", rslt);
        return retvFail;
//    }
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
