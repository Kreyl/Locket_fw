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
#include "main.h"
#include "SimpleSensors.h"
#include "buttons.h"

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

int32_t ID;
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
static TmrKL_t TmrEverySecond {TIME_MS2I(1000), evtIdEverySecond, tktPeriodic};
//static TmrKL_t TmrRxTableCheck {MS2ST(2007), evtIdCheckRxTable, tktPeriodic};
static int32_t TimeS;
void CheckRxTable();
#endif

#if 1 // ========================== Logic ======================================
#define DISTANCE_dBm        -99  // Replace with pseudo-distance

enum VibroType_t { vbrNone, vbrOneTwoMany };

// How to react
struct Reaction_t {
    std::vector<LedRGBChunk_t> Seq;
    VibroType_t VibroType = vbrNone;
    bool MustStopTx = false;
};

// Locket settings
struct ReactItem_t {
    int32_t Source;     // Type to react to
    Reaction_t *PReact; // What to do
    int32_t Distance;   // Min dist when react
};

struct LocketType_t {
    Reaction_t *ReactOnPwrOn;
    std::vector<ReactItem_t> React;
};

// Indication command with all required data
struct IndicationCmd_t {
    Reaction_t *PReact; // What to do
    int32_t CountOfNeighbors;

    IndicationCmd_t& operator = (const IndicationCmd_t &Right) {
        PReact = Right.PReact;
        CountOfNeighbors = Right.CountOfNeighbors;
        return *this;
    }
    IndicationCmd_t() : PReact(nullptr), CountOfNeighbors(0) {}
    IndicationCmd_t(Reaction_t* AReact, int32_t ACnt) : PReact(AReact), CountOfNeighbors(ACnt) {}
};

std::vector<Reaction_t> Rctns;
std::vector<LocketType_t> LcktType;
std::vector<int32_t> CountOfTypes;

uint32_t SelfType = 0; // set by dip
bool MustTx = true;

CircBuf_t<IndicationCmd_t, 18> IndicationQ;

void ShowSelfType() {
    std::vector<LedRGBChunk_t> *Seq = &LcktType[SelfType].ReactOnPwrOn->Seq;
    if(!Seq->empty()) {
        Led.StartOrRestart(Seq->data());
    }
}

void ProcessIndicationQ() {
    IndicationCmd_t Cmd;
    while(IndicationQ.Get(&Cmd) == retvOk) {
        // LED
        std::vector<LedRGBChunk_t> *Seq = &Cmd.PReact->Seq;
        if(!Seq->empty()) {
            Led.StartOrRestart(Seq->data());
        }
        // Vibro
        if(Cmd.PReact->VibroType == vbrOneTwoMany) {
            if     (Cmd.CountOfNeighbors == 1) Vibro.StartOrRestart(vsqBrr);
            else if(Cmd.CountOfNeighbors == 2) Vibro.StartOrRestart(vsqBrrBrr);
            else if(Cmd.CountOfNeighbors >  2) Vibro.StartOrRestart(vsqBrrBrrBrr);
        }
        // Disable Radio TX if needed
        if(Cmd.PReact->MustStopTx == true) {
            MustTx = false;
            Radio.MustTx = false; // Stop it now
        }
        // Get out if LED or Vibro is started. Otherwise noone will call ProcessIndication.
        if(!Led.IsIdle() or !Vibro.IsIdle()) break;
    } // if get cmd

    if(IndicationQ.IsEmpty()) { // Q is empty
//        Printf("MTX: %u\r", MustTx);
        Radio.MustTx = MustTx; // start or stop transmitting
    }
}

void StartIndication() {
    MustTx = true;
    ProcessIndicationQ();
}

#define LED_DELAY   504

