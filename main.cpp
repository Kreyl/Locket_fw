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
#include "kl_i2c.h"
#include "pill.h"
#include "pill_mgr.h"

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
static TmrKL_t TmrEverySecond {MS2ST(1000), EVT_EVERY_SECOND, tktPeriodic};
static TmrKL_t TmrRxTableCheck {MS2ST(2007), EVT_RXCHECK, tktPeriodic};
static int32_t TimeS;
#endif

#if 1 // ==================== Application-specific objects =====================
#define C_MENACE_END_S      180
#define C_DANGER_END_S      300
#define C_PANIC_END_S       360 // Also duration of cataclysm, too
#define C_GAP_S             18  // Allow some unsync

enum CState_t {cstIdle=0, cstMenace=1, cstDanger=2, cstPanic=3};
enum CStateChange_t {cscNone=0, cscI2M=1, cscI2D=2, cscI2P=3, cscM2D=4, cscD2P=5, cscEnd=6};

class Cataclysm_t {
private:
    int32_t TimeElapsed = 0;
    int32_t TimeOfStart, TimeOfEnd = 0;
    void ProcessTimeElapsed() {
        // Calculate state by TimeElapsed
        CState_t NewState = cstMenace;
        if(TimeElapsed > C_PANIC_END_S) {
            NewState = cstIdle;
            TimeOfEnd = TimeS;
        }
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
            App.SignalEvt(EVT_INDICATION);
        }
    }
public:
    CState_t State = cstIdle;
    CStateChange_t StateChange = cscNone;
    void ProcessSignal(uint32_t ATimeElapsed) {
        // Ignore Signal if occured soon after Cataclysm end
        if(!IsOngoing()) {
            int32_t TimeSinceEnd = TimeS - TimeOfEnd;
            Uart.Printf("TimeSinceEnd: %d\r", TimeSinceEnd);
            if(TimeSinceEnd < 120) return;
        }
        TimeElapsed = ATimeElapsed;
        TimeOfStart = TimeS - TimeElapsed;  // Calculate Time Of Start (needed for buttonpress process)
        ProcessTimeElapsed();               // Get state and signal if needed
    }
    void Tick1S() {
        if(State == cstIdle) return; // Nothing to do here
        TimeElapsed++;
        ProcessTimeElapsed();        // Get state and signal if needed
    }
    bool IsOngoing() { return State != cstIdle; }
    CState_t WhenEvtOccured(int32_t TimeOfEvent) {
        int32_t LocalTime = TimeOfEvent - TimeOfStart;
        //Uart.Printf("tStart: %d; tEvt: %d\r", TimeOfStart, TimeOfEvent);
//        Uart.Printf("LocalTime: %d\r", LocalTime);
        if(IS_BETWEEN_INCL_BOTH(LocalTime, -C_GAP_S, C_MENACE_END_S)) return cstMenace;
        else if(IS_BETWEEN_INCL_R(LocalTime, C_MENACE_END_S, C_DANGER_END_S)) return cstDanger;
        else if(IS_BETWEEN_INCL_R(LocalTime, C_DANGER_END_S, C_PANIC_END_S)) return cstPanic;
        else return cstIdle; // Not during C
    }
};
static Cataclysm_t Cataclysm;

// ==== Sheltering ====
enum ShelteringState_t {sstNotSheltered, sstShelteredM, sstShelteredDP};
class Sheltering_t {
public:
    int32_t TimeOfBtnPress = 0;
    ShelteringState_t State = sstNotSheltered;
    void ProcessCChangeOrBtn() {
        CState_t BtnPressPeriod = Cataclysm.WhenEvtOccured(TimeOfBtnPress);
        ShelteringState_t NewState = State;
        switch(State) {
            case sstNotSheltered:
                if(BtnPressPeriod == cstMenace) NewState = sstShelteredM;
                else if(ANY_OF_2(BtnPressPeriod, cstDanger, cstPanic)) NewState = sstShelteredDP;
                break;
            case sstShelteredM:
            case sstShelteredDP:
                if(!Cataclysm.IsOngoing()) NewState = sstNotSheltered;
                break;
        } // switch
        if(State != NewState) {
            State = NewState;
            App.SignalEvt(EVT_INDICATION);
        }
    }
};
static Sheltering_t Sheltering;

