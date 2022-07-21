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
static uint32_t TimeS;
#endif

#if 1 // ========================== Logic ======================================
#define ARI_KAESU_IND_TIME_S        18
#define ARI_KAESU_TIME_DECREMENT    4

Config_t Cfg;

class Indication_t {
private:
    int32_t AriTime = 0, KaesuTime = 0;
    CircBuf_t<const LedRGBChunk_t*, 18> Que;
    const LedRGBChunk_t* Lsqs[TYPE_CNT] = { lsqAri, lsqKaesu, lsqNorth, lsqSouth, lsqVatey };
    int32_t PrevTypes[TYPE_CNT] = {0};

    void PutNumberToQ(uint32_t Cnt, const LedRGBChunk_t* lsq) {
        switch(Cnt) {
            case 0: break; // Noone of such kind is around
            case 1:
                Que.PutIfNotOverflow(lsq);
                break;
            case 2:
                Que.PutIfNotOverflow(lsq);
                Que.PutIfNotOverflow(lsq);
                break;
            default:
                Que.PutIfNotOverflow(lsq);
                Que.PutIfNotOverflow(lsq);
                Que.PutIfNotOverflow(lsq);
                break;
        } // switch
    }
public:
    void ShowSelfType() {
        FlushQueue();
        Que.PutIfNotOverflow(Lsqs[Cfg.Type]);
        ProcessQue();
    }

    void ShowType(uint32_t AType) {
        FlushQueue();
        Que.PutIfNotOverflow(Lsqs[AType]);
        ProcessQue();
    }

    bool TypeHasChanged(uint32_t OldCnt, uint32_t NewCnt) {
        return ((OldCnt == 0 and NewCnt > 0) or (OldCnt > 0 and NewCnt == 0));
    }

    void ProcessTypesAround(uint32_t *PTypesNear) {
        // Ari / Kaesu
        if(PTypesNear[TYPE_ARI]) AriTime = ARI_KAESU_IND_TIME_S;
        if(PTypesNear[TYPE_KAESU]) KaesuTime = ARI_KAESU_IND_TIME_S;

        // === Show who is near ===
        // Ari & Kaesu
        if(AriTime > 0) {
            Que.PutIfNotOverflow(lsqAri);
            Vibro.StartOrContinue(vsqBrr);
            AriTime -= ARI_KAESU_TIME_DECREMENT;
        }
        if(KaesuTime > 0) {
            Que.PutIfNotOverflow(lsqKaesu);
            Vibro.StartOrContinue(vsqBrr);
            KaesuTime -= ARI_KAESU_TIME_DECREMENT;
        }

        if(!(AriTime > 0 or KaesuTime > 0)) {
            // Check changes
            bool HasChanged = false;
            if(TypeHasChanged(PrevTypes[TYPE_NORTH], PTypesNear[TYPE_NORTH])) HasChanged = true;
            if(TypeHasChanged(PrevTypes[TYPE_SOUTH], PTypesNear[TYPE_SOUTH])) HasChanged = true;
            if(TypeHasChanged(PrevTypes[TYPE_VATEY], PTypesNear[TYPE_VATEY])) HasChanged = true;
            PrevTypes[TYPE_NORTH] = PTypesNear[TYPE_NORTH];
            PrevTypes[TYPE_SOUTH] = PTypesNear[TYPE_SOUTH];
            PrevTypes[TYPE_VATEY] = PTypesNear[TYPE_VATEY];

            // Others
            PutNumberToQ(PTypesNear[TYPE_NORTH], lsqNorth);
            PutNumberToQ(PTypesNear[TYPE_SOUTH], lsqSouth);
            PutNumberToQ(PTypesNear[TYPE_VATEY], lsqVatey);
            if(HasChanged and Cfg.VibroEn) Vibro.StartIfIdle(vsqBrrBrr);
        }
        ProcessQue();

    }

    void ProcessQue() {
        const LedRGBChunk_t* Seq;
        if(Que.Get(&Seq) == retvOk) Led.StartOrRestart(Seq);
    }

    void FlushQueue() {
        Led.Stop();
        Que.Flush();
    }
} Indi;