void ReadConfig() {
    CountOfTypes.resize(6); // Locket types
#if 1 // Setup reactions
    Rctns.resize(7);
    std::vector<LedRGBChunk_t> *Seq;
    // Indx 0, Yellow
    Seq = &Rctns[0].Seq;
    Seq->clear();
    Seq->push_back(LedRGBChunk_t(csSetup, 0, {255, 255, 0}));
    Seq->push_back(LedRGBChunk_t(csWait, LED_DELAY));
    Seq->push_back(LedRGBChunk_t(csSetup, 0, {0, 0, 0}));
    Seq->push_back(LedRGBChunk_t(csWait, LED_DELAY));
    Seq->push_back(LedRGBChunk_t(csSetup, 0, {255, 255, 0}));
    Seq->push_back(LedRGBChunk_t(csWait, LED_DELAY));
    Seq->push_back(LedRGBChunk_t(csSetup, 0, {0, 0, 0}));
    Seq->push_back(LedRGBChunk_t(csWait, LED_DELAY));
    Seq->push_back(LedRGBChunk_t(csEnd));
    Rctns[0].VibroType = vbrOneTwoMany;

    // Indx 1, White
    Seq = &Rctns[1].Seq;
    Seq->clear();
    Seq->push_back(LedRGBChunk_t(csSetup, 0, {255, 255, 255}));
    Seq->push_back(LedRGBChunk_t(csWait, LED_DELAY));
    Seq->push_back(LedRGBChunk_t(csSetup, 0, {0, 0, 0}));
    Seq->push_back(LedRGBChunk_t(csWait, LED_DELAY));
    Seq->push_back(LedRGBChunk_t(csSetup, 0, {255, 255, 255}));
    Seq->push_back(LedRGBChunk_t(csWait, LED_DELAY));
    Seq->push_back(LedRGBChunk_t(csSetup, 0, {0, 0, 0}));
    Seq->push_back(LedRGBChunk_t(csWait, LED_DELAY));
    Seq->push_back(LedRGBChunk_t(csEnd));
    Rctns[1].VibroType = vbrOneTwoMany;

    // Indx 2, Isalamiri
    Rctns[2].MustStopTx = true;

    // Indx 3, Red
    Seq = &Rctns[3].Seq;
    Seq->clear();
    Seq->push_back(LedRGBChunk_t(csSetup, 0, {255, 0, 0}));
    Seq->push_back(LedRGBChunk_t(csWait, LED_DELAY));
    Seq->push_back(LedRGBChunk_t(csSetup, 0, {0, 0, 0}));
    Seq->push_back(LedRGBChunk_t(csWait, LED_DELAY));
    Seq->push_back(LedRGBChunk_t(csSetup, 0, {255, 0, 0}));
    Seq->push_back(LedRGBChunk_t(csWait, LED_DELAY));
    Seq->push_back(LedRGBChunk_t(csSetup, 0, {0, 0, 0}));
    Seq->push_back(LedRGBChunk_t(csWait, LED_DELAY));
    Seq->push_back(LedRGBChunk_t(csEnd));
    Rctns[3].VibroType = vbrOneTwoMany;

    // Indx 4, Blue
    Seq = &Rctns[4].Seq;
    Seq->clear();
    Seq->push_back(LedRGBChunk_t(csSetup, 0, {0, 0, 255}));
    Seq->push_back(LedRGBChunk_t(csWait, LED_DELAY));
    Seq->push_back(LedRGBChunk_t(csSetup, 0, {0, 0, 0}));
    Seq->push_back(LedRGBChunk_t(csWait, LED_DELAY));
    Seq->push_back(LedRGBChunk_t(csSetup, 0, {0, 0, 255}));
    Seq->push_back(LedRGBChunk_t(csWait, LED_DELAY));
    Seq->push_back(LedRGBChunk_t(csSetup, 0, {0, 0, 0}));
    Seq->push_back(LedRGBChunk_t(csWait, LED_DELAY));
    Seq->push_back(LedRGBChunk_t(csEnd));
    Rctns[4].VibroType = vbrOneTwoMany;

    // Indx 5, TwoColors
    Seq = &Rctns[5].Seq;
    Seq->clear();
    Seq->push_back(LedRGBChunk_t(csSetup, 0, {0, 0, 255}));
    Seq->push_back(LedRGBChunk_t(csWait, LED_DELAY));
    Seq->push_back(LedRGBChunk_t(csSetup, 0, {0, 0, 0}));
    Seq->push_back(LedRGBChunk_t(csWait, LED_DELAY));
    Seq->push_back(LedRGBChunk_t(csSetup, 0, {255, 0, 0}));
    Seq->push_back(LedRGBChunk_t(csWait, LED_DELAY));
    Seq->push_back(LedRGBChunk_t(csSetup, 0, {0, 0, 0}));
    Seq->push_back(LedRGBChunk_t(csWait, LED_DELAY));
    Seq->push_back(LedRGBChunk_t(csEnd));
    Rctns[5].VibroType = vbrOneTwoMany;

    // Indx 6, IsalamiriOn
    Seq = &Rctns[6].Seq;
    Seq->clear();
    Seq->push_back(LedRGBChunk_t(csSetup, 0, {0, 255, 0}));
    Seq->push_back(LedRGBChunk_t(csWait, LED_DELAY));
    Seq->push_back(LedRGBChunk_t(csSetup, 0, {0, 0, 0}));
    Seq->push_back(LedRGBChunk_t(csWait, LED_DELAY));
    Seq->push_back(LedRGBChunk_t(csSetup, 0, {0, 255, 0}));
    Seq->push_back(LedRGBChunk_t(csWait, LED_DELAY));
    Seq->push_back(LedRGBChunk_t(csSetup, 0, {0, 0, 0}));
    Seq->push_back(LedRGBChunk_t(csWait, LED_DELAY));
    Seq->push_back(LedRGBChunk_t(csEnd));
#endif

    // Setup locket types
    LcktType.resize(6);
    LocketType_t *Lct;
    ReactItem_t *ReactItem;
    // ======== Setup locket types ========
    // === YellowTransmit ===
    Lct = &LcktType[0];
    Lct->ReactOnPwrOn = &Rctns[0]; // Yellow
    Lct->React.resize(1);
    ReactItem = &Lct->React[0];
    ReactItem->Source = 2;         // IsalamiriTransmit
    ReactItem->PReact = &Rctns[2]; // Isalamiri
    ReactItem->Distance = 1;

    // === WhiteTransmit ===
    Lct = &LcktType[1];
    Lct->ReactOnPwrOn = &Rctns[1]; // White
    Lct->React.resize(1);
    ReactItem = &Lct->React[0];
    ReactItem->Source = 2;         // IsalamiriTransmit
    ReactItem->PReact = &Rctns[2]; // Isalamiri
    ReactItem->Distance = 1;

    // === IsalamiriTransmit ===
    LcktType[2].ReactOnPwrOn = &Rctns[6]; // IsalamiriOn
    LcktType[2].React.resize(0);

    // === BlueTransmit ===
    Lct = &LcktType[3];
    Lct->ReactOnPwrOn = &Rctns[4]; // Blue
    Lct->React.resize(6);
    ReactItem = &Lct->React[0];
    ReactItem->Source = 0;         // YellowTransmit
    ReactItem->PReact = &Rctns[0]; // Yellow
    ReactItem->Distance = 1;

    ReactItem = &Lct->React[1];
    ReactItem->Source = 1;         // WhiteTransmit
    ReactItem->PReact = &Rctns[1]; // White
    ReactItem->Distance = 1;

    ReactItem = &Lct->React[2];
    ReactItem->Source = 2;         // IsalamiriTransmit
    ReactItem->PReact = &Rctns[2]; // Isalamiri
    ReactItem->Distance = 1;

    ReactItem = &Lct->React[3];
    ReactItem->Source = 3;         // BlueTransmit
    ReactItem->PReact = &Rctns[4]; // Blue
    ReactItem->Distance = 1;

    ReactItem = &Lct->React[4];
    ReactItem->Source = 4;         // RedTransmit
    ReactItem->PReact = &Rctns[3]; // Red
    ReactItem->Distance = 1;

    ReactItem = &Lct->React[5];
    ReactItem->Source = 5;         // TwoColorsTransmit
    ReactItem->PReact = &Rctns[5]; // TwoColors
    ReactItem->Distance = 1;

    // === RedTransmit ===
    Lct = &LcktType[4];
    Lct->ReactOnPwrOn = &Rctns[3]; // Red
    Lct->React.resize(6);
    ReactItem = &Lct->React[0];
    ReactItem->Source = 0;         // YellowTransmit
    ReactItem->PReact = &Rctns[0]; // Yellow
    ReactItem->Distance = 1;

    ReactItem = &Lct->React[1];
    ReactItem->Source = 1;         // WhiteTransmit
    ReactItem->PReact = &Rctns[1]; // White
    ReactItem->Distance = 1;

    ReactItem = &Lct->React[2];
    ReactItem->Source = 2;         // IsalamiriTransmit
    ReactItem->PReact = &Rctns[2]; // Isalamiri
    ReactItem->Distance = 1;

    ReactItem = &Lct->React[3];
    ReactItem->Source = 3;         // BlueTransmit
    ReactItem->PReact = &Rctns[4]; // Blue
    ReactItem->Distance = 1;

    ReactItem = &Lct->React[4];
    ReactItem->Source = 4;         // RedTransmit
    ReactItem->PReact = &Rctns[3]; // Red
    ReactItem->Distance = 1;

    ReactItem = &Lct->React[5];
    ReactItem->Source = 5;         // TwoColorsTransmit
    ReactItem->PReact = &Rctns[5]; // TwoColors
    ReactItem->Distance = 1;

    // === TwoColorsTransmit ===
    Lct = &LcktType[5];
    Lct->ReactOnPwrOn = &Rctns[5]; // TwoColors
    Lct->React.resize(6);
    ReactItem = &Lct->React[0];
    ReactItem->Source = 0;         // YellowTransmit
    ReactItem->PReact = &Rctns[0]; // Yellow
    ReactItem->Distance = 1;

    ReactItem = &Lct->React[1];
    ReactItem->Source = 1;         // WhiteTransmit
    ReactItem->PReact = &Rctns[1]; // White
    ReactItem->Distance = 1;

    ReactItem = &Lct->React[2];
    ReactItem->Source = 2;         // IsalamiriTransmit
    ReactItem->PReact = &Rctns[2]; // Isalamiri
    ReactItem->Distance = 1;

    ReactItem = &Lct->React[3];
    ReactItem->Source = 3;         // BlueTransmit
    ReactItem->PReact = &Rctns[4]; // Blue
    ReactItem->Distance = 1;

    ReactItem = &Lct->React[4];
    ReactItem->Source = 4;         // RedTransmit
    ReactItem->PReact = &Rctns[3]; // Red
    ReactItem->Distance = 1;

    ReactItem = &Lct->React[5];
    ReactItem->Source = 5;         // TwoColorsTransmit
    ReactItem->PReact = &Rctns[5]; // TwoColors
    ReactItem->Distance = 1;
}

