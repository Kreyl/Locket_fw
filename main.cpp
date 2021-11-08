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
static TmrKL_t TmrEverySecond {TIME_MS2I(1000), evtIdEverySecond, tktPeriodic};
//static TmrKL_t TmrRxTableCheck {MS2ST(2007), evtIdCheckRxTable, tktPeriodic};
static uint32_t TimeS;
static uint32_t TimeToBeHidden = 0, TimeToBeSilent = 0;
#endif

#if 1 // ========================== Logic ======================================
#define GOOD_DISTANCE_dBm       (-111)
#define ARI_KAESU_IND_TIME_S    18
#define CHANGED_IND_TIME_S      2
#define TIME_TO_BE_HIDDEN_S     3600
#define TIME_TO_BE_SILENT_S     600

#define VIBRO_ARI_KAESU         vsqBrrBrr
#define VIBRO_SMTH_CHANGED      vsqBrr
#define VIBRO_ATTACK            vsqBrr

void CheckRxTable();
void ProcessButtonsAriKaesu(uint8_t BtnID, BtnEvt_t Type);
void ProcessButtonsOthers(uint8_t BtnID, BtnEvt_t Type);

class TypesAround_t {
private:
    int32_t ITypes1[TYPE_CNT] = {0}, ITypes2[TYPE_CNT] = {0};
    int32_t *pNew = ITypes1, *pOld = ITypes2;
    bool AriIsNear = false, KaesuIsNear = false;
public:
    void IncCnt(uint8_t Type) { pNew[Type]++; }

    bool IsAriAppeared() {
        if(pNew[TYPE_ARI] != 0 and !AriIsNear) {
            AriIsNear = true;
            return true;
        }
        else if(pNew[TYPE_ARI] == 0) AriIsNear = false;
        return false;
    }
    bool IsKaesuAppeared() {
        if(pNew[TYPE_KAESU] != 0 and !KaesuIsNear) {
            KaesuIsNear = true;
            return true;
        }
        else if(pNew[TYPE_KAESU] == 0) KaesuIsNear = false;
        return false;
    }

    bool HasChangedOthers() {
        for(int i=2; i<TYPE_CNT; i++) {
            if(pNew[i] != pOld[i]) {
                Printf("Changed: ");
                for(int j=2; j<TYPE_CNT; j++)  Printf("%u %u; ", pNew[j], pOld[j]);
                PrintfEOL();
                return true;
            }
        }
        return false;
    }
    void PrepareNew() {
        // Switch pointers
        if(pNew == ITypes1) {
            pNew = ITypes2;
            pOld = ITypes1;
        }
        else {
            pNew = ITypes1;
            pOld = ITypes2;
        }
        // Clear new
        for(int i=0; i<TYPE_CNT; i++) pNew[i] = 0;
    }

    uint32_t IsNear(uint8_t Type) { return pNew[Type]; }
} TypesAround;

class Indication_t {
private:
    int32_t AriTime = 0, KaesuTime = 0, ChangedTime = 0;
    CircBuf_t<const LedRGBChunk_t*, 18> Que;
    const LedRGBChunk_t* Lsqs[TYPE_CNT] = {
            lsqAri, lsqKaesu, lsqNorth, lsqNorthStrong, lsqSouth, lsqSouthStrong, lsqNorthCursed, lsqSouthCursed
    };

public:
    void ShowSelfType() { Led.StartOrRestart(Lsqs[Cfg.Type]); }

    void DoAriAppear() {
        AriTime = ARI_KAESU_IND_TIME_S;
        Vibro.StartIfIdle(VIBRO_ARI_KAESU);
    }
    void DoKaesuAppear() {
        KaesuTime = ARI_KAESU_IND_TIME_S;
        Vibro.StartIfIdle(VIBRO_ARI_KAESU);
    }
    void DoChanged() {
        if(ChangedTime == 0) ChangedTime = CHANGED_IND_TIME_S;
        Vibro.StartIfIdle(VIBRO_SMTH_CHANGED);
    }

    void Tick() {
        if(AriTime != 0 or KaesuTime != 0) Vibro.StartIfIdle(VIBRO_ARI_KAESU);
        else if(ChangedTime != 0) Vibro.StartIfIdle(VIBRO_SMTH_CHANGED);
        // Time
        if(AriTime != 0) {
            TypesAround.IncCnt(TYPE_ARI);
            AriTime--;
        }
        if(KaesuTime != 0) {
            TypesAround.IncCnt(TYPE_KAESU);
            KaesuTime--;
        }
        if(ChangedTime != 0) ChangedTime--;
    }