PillType_t PillType;

// ==== Health ====
enum HState_t {hstHealthy=0, hstIll=1, hstDead=2};

class Health_t {
public:
    HState_t State = hstHealthy;
    void Load() {
        uint32_t estate = EE.Read32(EE_ADDR_HEALTH_STATE);
        if(estate > 2) {
            State = hstHealthy;
            Uart.Printf("Using default state\r");
        }
        else {
            State = (HState_t)estate;
            Uart.Printf("Loaded State: %u\r", State);
        }
    }
    void Save() {
        EE.Write32(EE_ADDR_HEALTH_STATE, (uint32_t)State);
    }

    void ProcessCChange(CStateChange_t CstCh, ShelteringState_t ShState) {
        Uart.Printf("CstCh: %u; ShState: %u\r", CstCh, ShState);
        HState_t NewState = State;
        switch(State) {
            case hstHealthy:
                // Become ill if met Danger or Panic state not being in shelter
                if(ANY_OF_3(CstCh, cscI2D, cscI2P, cscM2D) and ShState != sstShelteredM) NewState = hstIll;
                break;
            case hstIll:
                // Die if met with Cataclysm End not being in shelter, or if met Cataclysm Start being ill
                if((CstCh == cscEnd and ShState == sstNotSheltered) or ANY_OF_3(CstCh, cscI2M, cscI2D, cscI2P)) NewState = hstDead;
                break;
            case hstDead: break; // In this case, Cataclysm changes nothing
        }
        if(State != NewState) {
            State = NewState;
            Save();
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
            Save();
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

    Led.Init();
//    Led.SetupSeqEndEvt(chThdGetSelfX(), EVT_LED_SEQ_END);
    Vibro.Init();
#if BEEPER_ENABLED // === Beeper ===
    Beeper.Init();
    Beeper.StartOrRestart(bsqBeepBeep);
    chThdSleepMilliseconds(702);    // Let it complete the show
#endif
#if BTN_ENABLED
    PinSensors.Init();
#endif
//    Adc.Init();

#if PILL_ENABLED // === Pill ===
    i2c1.Init();
    PillMgr.Init();
#endif

    // ==== Time and timers ====
    TmrEverySecond.InitAndStart();
    TimeS = C_PANIC_END_S + 9;    // if cataclysm would start immideately
//    TmrRxTableCheck.InitAndStart();

    // ==== Radio ====
    if(Radio.Init() == OK) Led.StartOrRestart(lsqStart);
    else Led.StartOrRestart(lsqFailure);
    chThdSleepMilliseconds(1008);

    // ==== App-specific ====
    Health.Load();

    App.SignalEvt(EVT_EVERY_SECOND); // check it now
    App.SignalEvt(EVT_INDICATION);

    // Main cycle
    App.ITask();
}

__noreturn
void App_t::ITask() {
    while(true) {
        __unused eventmask_t Evt = chEvtWaitAny(ALL_EVENTS);
        if(Evt & EVT_EVERY_SECOND) {
            TimeS++;
            Cataclysm.Tick1S();
//            ReadAndSetupMode();
        }

#if BTN_ENABLED
        if(Evt & EVT_BUTTONS) {
            Uart.Printf("Btn\r");
            BtnEvtInfo_t EInfo;
            while(BtnGetEvt(&EInfo) == OK) {
                if(EInfo.Type == bePress) {
                    Sheltering.TimeOfBtnPress = TimeS;
                    Sheltering.ProcessCChangeOrBtn();
                } // if Press
            } // while
        }
#endif

        if(Evt & EVT_RX) {
            int32_t TimeRx = Radio.PktRx.Time;
            Uart.Printf("RX %u\r", TimeRx);
            Cataclysm.ProcessSignal(TimeRx);
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

#if 1 // ==== App specific ====
        if(Evt & EVT_CSTATE_CHANGE) {
//            Uart.Printf("C chng: %u\r", Cataclysm.StateChange);
            if(Cataclysm.StateChange != cscEnd) Sheltering.ProcessCChangeOrBtn();
            Health.ProcessCChange(Cataclysm.StateChange, Sheltering.State);
            if(Cataclysm.StateChange == cscEnd) Sheltering.ProcessCChangeOrBtn();
        }

        if(Evt & EVT_INDICATION) {
            // Always
            if(Health.State == hstDead) {
                Led.StartOrContinue(lsqDead);
                Vibro.StartOrContinue(vsqDeath);
            }
            else {
                // When Cataclysm is everywhere
                if(Cataclysm.IsOngoing()) {
                    bool Sheltered = ANY_OF_2(Sheltering.State, sstShelteredM, sstShelteredDP);
                    // LED
                    switch(Cataclysm.State) {
                        case cstMenace:
                            if(Sheltered) Led.StartOrContinue(lsqMenaceSh);
                            else          Led.StartOrContinue(lsqMenaceNoSh);
                            break;
                        case cstDanger:
                            if(Sheltered) Led.StartOrContinue(lsqDangerSh);
                            else          Led.StartOrContinue(lsqDangerNoSh);
                            break;
                        case cstPanic:
                            if(Sheltered) Led.StartOrContinue(lsqPanicSh);
                            else          Led.StartOrContinue(lsqPanicNoSh);
                            break;
                        default: break;
                    }
                    // Vibro
                    if(Sheltered) Vibro.Stop();
                    else Vibro.StartOrRestart(vsqCataclysm);
                }
                // When all is calm
                else {
                    if(Health.State == hstHealthy) {
                        Led.StartOrContinue(lsqHealthy);
                        Vibro.Stop();
                    }
                    else { // Ill
                        Led.StartOrContinue(lsqIll);
                        Vibro.StartOrContinue(vsqIll);
                    }
                } // all calm
            } // not dead
        }
#endif

#if PILL_ENABLED // ==== Pill ====
        if(Evt & EVT_PILL_CONNECTED) {
            Uart.Printf("Pill: %d\r", PillMgr.Pill.TypeInt32);
            if(PillMgr.Pill.Type == ptCure) {
                Led.StartOrRestart(lsqPillCure);
                chThdSleepMilliseconds(999);
                Health.ProcessPill(PillMgr.Pill.Type);
            }
            else if(PillMgr.Pill.Type == ptPanacea) {
                Led.StartOrRestart(lsqPillPanacea);
                chThdSleepMilliseconds(999);
                Health.ProcessPill(PillMgr.Pill.Type);
            }
            else {
                Led.StartOrRestart(lsqPillBad);
                chThdSleepMilliseconds(999);
            }
            App.SignalEvt(EVT_INDICATION);  // Restart indication after pill show
        }

        if(Evt & EVT_PILL_DISCONNECTED) {
            Uart.Printf("Pill Discon\r");
        }
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
//    Radio.MustTx = false; // Just in case
    // Reset everything
    Vibro.Stop();
    Led.Stop();
    // Analyze switch
    OldDipSettings = b;
    Uart.Printf("Dip: %02X\r", b);
//    Vibro.StartOrRestart(vsqBrr);
//    if(b == 0) {
//        App.Mode = modePlayer;
//        Led.StartOrRestart(lsqModePlayerStart);
//        chThdSleepMilliseconds(720);    // Let blink end
        // Get Health from EE
//        Health.Load();

//        Led.StartOrRestart(lsqModeTx);  // Start Idle indication
//}
//    else {
//        App.Mode = modeTx;
//        Led.StartOrRestart(lsqModeTxStart);
//        chThdSleepMilliseconds(720);    // Let blink end
//        Led.StartOrRestart(lsqModeTx);  // Start Idle indication
//    }
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

    else if(PCmd->NameIs("RX")) {
        if(PCmd->GetNextInt32(&dw32) != OK) { PShell->Ack(CMD_ERROR); return; }
        Radio.PktRx.Time = dw32;
        App.SignalEvt(EVT_RX);
    }
//    else if(PCmd->NameIs("cst")) {
//        if(PCmd->GetNextInt32(&dw32) != OK) { PShell->Ack(CMD_ERROR); return; }
//        Cataclysm.StateChange = (CStateChange_t)dw32;   // State transition
//        if(PCmd->GetNextInt32(&dw32) != OK) { PShell->Ack(CMD_ERROR); return; }
//        IsInShelter = dw32;             // Is in shelter
//        App.SignalEvt(EVT_CSTATE_CHANGE);
//    }

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