void CheckRxTable() {
    // Analyze table: get count of every type near
    uint32_t TypesCnt[TYPE_CNT] = {0};
    RxTable_t *Tbl = Radio.GetRxTable();
    Tbl->ProcessCountingDistinctTypes(TypesCnt, TYPE_CNT);
    Indi.ProcessTypesAround(TypesCnt);
}

void ProcessButtonsAriKaesu(uint8_t BtnID, BtnEvt_t Type) {
    if(Type != beLongPress) return;
    Vibro.StartOrRestart(vsqBrrBrr);
    if(BtnID == 0) {
        Radio.PktTxFar.Type = TYPE_ARI;
        Indi.ShowType(TYPE_ARI);
        Radio.DoTransmitFar(900);
    }
    else if(BtnID == 1) {
        Radio.PktTxFar.Type = TYPE_KAESU;
        Indi.ShowType(TYPE_KAESU);
        Radio.DoTransmitFar(900);
    }
}
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
    ReadIDfromEE();
    Printf("\r%S %S; ID=%u\r", APP_NAME, XSTRINGIFY(BUILD_TIME), Cfg.ID);
//    Printf("\r%X\t%X\t%X\r", GetUniqID1(), GetUniqID2(), GetUniqID3());
//    if(Sleep::WasInStandby()) {
//        Printf("WasStandby\r");
//        Sleep::ClearStandbyFlag();
//    }
    Clk.PrintFreqs();
    Random::Seed(GetUniqID3());   // Init random algorythm with uniq ID

    Led.Init();
    Led.SetupSeqEndEvt(evtIdLedSeqDone);
    Vibro.Init();

#if BEEPER_ENABLED // === Beeper ===
    Beeper.Init();
    Beeper.StartOrRestart(bsqBeepBeep);
    chThdSleepMilliseconds(702);    // Let it complete the show
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

    ReadAndSetupMode();
    TmrEverySecond.StartOrRestart();
    Vibro.StartOrRestart(vsqBrrBrr);

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
                break;

#if BUTTONS_ENABLED
            case evtIdButtons:
                Printf("Btn %u %u\r", Msg.BtnEvtInfo.BtnID, Msg.BtnEvtInfo.Type);
                if(Cfg.IsAriKaesu()) ProcessButtonsAriKaesu(Msg.BtnEvtInfo.BtnID, Msg.BtnEvtInfo.Type);
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
    uint8_t t = (b >> 4) & 0b111; // Group 234
    Cfg.Type = (t > (TYPE_CNT-1))? (TYPE_CNT-1) : t;
    // Select VibroEn
    Cfg.VibroEn = b & 0x80;
    // Select power
    b &= 0b1111; // Remove high bits = group 5678
    Cfg.TxPower = (b > 11)? CC_PwrPlus12dBm : PwrTable[b];
    Printf("Type: %u; VibroEn: %u; Pwr: %S\r", Cfg.Type, Cfg.VibroEn, CC_PwrToString(Cfg.TxPower));
    Indi.ShowSelfType();
}

#if 1 // ================= Command processing ====================
void OnCmd(Shell_t *PShell) {
	Cmd_t *PCmd = &PShell->Cmd;
    // Handle command
    if(PCmd->NameIs("Ping")) PShell->Ok();
    else if(PCmd->NameIs("Version")) PShell->Print("%S %S\r", APP_NAME, XSTRINGIFY(BUILD_TIME));
    else if(PCmd->NameIs("GetID")) PShell->Print("ID: %u\r", Cfg.ID);

    else if(PCmd->NameIs("SetID")) {
        int32_t FID = 0;
        if(PCmd->GetNext<int32_t>(&FID) != retvOk) { PShell->CmdError(); return; }
        if(ISetID(FID) == retvOk) PShell->Ok();
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
    Cfg.ID = EE::Read32(EE_ADDR_DEVICE_ID);  // Read device ID
    if(Cfg.ID < ID_MIN or Cfg.ID > ID_MAX) {
        Printf("\rUsing default ID\r");
        Cfg.ID = ID_DEFAULT;
    }
}

uint8_t ISetID(int32_t NewID) {
    if(NewID < ID_MIN or NewID > ID_MAX) return retvFail;
    uint8_t rslt = EE::Write32(EE_ADDR_DEVICE_ID, NewID);
    if(rslt == retvOk) {
        Cfg.ID = NewID;
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