    void ShowWhoIsNear() {
        // Ari & Kaesu
        if(TypesAround.IsNear(TYPE_ARI)) Que.PutIfNotOverflow(lsqAri);
        if(TypesAround.IsNear(TYPE_KAESU)) Que.PutIfNotOverflow(lsqKaesu);
        // Others
        if(Cfg.IsStrong()) {
            for(int i=2; i<TYPE_CNT; i++) {
                uint32_t Cnt = TypesAround.IsNear(i);
                switch(Cnt) {
                    case 0: break;
                    case 1: Que.PutIfNotOverflow(Lsqs[i]); break;
                    case 2:
                        Que.PutIfNotOverflow(Lsqs[i]);
                        Que.PutIfNotOverflow(Lsqs[i]);
                        break;
                    default:
                        Que.PutIfNotOverflow(Lsqs[i]);
                        Que.PutIfNotOverflow(Lsqs[i]);
                        Que.PutIfNotOverflow(Lsqs[i]);
                        break;
                } // switch
            } // for
        }
        ProcessQue();
    }

    void ProcessQue() {
        const LedRGBChunk_t* Seq;
        if(Que.Get(&Seq) == retvOk) Led.StartOrRestart(Seq);
    }

    void FlushQueue() { Que.Flush(); }

    void AddVisible() { Que.PutIfNotOverflow(lsqVisible); }
    void AddHidden()  { Que.PutIfNotOverflow(lsqHidden); }
    void AddSilent()  { Que.PutIfNotOverflow(lsqSilent); }
} Indi;

void BeSilent() {
    TimeToBeSilent = TIME_TO_BE_SILENT_S;
    Cfg.MustTxInEachOther = false;
}

void CheckRxTable() {
    // Analyze table: get count of every type near
    TypesAround.PrepareNew();
    RxTable_t& Tbl = Radio.GetRxTable();
    for(uint32_t i=0; i<Tbl.Cnt; i++) { // i is just number of pkt in table, no relation with type
        rPkt_t Pkt = Tbl[i];
        if(Pkt.Type <= (TYPE_CNT-1)) { // Type is ok
            // Process Ari and Kaesu, do not add to list
            if(Pkt.Type == TYPE_ARI) {
                if((Cfg.IsNorth() and Pkt.RCmd == RCMD_NORTH_ARI_KAESU) or (Cfg.IsSouth() and Pkt.RCmd == RCMD_SOUTH_ARI_KAESU)) {
                    TypesAround.IncCnt(Pkt.Type);
                    Indi.DoAriAppear();
                }
                else if(Pkt.RCmd == RCMD_BESILENT) BeSilent();
            }
            else if(Pkt.Type == TYPE_KAESU) {
                if((Cfg.IsNorth() and Pkt.RCmd == RCMD_NORTH_ARI_KAESU) or (Cfg.IsSouth() and Pkt.RCmd == RCMD_SOUTH_ARI_KAESU)) {
                    TypesAround.IncCnt(Pkt.Type);
                    Indi.DoKaesuAppear();
                }
                else if(Pkt.RCmd == RCMD_BESILENT) BeSilent();
            }
            else { // Some other
                if(TimeToBeSilent == 0) { // Do not react even if something is received
                    // Check if Attack/Retreat
                    if(Cfg.IsNorth()) {
                        if     (Pkt.RCmd == RCMD_NORTH_ATTACK)  Vibro.StartIfIdle(vsqAttack);
                        else if(Pkt.RCmd == RCMD_NORTH_RETREAT) Vibro.StartIfIdle(vsqRetreat);
                    }
                    else if(Cfg.IsSouth()) {
                        if     (Pkt.RCmd == RCMD_SOUTH_ATTACK)  Vibro.StartIfIdle(vsqAttack);
                        else if(Pkt.RCmd == RCMD_SOUTH_RETREAT) Vibro.StartIfIdle(vsqRetreat);
                    }
                    // Add to list
                    if(Pkt.Rssi > GOOD_DISTANCE_dBm) TypesAround.IncCnt(Pkt.Type);
                }
            }
        }
    }

    // Check if Others changed
//    if(TypesAround.HasChangedOthers()) {
//        Indi.DoChanged();
//    }
}

void ProcessButtonsAriKaesu(uint8_t BtnID, BtnEvt_t Type) {
    if(Type != beLongPress) return;
    Vibro.StartOrRestart(vsqBrrBrr);
    if(BtnID == 0) {
        Radio.PktTxFar.RCmd = RCMD_NORTH_ARI_KAESU;
        Indi.ShowSelfType();
        Radio.DoTransmitFar(900);
    }
    else if(BtnID == 1) {
        Radio.PktTxFar.RCmd = RCMD_SOUTH_ARI_KAESU;
        Indi.ShowSelfType();
        Radio.DoTransmitFar(900);
    }
    else if(BtnID == 2) {
        Radio.PktTxFar.RCmd = RCMD_BESILENT;
        Led.StartOrRestart(lsqHidden);
        Radio.DoTransmitFar(900);
    }
}

