#include "board.h"
#include "led.h"
#include "vibro.h"
#include "Sequences.h"
#include "radio_lvl1.h"
#include "kl_i2c.h"
#include "kl_lib.h"
#include "pill.h"
#include "pill_mgr.h"
#include "MsgQ.h"
#include "main.h"
#include "SimpleSensors.h"
#include "buttons.h"

#if 1 // ======================== Variables and defines ========================
// Forever
EvtMsgQ_t<EvtMsg_t, MAIN_EVT_Q_LEN> EvtQMain;
static const UartParams_t CmdUartParams(115200, CMD_UART_PARAMS);
CmdUart_t Uart{&CmdUartParams};
static void ITask();
static void OnCmd(Shell_t *PShell);

// EEAddresses
#define EE_ADDR_DEVICE_ID       0

uint8_t ID;
static const PinInputSetup_t DipSwPin[DIP_SW_CNT] = { DIP_SW6, DIP_SW5, DIP_SW4, DIP_SW3, DIP_SW2, DIP_SW1 };
static uint8_t GetDipSwitch();
static uint8_t ISetID(int32_t NewID);
void ReadIDfromEE();

void ReadAndSetupMode();
void CheckRxTable();

LedRGBwPower_t Led { LED_R_PIN, LED_G_PIN, LED_B_PIN, LED_EN_PIN };
Vibro_t Vibro {VIBRO_SETUP};

uint8_t SignalTx = 0; // Initially, transmit nothing
bool ReactToSilmAndOath = false;
bool ReactToLight = false;
bool ReactToDark = false;

static struct MegaSeq_t {
    uint8_t Cnt, CurrIndx;
    LedRGBChunk_t *Lsq[7];
    void Reset() {
        Cnt = 0;
        CurrIndx = 0;
    }
    void PutLsq(LedRGBChunk_t *PLsq) {
        if(Cnt >= 7) return;
        Lsq[Cnt] = PLsq;
        Cnt++;
    }
    bool IsEnd() { return CurrIndx == Cnt; }
    LedRGBChunk_t* GetNextLsq() {
        LedRGBChunk_t* p = Lsq[CurrIndx];
        CurrIndx++;
        return p;
    }
} MegaSeq;

// ==== Timers ====
static TmrKL_t TmrEverySecond {MS2ST(1000), evtIdEverySecond, tktPeriodic};
static TmrKL_t TmrRxTableCheck {MS2ST(4500), evtIdCheckRxTable, tktPeriodic};
#endif

int main(void) {
    // ==== Init Vcore & clock system ====
    SetupVCore(vcore1V2);
    Clk.UpdateFreqValues();

    // === Init OS ===
    halInit();
    chSysInit();
    EvtQMain.Init();

    // ==== Init hardware ====
    Uart.Init();
    Uart.StartRx();
    ReadIDfromEE();
    Printf("\r%S %S ID=%u\r", APP_NAME, XSTRINGIFY(BUILD_TIME), ID);
    Clk.PrintFreqs();

    Led.Init();
    Led.SetupSeqEndEvt(EvtMsg_t(evtIdLedEnd));
    Vibro.Init();
    Vibro.StartOrRestart(vsqBrr);

    // ==== Time and timers ====
    TmrEverySecond.StartOrRestart();
    TmrRxTableCheck.StartOrRestart();

    // ==== Radio ====
    if(Radio.Init() == retvOk) Led.StartOrRestart(lsqStart);
    else Led.StartOrRestart(lsqFailure);
    chThdSleepMilliseconds(1008);

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
                break;

            case evtIdCheckRxTable: CheckRxTable(); break;

            case evtIdLedEnd:
                if(!MegaSeq.IsEnd()) Led.StartOrRestart(MegaSeq.GetNextLsq());
                break;

            case evtIdShellCmd:
                OnCmd((Shell_t*)Msg.Ptr);
                ((Shell_t*)Msg.Ptr)->SignalCmdProcessed();
                break;

            default: Printf("Unhandled Msg %u\r", Msg.ID); break;
        } // Switch
    } // while true
} // ITask()

void ProcessLsq(LedRGBChunk_t *PLsq, uint8_t Cnt) {
    if(Cnt > 0) {
        if(Cnt == 1)      PLsq[2].RepeatCnt = 0; // Single blink, no repeat
        else if(Cnt == 2) PLsq[2].RepeatCnt = 1; // Double blink
        else              PLsq[2].RepeatCnt = 2; // Triple blink
        MegaSeq.PutLsq(PLsq);  // Add lsq to megasequence
    }
}