void CheckRxTable() {
    std::vector<ReactItem_t>& SelfReact = LcktType[SelfType].React;
    // Is it needed? Do we have reaction?
    if(SelfReact.empty()) return;
    // Analyze table: get count of every type near
    for(int32_t &Cnt : CountOfTypes) Cnt = 0;   // Reset count
    int32_t MaxTypeID = CountOfTypes.size() - 1;
    RxTable_t Tbl = Radio.GetRxTable();
    for(uint32_t i=0; i<Tbl.Cnt; i++) { // i is just number of pkt in table, no relation with type
        rPkt_t Pkt = Tbl[i];
        if(Pkt.Type <= MaxTypeID) { // Type is ok
            // Check if Distance is ok
//            if(Pkt.Rssi > SelfReact[Pkt.Type].Distance) { // TODO
            if(Pkt.Rssi > -75) {
                CountOfTypes[Pkt.Type]++;
            }
        }
    }
    // Put reactions to queue
    for(int32_t TypeRcvd=0; TypeRcvd <= MaxTypeID; TypeRcvd++) {
        int32_t N = CountOfTypes[TypeRcvd];
        if(N) { // Here they are! Do we need to react?
            for(ReactItem_t& ritem : SelfReact) {
                if(ritem.Source == TypeRcvd) { // Yes, we must react
//                    Printf("Put\r");
                    IndicationQ.PutIfNotOverflow(IndicationCmd_t(ritem.PReact, N));
                    break;
                }
            } // for reactitem
        } // if N
    } // for
    StartIndication();
}
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
    Printf("\r%S %S; ID=%u\r", APP_NAME, XSTRINGIFY(BUILD_TIME), ID);