void ProcessButtonsOthers(uint8_t BtnID, BtnEvt_t Type) {
    if(BtnID == 0) {
        if(Type == beShortPress) {
            // Show visibility
            if(TimeToBeSilent != 0) {
                Indi.AddSilent();
                Indi.ProcessQue();
            }
            else {
                if(TimeToBeHidden == 0) Indi.AddVisible();
                else Indi.AddHidden();
                Indi.ShowWhoIsNear();
            }
        }
        else if(Type == beLongPress and Cfg.IsStrong()) {
            if(TimeToBeHidden == 0) { // Be hidden
                TimeToBeHidden = TIME_TO_BE_HIDDEN_S;
                Cfg.MustTxInEachOther = false;
                Led.StartOrRestart(lsqHidden);
            }
            else { // Be visible
                TimeToBeHidden = 0;
                Cfg.MustTxInEachOther = true;
                Led.StartOrRestart(lsqVisible);
            }
        }
    }
    // Retreat
    else if(BtnID == 1 and Type == beLongPress) {
        Vibro.StartOrRestart(vsqBrrBrr);
        if(Cfg.IsNorth()) Radio.PktTxFar.RCmd = RCMD_NORTH_RETREAT;
        else if(Cfg.IsSouth()) Radio.PktTxFar.RCmd = RCMD_SOUTH_RETREAT;
        Radio.DoTransmitFar(90);
    }
    // Attack
    else if(BtnID == 2 and Type == beLongPress) {
        Vibro.StartOrRestart(vsqBrrBrr);
        if(Cfg.IsNorth()) Radio.PktTxFar.RCmd = RCMD_NORTH_ATTACK;
        else if(Cfg.IsSouth()) Radio.PktTxFar.RCmd = RCMD_SOUTH_ATTACK;
        Radio.DoTransmitFar(90);
    }
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
    Printf("\r%S %S; ID=%u\r", APP_NAME, XSTRINGIFY(BUILD_TIME), Cfg.ID);
//    Printf("\r%X\t%X\t%X\r", GetUniqID1(), GetUniqID2(), GetUniqID3());
//    if(Sleep::WasInStandby()) {
//        Uart.Printf("WasStandby\r");
//        Sleep::ClearStandbyFlag();
//    }
    Clk.PrintFreqs();
    Random::Seed(GetUniqID3());   // Init random algorythm with uniq ID

    Printf("PktSz: %u\r", RPKT_LEN);

    Led.Init();
    Led.SetupSeqEndEvt(evtIdLedSeqDone);
    Vibro.Init();
//    Vibro.SetupSeqEndEvt(evtIdVibroSeqDone);
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

    ReadAndSetupMode();

    // ==== Radio ====
    if(Radio.Init() != retvOk) {
        Led.StartOrRestart(lsqFailure);
        chThdSleepMilliseconds(1008);
    }

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
                if(TimeS % 4 == 0) CheckRxTable();
                Indi.Tick();
                // Hidden & Silent
                if(TimeToBeHidden != 0) {
                    TimeToBeHidden--;
                    if(TimeToBeHidden == 0 and TimeToBeSilent == 0) Cfg.MustTxInEachOther = true;
                }
                if(TimeToBeSilent != 0) {
                    TimeToBeSilent--;
                    if(TimeToBeHidden == 0 and TimeToBeSilent == 0) Cfg.MustTxInEachOther = true;
                }
                break;

#if BUTTONS_ENABLED
            case evtIdButtons:
                Printf("Btn %u %u\r", Msg.BtnEvtInfo.BtnID, Msg.BtnEvtInfo.Type);
                if(Cfg.IsAriKaesu()) ProcessButtonsAriKaesu(Msg.BtnEvtInfo.BtnID, Msg.BtnEvtInfo.Type);
                else ProcessButtonsOthers(Msg.BtnEvtInfo.BtnID, Msg.BtnEvtInfo.Type);
                break;
#endif

            case evtIdLedSeqDone:
                Indi.ProcessQue(); // proceed with indication if current completed
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
    Printf("Dip: 0x%02X; ", b);
    OldDipSettings = b;
    // Reset everything
    Indi.FlushQueue();
    Vibro.Stop();
    Led.Stop();
    // Select self type
    chSysLock();
    uint32_t Type = b >> 4;
//    if(SelfType >= LcktType.size()) SelfType = LcktType.size() - 1;
    chSysUnlock();
    // Select power
    b &= 0b1111; // Remove high bits
    Printf("Type: %u; Pwr: %u\r", Type, b);
    Cfg.SetSelfType(Type);
    Radio.PktTx.Type = Cfg.Type;
    Radio.PktTxFar.Type = Cfg.Type;
    Indi.ShowSelfType();
    Cfg.TxPower = (b > 11)? CC_PwrPlus12dBm : PwrTable[b];
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

    if(PCmd->NameIs("State")) {
        Printf("TTBS: %u; TTBH: %u\r", TimeToBeSilent, TimeToBeHidden);
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
    Radio.PktTx.ID = Cfg.ID;
    Radio.PktTxFar.ID = Cfg.ID;
}

uint8_t ISetID(int32_t NewID) {
    if(NewID < ID_MIN or NewID > ID_MAX) return retvFail;
    uint8_t rslt = EE::Write32(EE_ADDR_DEVICE_ID, NewID);
    if(rslt == retvOk) {
        Cfg.ID = NewID;
        Radio.PktTx.ID = Cfg.ID;
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
