#include "board.h"
#include "led.h"
#include "vibro.h"
#include "beeper.h"
#include "Sequences.h"
#include "radio_lvl1.h"
#include "kl_i2c.h"
#include "kl_lib.h"
#include "kl_buf.h"
#include "pill.h"
#include "pill_mgr.h"
#include "MsgQ.h"
#include "SimpleSensors.h"
#include "buttons.h"

#include "Config.h"

#include <vector>

#if 1 // ======================== Variables and defines ========================
// Forever
EvtMsgQ_t<EvtMsg_t, MAIN_EVT_Q_LEN> EvtQMain;
static const UartParams_t CmdUartParams(115200, CMD_UART_PARAMS);
CmdUart_t Uart{&CmdUartParams};
static void ITask();
static void OnCmd(Shell_t *PShell);

static void ReadAndSetupMode();

// EEAddresses
#define EE_ADDR_DEVICE_ID       0

static const PinInputSetup_t DipSwPin[DIP_SW_CNT] = { DIP_SW8, DIP_SW7, DIP_SW6, DIP_SW5, DIP_SW4, DIP_SW3, DIP_SW2, DIP_SW1 };
static uint8_t GetDipSwitch();
static uint8_t ISetID(int32_t NewID);
void ReadIDfromEE();

// ==== Periphery ====
Vibro_t Vibro {VIBRO_SETUP};
#if BEEPER_ENABLED
Beeper_t Beeper {BEEPER_PIN};
#endif

LedRGBwPower_t Led { LED_R_PIN, LED_G_PIN, LED_B_PIN, LED_EN_PIN };

// ==== Timers ====
static TmrKL_t TmrEverySecond {TIME_MS2I(540), evtIdEverySecond, tktPeriodic};
//static TmrKL_t TmrRxTableCheck {MS2ST(2007), evtIdCheckRxTable, tktPeriodic};
static uint32_t TimeS;
#endif

int main(void) {
    // ==== Init Vcore & clock system ====
    SetupVCore(vcore1V5);
    Clk.SetMSI4MHz();
    Clk.UpdateFreqValues();

    // === Init OS ===
    halInit();
    chSysInit();
    EvtQMain.Init();

    // ==== Init hardware ====
    Uart.Init();
    ReadIDfromEE();
    Printf("\r%S %S; ID=%u\r", APP_NAME, XSTRINGIFY(BUILD_TIME), Cfg.ID);
//    Printf("\r%X\t%X\t%X\r", GetUniqID1(), GetUniqID2(), GetUniqID3());
//    if(Sleep::WasInStandby()) {
//        Uart.Printf("WasStandby\r");
//        Sleep::ClearStandbyFlag();
//    }
    Clk.PrintFreqs();
    Random::Seed(GetUniqID3());   // Init random algorythm with uniq ID

    Led.Init();
    Led.StartOrRestart(lsqStart);

    Printf("Vibro\r");
    Vibro.Init();
//    Vibro.SetupSeqEndEvt(evtIdVibroSeqDone);
//    Vibro.StartOrRestart(vsqBrrBrr);
//    chThdSleepMilliseconds(270);

#if BEEPER_ENABLED // === Beeper ===
    Printf("Beeper\r");
    Beeper.Init();
    Beeper.StartOrRestart(bsqBeepPillOk);
    chThdSleepMilliseconds(999);
#endif
#if BUTTONS_ENABLED
    SimpleSensors::Init();
#endif
//    Adc.Init();

#if PILL_ENABLED // === Pill ===
    i2c1.Init();
    PillMgr.Init();
#endif

    ReadAndSetupMode();

    // ==== Radio ====
    if(Radio.Init() != retvOk) Led.StartOrRestart(lsqFailure);

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
                TimeS++;
                ReadAndSetupMode();
                break;

#if BUTTONS_ENABLED
                case evtIdButtons:
                Printf("Btn %u %u\r", Msg.BtnEvtInfo.BtnID, Msg.BtnEvtInfo.Type);
                if (Msg.BtnEvtInfo.BtnID == 0 && Msg.BtnEvtInfo.Type == beShortPress) {
                	Beeper.StartOrRestart(bsqBeepPillOk);
                }
                break;
#endif

            case evtIdLedSeqDone:
                break;

            case evtIdShellCmd:
                OnCmd((Shell_t*)Msg.Ptr);
                ((Shell_t*)Msg.Ptr)->SignalCmdProcessed();
                break;
            default: Printf("Unhandled Msg %u\r", Msg.ID); break;
        } // Switch
    } // while true
} // ITask()