void CheckRxTable() {
    uint8_t TableCnt = Radio.RxTable.GetCount();
    uint8_t SignalCnt = 0; // Count of sources of signal to consider
    uint8_t SilmCnt = 0, OatherCnt = 0, LightCnt = 0, DarkCnt = 0;
    // Analyze RxTable
    if(TableCnt > 0) {
        uint8_t ConsiderThisSignal;
        for(uint32_t i=0; i<TableCnt; i++) {
            uint8_t Signal = Radio.RxTable.Buf[i].Signal; // To make things shorter
            ConsiderThisSignal = 0;
            if(ReactToSilmAndOath and (Signal & SIGN_SILMARIL)) { SilmCnt++;   ConsiderThisSignal = 1; }
            if(ReactToSilmAndOath and (Signal & SIGN_OATH))     { OatherCnt++; ConsiderThisSignal = 1; }
            if(ReactToLight       and (Signal & SIGN_LIGHT))    { LightCnt++;  ConsiderThisSignal = 1; }
            if(ReactToDark        and (Signal & SIGN_DARK))     { DarkCnt++;   ConsiderThisSignal = 1; }
            SignalCnt += ConsiderThisSignal;
        }
        Radio.RxTable.Clear();
    } // TableCnt > 0
    else return; // Nobody near
//    EvtQMain.SendNowOrExit(EvtMsg_t(evtIdIndicate));
    // ==== Indicate ====
    if(SignalCnt == 0) return; // Nobody near is of our interest
    // Vibro
    if(SignalCnt == 1) Vibro.StartOrRestart(vsqBrr);
    else if(SignalCnt == 2) Vibro.StartOrRestart(vsqBrrBrr);
    else Vibro.StartOrRestart(vsqBrrBrrBrr);
    // Sequence of light
    MegaSeq.Reset();
    ProcessLsq(lsqSilm,  SilmCnt);
    ProcessLsq(lsqOath,  OatherCnt);
    ProcessLsq(lsqLight, LightCnt);
    ProcessLsq(lsqDark,  DarkCnt);
    // End of all
    MegaSeq.PutLsq(lsqNone);
    // Start Led
    if(!MegaSeq.IsEnd()) Led.StartOrRestart(MegaSeq.GetNextLsq());
}

void ReadAndSetupMode() {
    static uint8_t OldDipSettings = 0xFF;
    uint8_t b = GetDipSwitch();
    if(b == OldDipSettings) return;
    // ==== Something has changed ====
//    Printf("Dip: 0x%02X\r", b);
    OldDipSettings = b;
    uint8_t tmp;
    // What to Tx
    tmp = b & 0b111000;
    SignalTx = tmp >> 2;
    // What to Rx
    tmp = b & 0b000111;
    uint8_t WhatToRx = tmp << 1;
    // Display
    if(SignalTx & SIGN_OATH)  Printf("Tx Oath\r");
    if(SignalTx & SIGN_LIGHT) Printf("Tx Light\r");
    if(SignalTx & SIGN_DARK)  Printf("Tx Dark\r");
    if(WhatToRx & SIGN_OATH)  Printf("Rx Oath\r");
    if(WhatToRx & SIGN_LIGHT) Printf("Rx Light\r");
    if(WhatToRx & SIGN_DARK)  Printf("Rx Dark\r");
    // Setup
    ReactToSilmAndOath = (WhatToRx & SIGN_OATH);
    ReactToLight = (WhatToRx & SIGN_LIGHT);
    ReactToDark = (WhatToRx & SIGN_DARK);
}


#if 1 // ======================= Command processing ============================
void OnCmd(Shell_t *PShell) {
	Cmd_t *PCmd = &PShell->Cmd;
    __attribute__((unused)) int32_t dw32 = 0;  // May be unused in some configurations
//    Uart.Printf("%S\r", PCmd->Name);
    // Handle command
    if(PCmd->NameIs("Ping")) {
        PShell->Ack(retvOk);
    }
    else if(PCmd->NameIs("Version")) PShell->Printf("%S %S\r", APP_NAME, XSTRINGIFY(BUILD_TIME));

    else if(PCmd->NameIs("GetID")) PShell->Reply("ID", ID);

    else if(PCmd->NameIs("SetID")) {
        if(PCmd->GetNext<uint8_t>(&ID) != retvOk) { PShell->Ack(retvCmdError); return; }
        uint8_t r = ISetID(ID);
        PShell->Ack(r);
    }

    else PShell->Ack(retvCmdUnknown);
}
#endif

#if 1 // =========================== ID management =============================
void ReadIDfromEE() {
    ID = EE::Read32(EE_ADDR_DEVICE_ID);  // Read device ID
    if(ID < ID_MIN or ID > ID_MAX) {
        Printf("\rUsing default ID\r");
        ID = 1;
    }
}

uint8_t ISetID(int32_t NewID) {
    if(NewID < ID_MIN or NewID > ID_MAX) return retvFail;
    uint8_t rslt = EE::Write32(EE_ADDR_DEVICE_ID, NewID);
    if(rslt == retvOk) {
        ID = NewID;
        Printf("New ID: %u\r", ID);
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
