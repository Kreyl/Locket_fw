#include "board.h"
#include "led.h"
#include "vibro.h"
#include "beeper.h"
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
// SM
#include "qhsm.h"
#include "eventHandlers.h"
#include "mHoS.h"

#if 1 // ======================== Variables and defines ========================
// Forever
EvtMsgQ_t<EvtMsg_t, MAIN_EVT_Q_LEN> EvtQMain;
static const UartParams_t CmdUartParams(115200, CMD_UART_PARAMS);
CmdUart_t Uart{&CmdUartParams};
static void ITask();
static void OnCmd(Shell_t *PShell);

static void ReadAndSetupMode();

// EEAddresses
#define EE_ADDR_DEVICE_ID   0
// StateMachines
#define EE_ADDR_STATE       2048
#define EE_ADDR_HP          2052
#define EE_ADDR_MAX_HP      2056
#define EE_ADDR_DEFAULT_HP  2060
void InitSM();
void SendEventSM(int QSig, unsigned int SrcID, unsigned int Value);
static mHoSQEvt e;

int32_t ID;
static const PinInputSetup_t DipSwPin[DIP_SW_CNT] = { DIP_SW8, DIP_SW7, DIP_SW6, DIP_SW5, DIP_SW4, DIP_SW3, DIP_SW2, DIP_SW1 };
static uint8_t GetDipSwitch();
static uint8_t ISetID(int32_t NewID);
void ReadIDfromEE();

// ==== Periphery ====
Vibro_t VibroMotor {VIBRO_SETUP};
#if BEEPER_ENABLED
Beeper_t Beeper {BEEPER_PIN};
#endif

LedRGBwPower_t Led { LED_R_PIN, LED_G_PIN, LED_B_PIN, LED_EN_PIN };

// ==== Timers ====
static TmrKL_t TmrEverySecond {TIME_MS2I(1000), evtIdEverySecond, tktPeriodic};
//static TmrKL_t TmrRxTableCheck {MS2ST(2007), evtIdCheckRxTable, tktPeriodic};
static int32_t TimeS;
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
//    Led.SetupSeqEndEvt(chThdGetSelfX(), EVT_LED_SEQ_END);
    VibroMotor.Init();
#if BEEPER_ENABLED // === Beeper ===
    Beeper.Init();
    Beeper.StartOrRestart(bsqBeepBeep);
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

    // ==== Time and timers ====
//    TmrEverySecond.StartOrRestart();
//    TmrRxTableCheck.StartOrRestart();

    // ==== Radio ====
    if(Radio.Init() == retvOk) Led.StartOrRestart(lsqStart);
    else Led.StartOrRestart(lsqFailure);
    VibroMotor.StartOrRestart(vsqBrrBrr);
    chThdSleepMilliseconds(1008);

    InitSM();

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
                SendEventSM(TICK_SEC_SIG, 0, 0);
                break;

            case evtIdDamagePkt: SendEventSM(DAMAGE_RECEIVED_SIG, Msg.Value, 1); break;
            case evtIdDeathPkt:  SendEventSM(KILL_SIGNAL_RECEIVED_SIG, 0, 0);    break;
            case evtIdUpdateHP:  SendEventSM(UPDATE_HP_SIG, 0, Msg.Value);       break;

#if BUTTONS_ENABLED
            case evtIdButtons:
//                Printf("Btn %u\r", Msg.BtnEvtInfo.BtnID);
                if(Msg.BtnEvtInfo.Type == beShortPress) {
                    SendEventSM(BUTTON_PRESSED_SIG, 0, 0);
                }
                else if(Msg.BtnEvtInfo.Type == beLongCombo and Msg.BtnEvtInfo.BtnCnt == 3) {
                    Printf("Combo\r");
                    SendEventSM(BUTTON_LONG_PRESSED_SIG, 0, 0);
                }
                break;
#endif

#if PILL_ENABLED // ==== Pill ====
            case evtIdPillConnected:
                Printf("Pill: %u\r", PillMgr.Pill.DWord32);
                switch(PillMgr.Pill.DWord32) {
                    case 1: SendEventSM(PILL_MUTANT_SIG, 0, 0); break;
                    case 2: SendEventSM(PILL_IMMUNE_SIG, 0, 0); break;
                    case 3: SendEventSM(PILL_HP_DOUBLE_SIG, 0, 0); break;
                    case 4: SendEventSM(PILL_HEAL_SIG, 0, 0); break;
                    case 5: SendEventSM(PILL_SURGE_SIG , 0, 0); break;
                    default: SendEventSM(PILL_RESET_SIG, 0, 0); break;
                }
                break;

            case evtIdPillDisconnected:
                Printf("Pill disconn\r");
                SendEventSM(PILL_REMOVED_SIG, 0, 0);