__unused
void ReadAndSetupMode() {
    static uint32_t OldDipSettings = 0xFFFF;
    uint8_t b = GetDipSwitch();
    if(b == OldDipSettings) return;
    // ==== Something has changed ====
    Printf("Dip: 0x%02X\r", b);
    OldDipSettings = b;
    // Reset everything
    Vibro.Stop();
    Led.Stop();
    // Select self type
    // Select power
//    b &= 0b1111; // Remove high bits
//    Printf("Type: %u; Pwr: %u\r", Type, b);
//    Cfg.SetSelfType(Type);
//    Radio.PktTx.Type = Cfg.Type;
//    Radio.PktTxFar.Type = Cfg.Type;
//    Indi.ShowSelfType();
//    Cfg.TxPower = (b > 11)? CC_PwrPlus12dBm : PwrTable[b];
}

#if 1 // ================= Command processing ====================
void OnCmd(Shell_t *PShell) {
    Cmd_t *PCmd = &PShell->Cmd;
    __attribute__((unused)) int32_t dw32 = 0;  // May be unused in some configurations
//    Uart.Printf("%S\r", PCmd->Name);
    // Handle command
    if(PCmd->NameIs("Ping")) {
        PShell->Ack(retvOk);
    }
    else if(PCmd->NameIs("Version")) PShell->Print("%S %S\r", APP_NAME, XSTRINGIFY(BUILD_TIME));

    else if(PCmd->NameIs("GetID")) PShell->Reply("ID", Cfg.ID);

    else if(PCmd->NameIs("SetID")) {
        int32_t FID = 0;
        if(PCmd->GetNext<int32_t>(&FID) != retvOk) { PShell->Ack(retvCmdError); return; }
        uint8_t r = ISetID(FID);
//        RMsg_t msg;
//        msg.Cmd = R_MSG_SET_CHNL;
//        msg.Value = ID2RCHNL(ID);
//        Radio.RMsgQ.SendNowOrExit(msg);
        PShell->Ack(r);
    }
    else if(PCmd->NameIs("Led")) {
        uint32_t r, g, b, delay;
        PCmd->GetNext<uint32_t>(&r);
        PCmd->GetNext<uint32_t>(&g);
        PCmd->GetNext<uint32_t>(&b);
        PCmd->GetNext<uint32_t>(&delay);
        const LedRGBChunk_t lsqOnCommand[] = {
                {csSetup, 0, {r, g, b}},
                {csWait, delay},
                {csEnd},
        };

        Led.StartOrRestart(lsqOnCommand);
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
#endif

//    else if(PCmd->NameIs("Pill")) {
//        if(PCmd->GetNextInt32(&dw32) != OK) { PShell->Ack(CMD_ERROR); return; }
//        PillType = (PillType_t)dw32;
//        App.SignalEvt(EVT_PILL_CHECK);
//    }

    else PShell->Ack(retvCmdUnknown);
}
#endif

#if 1 // =========================== ID management =============================
void ReadIDfromEE() {
    Cfg.ID = EE::Read32(EE_ADDR_DEVICE_ID);  // Read device ID
    if(Cfg.ID < ID_MIN or Cfg.ID > ID_MAX) {
        Printf("\rUsing default ID\r");
        Cfg.ID = ID_DEFAULT;
    }
//    Radio.PktTx.ID = Cfg.ID;
}

uint8_t ISetID(int32_t NewID) {
    if(NewID < ID_MIN or NewID > ID_MAX) return retvFail;
    uint8_t rslt = EE::Write32(EE_ADDR_DEVICE_ID, NewID);
    if(rslt == retvOk) {
        Cfg.ID = NewID;
//        Radio.PktTx.ID = Cfg.ID;
        Printf("New ID: %u\r", Cfg.ID);
        return retvOk;
    }
    else {
        Printf("EE error: %u\r", rslt);
        return retvFail;
    }
}
#endif

// ====== DIP switch ======
uint8_t GetDipSwitch() {
    uint8_t Rslt = 0;
    for(int i=0; i<DIP_SW_CNT; i++) PinSetupInput(DipSwPin[i].PGpio, DipSwPin[i].Pin, DipSwPin[i].PullUpDown);
    for(int i=0; i<DIP_SW_CNT; i++) {
        if(!PinIsHi(DipSwPin[i].PGpio, DipSwPin[i].Pin)) Rslt |= (1 << i);
        PinSetupAnalog(DipSwPin[i].PGpio, DipSwPin[i].Pin);
    }
    return Rslt;
}
