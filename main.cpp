/*
 * main.cpp
 *
 *  Created on: 20 февр. 2014 г.
 *      Author: g.kruglov
 */

#include "main.h"
#include "SimpleSensors.h"
#include "buttons.h"
#include "board.h"
#include "led.h"
#include "vibro.h"
#include "beeper.h"
#include "Sequences.h"
#include "radio_lvl1.h"
#include "kl_adc.h"
#include "pill.h"

#if 1 // ======================== Variables and defines ========================
App_t App;

// EEAddresses
#define EE_ADDR_DEVICE_ID       0
#define EE_ADDR_HEALTH_STATE    8

static const PinInputSetup_t DipSwPin[DIP_SW_CNT] = { DIP_SW6, DIP_SW5, DIP_SW4, DIP_SW3, DIP_SW2, DIP_SW1 };
static uint8_t GetDipSwitch();
static void ReadAndSetupMode();
static uint8_t ISetID(int32_t NewID);
Eeprom_t EE;
void ReadIDfromEE();

// ==== Periphery ====
Vibro_t Vibro {VIBRO_PIN};
#if BEEPER_ENABLED
Beeper_t Beeper {BEEPER_PIN};
#endif

LedRGBwPower_t Led { LED_R_PIN, LED_G_PIN, LED_B_PIN, LED_EN_PIN };

// ==== Timers ====
static TmrKL_t TmrEverySecond {MS2ST(144), EVT_EVERY_SECOND, tktPeriodic};
static TmrKL_t TmrRxTableCheck {MS2ST(2007), EVT_RXCHECK, tktPeriodic};
static uint32_t TimeS;

static int32_t Dbg32;
#endif

#if 1 // ==================== Application-specific objects =====================
#define C_MENACE_END_S      180
#define C_DANGER_END_S      300
#define C_PANIC_END_S       360 // Also duration of cataclysm, too

enum CState_t {cstIdle=0, cstMenace=1, cstDanger=2, cstPanic=3};
enum CStateChange_t {cscNone=0, cscI2M=1, cscI2D=2, cscI2P=3, cscM2D=4, cscD2P=5, cscEnd=6};

class Cataclysm_t {
private:
    uint32_t TimeElapsed = 0;
    uint32_t TimeOfStart;
    void ProcessTimeElapsed() {
        // Calculate state by TimeElapsed
        CState_t NewState = cstMenace;
        if(TimeElapsed > C_PANIC_END_S) NewState = cstIdle;
        else if(TimeElapsed > C_DANGER_END_S) NewState = cstPanic;
        else if(TimeElapsed > C_MENACE_END_S) NewState = cstDanger;
//        Uart.Printf("Elapsed=%u; St=%u; Nst=%u\r", TimeElapsed, State, NewState);
        // Check if changed
        StateChange = cscNone;
        if(NewState != State) {
            // Calculate state transition
            switch(State) {
                case cstIdle:
                    switch(NewState) {
                        case cstMenace: StateChange = cscI2M; break;
                        case cstDanger: StateChange = cscI2D; break;
                        case cstPanic:  StateChange = cscI2P; break;
                        default: break;
                    }
                    break;

                case cstMenace: if(NewState == cstDanger) StateChange = cscM2D; break;
                case cstDanger: if(NewState == cstPanic)  StateChange = cscD2P; break;
                case cstPanic:  if(NewState == cstIdle)   StateChange = cscEnd; break;
            } // switch state
            State = NewState;
            App.SignalEvt(EVT_CSTATE_CHANGE);
        }
    }
public:
    CState_t State = cstIdle;
    CStateChange_t StateChange = cscNone;
    void ProcessSignal(uint32_t ATimeElapsed) {
//        if(State != cstIdle) return;    // Do not update time if Cataclysm is ongoing
        TimeElapsed = ATimeElapsed;
        TimeOfStart = TimeS - TimeElapsed;  // Calculate Time Of Start (needed for buttonpress process)
//        Uart.Printf("TimeOfStart=%u\r", TimeOfStart);
        ProcessTimeElapsed();               // Get state and signal if needed
    }
    void Tick1S() {
        if(State == cstIdle) return; // Nothing to do here
        TimeElapsed++;
//        Uart.Printf("Elapsed=%u\r", TimeElapsed);
        ProcessTimeElapsed();        // Get state and signal if needed
    }
    //bool HappenedAfterBeginning(uint32_t
};
static Cataclysm_t Cataclysm;

