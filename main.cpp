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
#include "adcL151.h"

#include "Config.h"

#if 1 // ======================== Variables and defines ========================
// Forever
EvtMsgQ_t<EvtMsg_t, MAIN_EVT_Q_LEN> EvtQMain;
static const UartParams_t CmdUartParams(115200, CMD_UART_PARAMS);
CmdUart_t Uart{&CmdUartParams};
static void ITask();
static void OnCmd(Shell_t *PShell);

static void ReadAndSetupMode();
extern uint32_t SupercyclesCntToCheckRxTable;

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
#endif

#if 1 // ========================== Logic ======================================
Config_t Cfg;
// RSSI to Color: [RSSI_MIN; RSSI_MAX] => [240; 0] == [Blue; Red]
#define RSSI_TO_SHOW    (-80L) // Min rssi to react to
#define RSSI_MIN        (-75L)
#define RSSI_MAX        (-54L)
#define HSV_H_MIN       240L // Blue
#define HSV_H_MAX       0L // Red

class Indication_t {
private:
    const LedRGBChunk_t* Lsqs[TYPE_CNT] = { lsqObserver, lsqTransmitter };
    LedRGBChunk_t lsqOne[3] = { {csSetup, SMOOTHVAL, clGreen}, {csSetup, SMOOTHVAL, clBlack}, {csEnd} };
    LedRGBChunk_t lsqTwo[5] = { {csSetup, SMOOTHVAL, clGreen}, {csSetup, SMOOTHVAL, clBlack}, {csSetup, SMOOTHVAL, clGreen}, {csSetup, SMOOTHVAL, clBlack}, {csEnd} };
    LedRGBChunk_t lsqThree[7] = { {csSetup, SMOOTHVAL, clGreen}, {csSetup, SMOOTHVAL, clBlack}, {csSetup, SMOOTHVAL, clGreen}, {csSetup, SMOOTHVAL, clBlack}, {csSetup, SMOOTHVAL, clMagenta}, {csSetup, SMOOTHVAL, clBlack}, {csEnd} };

    void Rssi2RGB(int32_t Rssi, Color_t *PClr) {
        if(Rssi > RSSI_MAX) Rssi = RSSI_MAX;
        else if(Rssi < RSSI_MIN) Rssi = RSSI_MIN;
        int32_t H = Proportion<int32_t>(RSSI_MIN, RSSI_MAX, HSV_H_MIN, HSV_H_MAX, Rssi);
        PClr->FromHSV(H, 100, 100);
    }

    uint32_t ICnt = 0;
public:
    void ShowSelfType() { Led.StartOrRestart(Lsqs[Cfg.Type]); }

    void ShowWhoIsNear(uint32_t Cnt, int32_t Rssi1, int32_t Rssi2) {
//        if(Cnt) Printf("%d; %d %d\r", Cnt, Rssi1, Rssi2);
        ICnt = Cnt;
        switch(Cnt) {
            case 0: return; break;
            case 1:
                Rssi2RGB(Rssi1, &lsqOne[0].Color);
                Led.StartOrRestart(lsqOne);
                break;
            case 2:
                Rssi2RGB(Rssi1, &lsqTwo[0].Color);
                Rssi2RGB(Rssi2, &lsqTwo[2].Color);
                Led.StartOrRestart(lsqTwo);
                break;
            default:
                Rssi2RGB(Rssi1, &lsqThree[0].Color);
                Rssi2RGB(Rssi2, &lsqThree[2].Color);
                Led.StartOrRestart(lsqThree);
                break;
        }
        if(Cnt != 0) Vibro.StartOrRestart(vsqBrr);
    }
} Indi;

void CheckRxTable() {
    uint32_t Cnt = 0;
    int32_t Rssi1 = -111, Rssi2 = -111;
    RxTable_t &Tbl = Radio.GetRxTable();
    for(uint32_t i=0; i<RXTABLE_SZ; i++) {
        if(Tbl[i].Cnt) {
            int32_t Rssi = Tbl[i].Rssi / Tbl[i].Cnt;
//            Printf("ID: %u; Rssi: %d\r", i, Rssi);
            if(Rssi > RSSI_TO_SHOW) {
                Cnt++;
                if(Rssi > Rssi2) Rssi2 = Rssi;
                if(Rssi2 > Rssi1) { // Switch Rssi1 and Rssi2
                    Rssi2 = Rssi1;
                    Rssi1 = Rssi;
                }
            }
        }
    }
    Tbl.CleanUp();
    Indi.ShowWhoIsNear(Cnt, Rssi1, Rssi2);
    if(Cnt <= 2) SupercyclesCntToCheckRxTable = 4;
    else SupercyclesCntToCheckRxTable = 6;
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
    Clk.PrintFreqs();
    Random::Seed(GetUniqID3());   // Init random algorythm with uniq ID

    Led.Init();
    Vibro.Init();

#if ADC_REQUIRED
    Adc.Init();
#endif

#if BEEPER_ENABLED
    Beeper.Init();
    Beeper.StartOrRestart(bsqBeepBeep);
    chThdSleepMilliseconds(702);    // Let it complete the show
#endif
#if BUTTONS_ENABLED
    SimpleSensors::Init();
#endif
#if PILL_ENABLED
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
                ReadAndSetupMode();
                break;

            case evtIdCheckRxTable:
                if(Cfg.Type == TYPE_OBSERVER) CheckRxTable();
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
    Vibro.Stop();
    Led.Stop();
    // Select type to tx and show
    Cfg.Type = (b & 0x80)? TYPE_TRANSMITTER : TYPE_OBSERVER;
    // Select power
    b &= 0b1111; // Remove high bits = group 5678
    Cfg.TxPower = (b > 11)? CC_PwrPlus12dBm : PwrTable[b];
    Printf("Type: %u; Pwr: %S\r", Cfg.Type, CC_PwrToString(Cfg.TxPower));
    Indi.ShowSelfType();
}

#if 1 // ================= Command processing ====================
void OnCmd(Shell_t *PShell) {
	Cmd_t *PCmd = &PShell->Cmd;
    // Handle command
    if(PCmd->NameIs("Ping")) PShell->Ok();
    else if(PCmd->NameIs("Version")) PShell->Print("%S %S\r", APP_NAME, XSTRINGIFY(BUILD_TIME));
    else if(PCmd->NameIs("GetID")) PShell->Print("ID: %u\r", Cfg.ID);
#if ADC_REQUIRED
    else if(PCmd->NameIs("GetBat")) Adc.StartMeasurement();
#endif

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