//                Led.StartOrRestart(lsqNoPill);
                break;
#endif

            case evtIdShellCmd:
                OnCmd((Shell_t*)Msg.Ptr);
                ((Shell_t*)Msg.Ptr)->SignalCmdProcessed();
                break;
            default: Printf("Unhandled Msg %u\r", Msg.ID); break;
        } // Switch
    } // while true
} // ITask()

#if 1 // ==== State Machines ====
void InitSM() {
    // Load saved data
    uint32_t HP = EE::Read32(EE_ADDR_HP);
    uint32_t MaxHP = EE::Read32(EE_ADDR_MAX_HP);
    uint32_t DefaultHP = EE::Read32(EE_ADDR_DEFAULT_HP);
    uint32_t State = EE::Read32(EE_ADDR_STATE);
    // Check if params are bad
    if(!(HP <= MaxHP and DefaultHP <= MaxHP and State <= 3)) {
        HP = 20;
        MaxHP = 20;
        DefaultHP = 20;
        State = SIMPLE;
    }
    // Init
    MHoS_ctor(HP, MaxHP, DefaultHP, State);
    QMSM_INIT(the_mHoS, (QEvt *)0);
}

void SendEventSM(int QSig, unsigned int SrcID, unsigned int Value) {
    e.super.sig = QSig;
    e.id = SrcID;
    e.value = Value;
    QMSM_DISPATCH(the_mHoS,  &(e.super));
}

extern "C" {
void SaveState(uint32_t AState) {
    if(EE::Write32(EE_ADDR_STATE, AState) != retvOk) Printf("Saving State fail\r");
}

BaseChunk_t vsqSMBrr[] = {
        {csSetup, VIBRO_VOLUME},
        {csWait, 99},
        {csSetup, 0},
        {csEnd}
};

void Vibro(uint32_t Duration_ms) {
    vsqSMBrr[1].Time_ms = Duration_ms;
    VibroMotor.StartOrRestart(vsqSMBrr);
}


LedRGBChunk_t lsqSM[] = {
        {csSetup, 0, clRed},
        {csWait, 207},
        {csSetup, 0, {0,4,0}},
        {csEnd},
};

void Flash(uint8_t R, uint8_t G, uint8_t B, uint32_t Duration_ms) {
    lsqSM[0].Color.FromRGB(R, G, B);
    lsqSM[1].Time_ms = Duration_ms;
    Led.StartOrRestart(lsqSM);
}

void SendKillingSignal() {
    Radio.RMsgQ.SendWaitingAbility(RMsg_t(R_MSG_SEND_KILL), 999);
}

#define THE_WORD    0xCA115EA1
void ClearPill() {
    uint32_t DWord32 = THE_WORD;
    if(PillMgr.Write(0, &DWord32, 4) != retvOk) Printf("ClearPill fail\r");
}

bool PillWasImmune() {
    uint32_t DWord32;
    if(PillMgr.Read(0, &DWord32, 4) == retvOk) return (DWord32 == THE_WORD);
    else return false;
}

void SaveHP(uint32_t HP) {
    if(EE::Write32(EE_ADDR_HP, HP) != retvOk) Printf("Saving HP fail\r");
}

void SaveMaxHP(uint32_t HP) {
    if(EE::Write32(EE_ADDR_MAX_HP, HP) != retvOk) Printf("Saving MaxHP fail\r");
}

void SaveDefaultHP(uint32_t HP) {
    if(EE::Write32(EE_ADDR_DEFAULT_HP, HP) != retvOk) Printf("Saving DefHP fail\r");
}
} // extern C
#endif

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
    Printf("Dip: 0x%02X\r", b);
    OldDipSettings = b;
    // Reset everything
    VibroMotor.Stop();
    Led.Stop();
    // Select mode
//    if(b & 0b100000) {
//        Led.StartOrRestart(lsqTx);
    // Select power
    b &= 0b11111; // Remove high bits
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


#if 1 // === Pill ===
    else if(PCmd->NameIs("ReadPill")) {
        int32_t DWord32;
        uint8_t Rslt = PillMgr.Read(0, &DWord32, 4);
        if(Rslt == retvOk) {
            PShell->Print("Read %d\r\n", DWord32);
        }
        else PShell->Ack(retvFail);
    }

    else if(PCmd->NameIs("WritePill")) {
        int32_t DWord32;
        if(PCmd->GetNext<int32_t>(&DWord32) == retvOk) {
            uint8_t Rslt = PillMgr.Write(0, &DWord32, 4);
            PShell->Ack(Rslt);
        }
        else PShell->Ack(retvCmdError);
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