bool IsInShelter = false;
PillType_t PillType;

enum HState_t {hstHealthy, hstIll, hstDead};

class Health_t {
public:
    HState_t State = hstHealthy;
    void Load() {}
    void Save() {}
    void ProcessCChange(CStateChange_t CstCh, bool AIsInShelter) {
        HState_t NewState = State;
        switch(State) {
            case hstHealthy:
                // Become ill if met Danger or Panic state not being in shelter
                if(ANY_OF_3(CstCh, cscI2D, cscI2P, cscM2D) and !AIsInShelter) NewState = hstIll;
                break;
            case hstIll:
                // Die if met with Cataclysm End not being in shelter, or if met Cataclysm Start being ill
                if((CstCh == cscEnd and !AIsInShelter) or ANY_OF_3(CstCh, cscI2M, cscI2D, cscI2P)) NewState = hstDead;
                break;
            case hstDead: break; // In this case, Cataclysm changes nothing
        }
        if(State != NewState) {
            State = NewState;
            App.SignalEvt(EVT_INDICATION);
        }
        Uart.Printf("HState1: %u\r", State);
    }
    void ProcessPill(PillType_t APillType) {
        Uart.Printf("HPill: %u\r", APillType);
        HState_t NewState = State;
        switch(State) {
            case hstHealthy: break;
            case hstIll:
                if(ANY_OF_2(APillType, ptCure, ptPanacea)) NewState = hstHealthy;
                break;
            case hstDead:
                if(APillType == ptPanacea) NewState = hstHealthy;
                break;
        } // switch
        if(State != NewState) {
            State = NewState;
            App.SignalEvt(EVT_INDICATION);
        }
        Uart.Printf("HState2: %u\r", State);
    }
};
static Health_t Health;
#endif

int main(void) {
    // ==== Init Vcore & clock system ====
    SetupVCore(vcore1V5);
//    Clk.SetMSI4MHz();
    Clk.UpdateFreqValues();

    // === Init OS ===
    halInit();
    chSysInit();
    App.InitThread();

    // ==== Init hardware ====
    Uart.Init(115200, UART_GPIO, UART_TX_PIN, UART_GPIO, UART_RX_PIN);
    ReadIDfromEE();
    Uart.Printf("\r%S %S; ID=%u\r", APP_NAME, BUILD_TIME, App.ID);
//    Uart.Printf("ID: %X %X %X\r", GetUniqID1(), GetUniqID2(), GetUniqID3());
//    if(Sleep::WasInStandby()) {
//        Uart.Printf("WasStandby\r");
//        Sleep::ClearStandbyFlag();
//    }
    Clk.PrintFreqs();
//    RandomSeed(GetUniqID3());   // Init random algorythm with uniq ID

    // === LED ===
    Led.Init();
//    Led.SetupSeqEndEvt(chThdGetSelfX(), EVT_LED_SEQ_END);

    Vibro.Init();

#if BEEPER_ENABLED // === Beeper ===
    Beeper.Init();
    Beeper.StartOrRestart(bsqBeepBeep);
    chThdSleepMilliseconds(702);    // Let it complete the show
#endif

//    Adc.Init();

#if PILL_ENABLED // === Pill ===
//    i2c1.Init();
//    PillMgr.Init();
//    TmrCheckPill.InitAndStart();
#endif

#if BTN_ENABLED
    PinSensors.Init();
#endif

    Health.Load();

    TmrEverySecond.InitAndStart();
    TimeS = C_PANIC_END_S + 9;    // if cataclysm would start immideately
//    TmrRxTableCheck.InitAndStart();
    App.SignalEvt(EVT_EVERY_SECOND); // check it now

    if(Radio.Init() != OK) {
//        Led.StartOrRestart(lsqFailure);
//        chThdSleepMilliseconds(1008);
    }

    // Main cycle
    App.ITask();
}