//    Uart.Printf("ID: %X %X %X\r", GetUniqID1(), GetUniqID2(), GetUniqID3());
//    if(Sleep::WasInStandby()) {
//        Uart.Printf("WasStandby\r");
//        Sleep::ClearStandbyFlag();
//    }
    Clk.PrintFreqs();
//    RandomSeed(GetUniqID3());   // Init random algorythm with uniq ID

    Led.Init();
    Led.SetupSeqEndEvt(evtIdLedSeqDone);
    Vibro.Init();
    Vibro.SetupSeqEndEvt(evtIdVibroSeqDone);
//    Vibro.StartOrRestart(vsqBrrBrr);
#if BEEPER_ENABLED // === Beeper ===
//    Beeper.Init();
//    Beeper.StartOrRestart(bsqBeepBeep);
//    chThdSleepMilliseconds(702);    // Let it complete the show
#endif
#if BUTTONS_ENABLED
    SimpleSensors::Init();
#endif
//    Adc.Init();

#if PILL_ENABLED // === Pill ===
    i2c1.Init();
    PillMgr.Init();
#endif

    // ==== Radio ====
    if(Radio.Init() != retvOk) {
        Led.StartOrRestart(lsqFailure);
        chThdSleepMilliseconds(1008);
    }

    ReadConfig();
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
                TimeS++;
                ReadAndSetupMode();
                if(TimeS % 4 == 0 and IndicationQ.IsEmpty()) CheckRxTable();
                break;

