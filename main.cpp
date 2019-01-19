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

DevType_t DeviceType = devtNone;
PillType_t PillType = pilltNone;

LedRGBwPower_t Led { LED_R_PIN, LED_G_PIN, LED_B_PIN, LED_EN_PIN };

// ==== Timers ====
static TmrKL_t TmrEverySecond {MS2ST(1000), evtIdEverySecond, tktPeriodic};
//static TmrKL_t TmrRxTableCheck {MS2ST(2007), evtIdCheckRxTable, tktPeriodic};
static int32_t TimeS;
static int32_t IndicationTimeout = 0;
#endif

int main(void) {
    // ==== Init Vcore & clock system ====
    SetupVCore(vcore1V2);
//    Clk.SetMSI4MHz();
    Clk.UpdateFreqValues();

    // === Init OS ===
    halInit();
    chSysInit();
    EvtQMain.Init();

    // ==== Init hardware ====
    Uart.Init();
//    ReadIDfromEE();
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
    Vibro.Init();
    Vibro.StartOrRestart(vsqBrrBrr);
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

    // ==== Time and timers ====
    TmrEverySecond.StartOrRestart();
//    TmrRxTableCheck.StartOrRestart();

    // ==== Radio ====
//    if(Radio.Init() == retvOk) Led.StartOrRestart(lsqStart);
//    else Led.StartOrRestart(lsqFailure);
//    chThdSleepMilliseconds(1008);

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
                if(IndicationTimeout > 0) {
                    if(--IndicationTimeout == 0) {
                        Led.Stop();
                        Vibro.Stop();
                    }
                }
                break;

#if BUTTONS_ENABLED
            case evtIdButtons: {
                Printf("Btn %u\r", Msg.BtnEvtInfo.BtnID);
                int32_t Zero = 0;
                if(DeviceType == devtParalyzer and PillType == pilltParalyzer) {
                    PillMgr.Write(0, &Zero, 4);
                    Led.StartOrRestart(lsqParalyzer);
                    IndicationTimeout = 10;
                }
                else if(DeviceType == devtRegen and PillType == pilltRegen) {
                    PillMgr.Write(0, &Zero, 4);
                    Led.StartOrRestart(lsqRegen);
                    Vibro.StartOrRestart(vsqRegen);
                    IndicationTimeout = 600;
                }
                else if(DeviceType == devtAccelerator and PillType == pilltAccelerator) {
                    PillMgr.Write(0, &Zero, 4);
                    Led.StartOrRestart(lsqAccel);
                    Vibro.StartOrRestart(vsqRegen);
                    IndicationTimeout = 10;
                }
                else if(PillType == pilltBlow) {
                    PillMgr.Write(0, &Zero, 4);
                    Led.StartOrRestart(lsqBlow);
                    Vibro.StartOrRestart(vsqBlow);
                    IndicationTimeout = 60;
                }
                PillType = pilltNone;
            } break;
#endif

//        if(Evt & EVT_RX) {
//            int32_t TimeRx = Radio.PktRx.Time;
//            Uart.Printf("RX %u\r", TimeRx);
//            Cataclysm.ProcessSignal(TimeRx);
//        }

            case evtIdCheckRxTable: {
                uint32_t Cnt = Radio.RxTable.GetCount();
                switch(Cnt) {
                    case 0: Vibro.Stop(); break;
                    case 1: Vibro.StartOrContinue(vsqBrr); break;
                    case 2: Vibro.StartOrContinue(vsqBrrBrr); break;
                    default: Vibro.StartOrContinue(vsqBrrBrrBrr); break;
                }
                Radio.RxTable.Clear();
            } break;

#if PILL_ENABLED // ==== Pill ====
            case evtIdPillConnected:
                Printf("Pill: %u\r", PillMgr.Pill.DWord32);
                Led.StartOrRestart(lsqOnPillConnect);
                if(PillMgr.Pill.DWord32 >= pilltNone and PillMgr.Pill.DWord32 <= pilltBlow) {
                    PillType = (PillType_t)PillMgr.Pill.DWord32;
                }
                break;

            case evtIdPillDisconnected:
                Printf("Pill disconn\r");
//                Led.StartOrRestart(lsqNoPill);
                PillType = pilltNone;
                break;
#endif

#if 0 // ==== Vibro seq end ====
        if(Evt & EVT_VIBRO_END) {
            // Restart vibration (or start new one) if needed
            if(pVibroSeqToPerform != nullptr) Vibro.StartSequence(pVibroSeqToPerform);
        }
#endif
#if 0 // ==== Led sequence end ====
        if(Evt & EVT_LED_SEQ_END) {
        }
#endif
        //        if(Evt & EVT_OFF) {
        ////            Uart.Printf("Off\r");
        //            chSysLock();
        //            Sleep::EnableWakeup1Pin();
        //            Sleep::EnterStandby();
        //            chSysUnlock();
        //        }

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
    Printf("Dip: 0x%02X\r", b);
    OldDipSettings = b;

    if(b >= devtParalyzer and b <= devtLight) {
        DeviceType = (DevType_t)b;
        // XXX
//        switch(DeviceType) {
//            case devtNone: break;
//            case devtParalyzer: Led.StartOrRestart(lsqParalyzerStart); break;
//            case devtRegen: Led.StartOrRestart(lsqRegenStart); break;
//            case devtAccelerator: Led.StartOrRestart(lsqAccelStart); break;
//            case devtLight: Led.StartOrRestart(lsqLightStart); break;
//        }
    }


    // Reset everything
//    Vibro.Stop();
//    Led.Stop();
    // Select mode
//    if(b & 0b100000) {
//        Led.StartOrRestart(lsqTx);
    // Select power
//    b &= 0b11111; // Remove high bits
//    RMsg_t msg;
//    msg.Cmd = R_MSG_SET_PWR;
//    msg.Value = (b > 11)? CC_PwrPlus12dBm : PwrTable[b];
//    Radio.RMsgQ.SendNowOrExit(msg);
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

//    else if(PCmd->NameIs("PillRead32")) {
//        int32_t Cnt = 0;
//        if(PCmd->GetNext<int32_t>(&Cnt) != OK) { PShell->Ack(CMD_ERROR); return; }
//        uint8_t MemAddr = 0, b = OK;
//        PShell->Printf("#PillData32 ");
//        for(int32_t i=0; i<Cnt; i++) {
//            b = PillMgr.Read(MemAddr, &dw32, 4);
//            if(b != OK) break;
//            PShell->Printf("%d ", dw32);
//            MemAddr += 4;
//        }
//        Uart.Printf("\r\n");
//        PShell->Ack(b);
//    }
//
//    else if(PCmd->NameIs("PillWrite32")) {
//        uint8_t b = CMD_ERROR;
//        uint8_t MemAddr = 0;
//        // Iterate data
//        while(true) {
//            if(PCmd->GetNextInt32(&dw32) != OK) break;
////            Uart.Printf("%X ", Data);
//            b = PillMgr.Write(MemAddr, &dw32, 4);
//            if(b != OK) break;
//            MemAddr += 4;
//        } // while
//        Uart.Ack(b);
//    }
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