__noreturn
void App_t::ITask() {
    while(true) {
        __unused eventmask_t Evt = chEvtWaitAny(ALL_EVENTS);
        if(Evt & EVT_EVERY_SECOND) {
            TimeS++;
//            Uart.Printf("t=%u\r", TimeS);
            Cataclysm.Tick1S();
//            ReadAndSetupMode();
        }

#if PILL_ENABLED // ==== Pill ====
        if(Evt & EVT_PILL_CHECK) {
            Health.ProcessPill(PillType);
//            PillMgr.Check();
//            switch(PillMgr.State) {
//                case pillJustConnected:
//                    Uart.Printf("Pill: %d %d\r", PillMgr.Pill.TypeInt32, PillMgr.Pill.ChargeCnt);
//                    break;
////                case pillJustDisconnected: Uart.Printf("Pill Discon\r"); break;
//                default: break;
//            }
        }
#endif

#if 0 // ==== Led sequence end ====
        if(Evt & EVT_LED_SEQ_END) {
        }
#endif

#if BTN_ENABLED
        if(Evt & EVT_BUTTONS) {
            Uart.Printf("Btn\r");
            BtnEvtInfo_t EInfo;
            while(BtnGetEvt(&EInfo) == OK) {
                if(EInfo.Type == bePress) {
                    if(Mode == modeTx) {
                        if(Radio.MustTx == false) {
                            Radio.MustTx = true;
                            Led.StartOrContinue(lsqTx);
                        }
                        else {
                            Radio.MustTx = false;
                            Led.StartOrRestart(lsqModeTx);
                        }
                    } // if TX
                    // Player
                    else {

                    }
                } // if Press
            } // while
        }
#endif

        if(Evt & EVT_RX) {
            Uart.Printf("RX %u\r", Dbg32);
            Cataclysm.ProcessSignal(Dbg32);
        }

        if(Evt & EVT_RXCHECK) {
//            if(ShowAliens == true) {
//                if(Radio.RxTable.GetCount() != 0) {
//                    Led.StartOrContinue(lsqTheyAreNear);
//                    Radio.RxTable.Clear();
//                }
//                else Led.StartOrContinue(lsqTheyDissapeared);
//            }
        }

//        if(Evt & EVT_OFF) {
////            Uart.Printf("Off\r");
//            chSysLock();
//            Sleep::EnableWakeup1Pin();
//            Sleep::EnterStandby();
//            chSysUnlock();
//        }

#if 1 // ==== Cataclysm state changed ====
        if(Evt & EVT_CSTATE_CHANGE) {
            Uart.Printf("C chng: %u\r", Cataclysm.StateChange);
            Health.ProcessCChange(Cataclysm.StateChange, IsInShelter);
        }
#endif

#if 0 // ==== Vibro seq end ====
        if(Evt & EVT_VIBRO_END) {
            // Restart vibration (or start new one) if needed
            if(pVibroSeqToPerform != nullptr) Vibro.StartSequence(pVibroSeqToPerform);
        }
#endif

#if UART_RX_ENABLED
        if(Evt & EVT_UART_NEW_CMD) {
            OnCmd((Shell_t*)&Uart);
            Uart.SignalCmdProcessed();
        }
#endif

    } // while true
} // App_t::ITask()