#if BUTTONS_ENABLED
            case evtIdButtons:
                Printf("Btn %u\r", Msg.BtnEvtInfo.BtnID);
                Radio.MustTx = true;
                Led.StartOrRestart(lsqTx);
                break;
#endif

            case evtIdLedSeqDone:
                if(Vibro.IsIdle()) ProcessIndicationQ(); // proceed with indication if current completed
                break;
            case evtIdVibroSeqDone:
                if(Led.IsIdle()) ProcessIndicationQ(); // proceed with indication if current completed
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
static const uint8_t PwrTable[12] = {
        CC_PwrMinus30dBm, // 0
        CC_PwrMinus27dBm, // 1
        CC_PwrMinus25dBm, // 2
        CC_PwrMinus20dBm, // 3
        CC_PwrMinus15dBm, // 4
        CC_PwrMinus10dBm, // 5
        CC_PwrMinus6dBm,  // 6
        CC_Pwr0dBm,       // 7
        CC_PwrPlus5dBm,   // 8
        CC_PwrPlus7dBm,   // 9
        CC_PwrPlus10dBm,  // 10
        CC_PwrPlus12dBm   // 11
};

__unused
void ReadAndSetupMode() {
    static uint32_t OldDipSettings = 0xFFFF;
    uint8_t b = GetDipSwitch();
    if(b == OldDipSettings) return;
    // ==== Something has changed ====
    Printf("Dip: 0x%02X; ", b);
    OldDipSettings = b;
    // Reset everything
    IndicationQ.Flush();
    Vibro.Stop();
    Led.Stop();
    // Select self type
    chSysLock();
    SelfType = b >> 4;
    if(SelfType >= LcktType.size()) SelfType = LcktType.size() - 1;
    chSysUnlock();
    // Select power
    b &= 0b1111; // Remove high bits
    Printf("Type: %u; Pwr: %u\r", SelfType, b);
    ShowSelfType();
    RMsg_t msg;
    msg.Cmd = R_MSG_SET_PWR;
    msg.Value = (b > 11)? CC_PwrPlus12dBm : PwrTable[b];
    Radio.RMsgQ.SendNowOrExit(msg);
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

    else if(PCmd->NameIs("GetID")) PShell->Reply("ID", ID);

    else if(PCmd->NameIs("SetID")) {
        if(PCmd->GetNext<int32_t>(&ID) != retvOk) { PShell->Ack(retvCmdError); return; }
        uint8_t r = ISetID(ID);
        RMsg_t msg;
        msg.Cmd = R_MSG_SET_CHNL;
        msg.Value = ID2RCHNL(ID);
        Radio.RMsgQ.SendNowOrExit(msg);
        PShell->Ack(r);
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
    ID = EE::Read32(EE_ADDR_DEVICE_ID);  // Read device ID
    if(ID < ID_MIN or ID > ID_MAX) {
        Printf("\rUsing default ID\r");
        ID = ID_DEFAULT;
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