__unused
void ReadAndSetupMode() {
    static uint8_t OldDipSettings = 0xFF;
    uint8_t b = GetDipSwitch();
    if(b == OldDipSettings) return;
    // Something has changed
    Radio.MustTx = false; // Just in case
    // Reset everything
    Vibro.Stop();
    Led.Stop();
    // Analyze switch
    OldDipSettings = b;
    Uart.Printf("Dip: %02X\r", b);
    Vibro.StartOrRestart(vsqBrr);
    if(b == 0) {
        App.Mode = modePlayer;
        Led.StartOrRestart(lsqModePlayerStart);
        chThdSleepMilliseconds(720);    // Let blink end
        // Get Health from EE
        // TODO

        Led.StartOrRestart(lsqModeTx);  // Start Idle indication
}
    else {
        App.Mode = modeTx;
        Led.StartOrRestart(lsqModeTxStart);
        chThdSleepMilliseconds(720);    // Let blink end
        Led.StartOrRestart(lsqModeTx);  // Start Idle indication
    }
}


#if UART_RX_ENABLED // ================= Command processing ====================
void App_t::OnCmd(Shell_t *PShell) {
	Cmd_t *PCmd = &PShell->Cmd;
    __attribute__((unused)) int32_t dw32 = 0;  // May be unused in some configurations
//    Uart.Printf("%S\r", PCmd->Name);
    // Handle command
    if(PCmd->NameIs("Ping")) {
        PShell->Ack(OK);
    }

    else if(PCmd->NameIs("GetID")) PShell->Reply("ID", App.ID);

    else if(PCmd->NameIs("SetID")) {
        if(PCmd->GetNextInt32(&dw32) != OK) { PShell->Ack(CMD_ERROR); return; }
        uint8_t r = ISetID(dw32);
        PShell->Ack(r);
    }

    else if(PCmd->NameIs("Evt")) {
        if(PCmd->GetNextInt32(&dw32) != OK) { PShell->Ack(CMD_ERROR); return; }
        App.SignalEvt(dw32);
    }

    else if(PCmd->NameIs("RX")) {
        if(PCmd->GetNextInt32(&Dbg32) != OK) { PShell->Ack(CMD_ERROR); return; }
        App.SignalEvt(EVT_RX);
    }
    else if(PCmd->NameIs("cst")) {
        if(PCmd->GetNextInt32(&dw32) != OK) { PShell->Ack(CMD_ERROR); return; }
        Cataclysm.StateChange = (CStateChange_t)dw32;   // State transition
        if(PCmd->GetNextInt32(&dw32) != OK) { PShell->Ack(CMD_ERROR); return; }
        IsInShelter = dw32;             // Is in shelter
        App.SignalEvt(EVT_CSTATE_CHANGE);
    }
    else if(PCmd->NameIs("Pill")) {
        if(PCmd->GetNextInt32(&dw32) != OK) { PShell->Ack(CMD_ERROR); return; }
        PillType = (PillType_t)dw32;
        App.SignalEvt(EVT_PILL_CHECK);
    }

    else PShell->Ack(CMD_UNKNOWN);
}
#endif

#if 1 // =========================== ID management =============================
void ReadIDfromEE() {
    App.ID = EE.Read32(EE_ADDR_DEVICE_ID);  // Read device ID
    if(App.ID < ID_MIN or App.ID > ID_MAX) {
        Uart.Printf("\rUsing default ID\r");
        App.ID = ID_DEFAULT;
    }
}

uint8_t ISetID(int32_t NewID) {
    if(NewID < ID_MIN or NewID > ID_MAX) return FAILURE;
    uint8_t rslt = EE.Write32(EE_ADDR_DEVICE_ID, NewID);
    if(rslt == OK) {
        App.ID = NewID;
        Uart.Printf("New ID: %u\r", App.ID);
        return OK;
    }
    else {
        Uart.Printf("EE error: %u\r", rslt);
        return FAILURE;
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
