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
#include "pill_mgr.h"
#include "kl_adc.h"

#if 1 // ======================== Variables and defines ========================
App_t App;

// EEAddresses
#define EE_ADDR_DEVICE_ID       0
#define EE_ADDR_COLOR           4

static const PinInputSetup_t DipSwPin[DIP_SW_CNT] = { DIP_SW6, DIP_SW5, DIP_SW4, DIP_SW3, DIP_SW2, DIP_SW1 };
static uint8_t GetDipSwitch();
static void ReadAndSetupMode();
static uint8_t ISetID(int32_t NewID);
Eeprom_t EE;
static void ReadIDfromEE();

// ==== Periphery ====
Vibro_t Vibro {VIBRO_PIN};
#if BEEPER_ENABLED
Beeper_t Beeper {BEEPER_PIN};
#endif

#if BTN_ENABLED
#define BTN_POLL_PERIOD_MS   27
static void TmrBtnCallback(void *p);

class Btn_t {
private:
    virtual_timer_t Tmr;
    bool IWasPressed = false;
    PinInput_t Pin {BTN_PIN};
public:
    void Init() {
        chVTSet(&Tmr, MS2ST(BTN_POLL_PERIOD_MS), TmrBtnCallback, nullptr);
        Pin.Init();
    }
    void OnTmrTick() {}
    void ITmrCallback() {
        chSysLockFromISR();
        if(Pin.IsHi() and !IWasPressed) {
            IWasPressed = true;
            App.SignalEvtI(EVT_BUTTON);
        }
        else if(!Pin.IsHi() and IWasPressed) IWasPressed = false;
        chVTSetI(&Tmr, MS2ST(BTN_POLL_PERIOD_MS), TmrBtnCallback, nullptr);
        chSysUnlockFromISR();
    }
} Btn;

void TmrBtnCallback(void *p) { Btn.ITmrCallback(); }
#endif

LedRGBwPower_t Led { LED_R_PIN, LED_G_PIN, LED_B_PIN, LED_EN_PIN };

// ==== Timers ====
static TmrKL_t TmrEverySecond {MS2ST(1000), EVT_EVERY_SECOND, tktPeriodic};
static TmrKL_t TmrIndication  {MS2ST(3600), EVT_INDICATION_OFF, tktOneShot};
#endif

#if 1 // ============================ Binding ==================================
LedRGBChunk_t lsqBinding[] = {
        {csSetup, 180, clWhite},
        {csSetup, 180, clBlack},
        {csWait, 999},
        {csGoto, 0},
};

enum BindingState_t {bndOff, bndNear, bndFar, bndLost, bndDeathOccured, bndColorSwitch, bndRxMoodIgnore};

// Timings
#define BND_BLINK_PERIOD_MS         2700
#define BND_FAR_BLINK_PERIOD_MS     720

// Indication of "far" state
struct FarIndication_t {
    uint32_t Interval;
    const BaseChunk_t *Vsq;
};
static const FarIndication_t FarIndication[] = {
        {5, vsqBrr},
        {5, vsqBrr},
        {5, vsqBrr},
//        {5, vsqBrr},
//        {5, vsqBrr},
//        {5, vsqBrr},
        // 30 s passed
        {5, vsqBrrBrr},
        {5, vsqBrrBrr},
//        {5, vsqBrrBrr},
//        {5, vsqBrrBrr},
        // 50 s passed
        {4, vsqBrrBrrBrr},
        {3, vsqBrrBrrBrr},
        {2, vsqBrrBrrBrr},
        {2, vsqBrrBrrBrr},
        // 60 s passed
        {0, nullptr} // End of sequence, death is here
};

// Binding events
enum BindingEvtType_t {bevtStart, bevtStop, bevtRadioPkt, bevtTmrIndTick, bevtTmrLost, bevtBtn, bevtTmrMood};
struct BindingEvt_t {
    BindingEvtType_t Type;
    Color_t Color;
    int32_t Rssi;
};

// Moods
static const Color_t MoodClr[] = {clBlue, clGreen, clYellow, clWhite};
#define MOOD_CNT    countof(MoodClr)

class Binding_t {
private:
    BindingState_t State = bndOff;
    const FarIndication_t *Pind;
    TmrKL_t TmrInd  {MS2ST(3600), EVT_BND_TMR_IND,  tktOneShot};
    TmrKL_t TmrLost {MS2ST(6300), EVT_BND_TMR_LOST, tktOneShot};
    // Mood management
    uint8_t MoodIndx = 0;
    TmrKL_t TmrMoodSwitch {MS2ST(2700), EVT_BND_TMR_MOOD, tktOneShot};
public:
    void ProcessEvt(BindingEvtType_t Evt);
    void Init() {
        TmrInd.Init();
        TmrLost.Init();
        TmrMoodSwitch.Init();
    }
} Binding;
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

    // === Vibro ===
    Vibro.Init();
    Vibro.SetupSeqEndEvt(chThdGetSelfX(), EVT_VIBRO_END);
    Vibro.StartSequence(vsqBrr);

#if BEEPER_ENABLED // === Beeper ===
    Beeper.Init();
    Beeper.StartSequence(bsqBeepBeep);
    chThdSleepMilliseconds(702);    // Let it complete the show
#endif

    Btn.Init();
//    Adc.Init();

#if PILL_ENABLED // === Pill ===
    i2c1.Init();
    PillMgr.Init();
    TmrCheckPill.InitAndStart();
#endif

    TmrEverySecond.InitAndStart();
    TmrIndication.Init();
    App.SignalEvt(EVT_EVERY_SECOND); // check it now

    Binding.Init();

    if(Radio.Init() == OK) Led.StartSequence(lsqStart);
    else Led.StartSequence(lsqFailure);
    chThdSleepMilliseconds(1008);

    // Main cycle
    App.ITask();
}

__noreturn
void App_t::ITask() {
    while(true) {
        __unused eventmask_t Evt = chEvtWaitAny(ALL_EVENTS);
        if(Evt & EVT_EVERY_SECOND) {
            ReadAndSetupMode();
        }

#if PILL_ENABLED // ==== Pill ====
        if(Evt & EVT_PILL_CHECK) {
            PillMgr.Check();
            switch(PillMgr.State) {
                case pillJustConnected:
                    Uart.Printf("Pill: %d %d\r", PillMgr.Pill.TypeInt32, PillMgr.Pill.ChargeCnt);
                    break;
//                case pillJustDisconnected: Uart.Printf("Pill Discon\r"); break;
                default: break;
            }
        }
#endif

#if 0 // ==== Led sequence end ====
        if(Evt & EVT_LED_SEQ_END) {
        }
#endif

        if(Evt & EVT_RADIO) {
            // Something received
            if(Mode == modeDetectorRx) {
                if(Led.IsIdle()) Led.StartSequence(lsqCyan);
                TmrIndication.Restart(); // Reset off timer
            }
            else if(Mode == modeBinding) Binding.ProcessEvt(bevtRadioPkt);
        }

        if(Evt & EVT_INDICATION_OFF) if(Mode != modeBinding) Led.StartSequence(lsqOff);

        if(Evt & EVT_BND_TMR_IND)  {
            if(Mode == modeBinding) Binding.ProcessEvt(bevtTmrIndTick);
        }
        if(Evt & EVT_BND_TMR_LOST) {
            if(Mode == modeBinding) Binding.ProcessEvt(bevtTmrLost);
        }
        if(Evt & EVT_BND_TMR_MOOD) {
            if(Mode == modeBinding) Binding.ProcessEvt(bevtTmrMood);
        }

#if BTN_ENABLED
        if(Evt & EVT_BUTTON) {
            if(Mode == modeBinding) Binding.ProcessEvt(bevtBtn);
        }
#endif

//        if(Evt & EVT_OFF) {
////            Uart.Printf("Off\r");
//            chSysLock();
//            Sleep::EnableWakeup1Pin();
//            Sleep::EnterStandby();
//            chSysUnlock();
//        }

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
}

#if 1 // ============================== Binding ================================
void Binding_t::ProcessEvt(BindingEvtType_t Evt) {
    Uart.Printf("* %u\r", Evt);
    switch(Evt) {
        case bevtStop:
            State = bndOff;
            Uart.Printf("bndOff\r");
            Radio.MustSleep = true;
            TmrInd.Stop();
            TmrLost.Stop();
            Led.Stop();
            Vibro.Stop();
            break;

        case bevtStart:
            State = bndNear;
            Uart.Printf("bndNear1\r");
            MoodIndx = 0;
            lsqBinding[0].Color = clBlue;
            lsqBinding[2].Time_ms = BND_BLINK_PERIOD_MS;
            Led.StartSequence(lsqBinding);
            TmrLost.Start();
            // Transmit same color
            lsqBinding[0].Color.ToRGB(&Radio.PktTx.R, &Radio.PktTx.G, &Radio.PktTx.B);
            Radio.MustSleep = false;
            break;

        case bevtRadioPkt:
            if(State == bndOff or State == bndDeathOccured) return;
            TmrLost.Restart();
            if(Radio.Rssi >= RSSI_BIND_THRS) {  // He is near
                Vibro.Stop(); // in case of returning from far
                TmrInd.Stop();
                // Setup received color if new
                if(State != bndColorSwitch and State != bndRxMoodIgnore) {
                    Color_t FClr(Radio.PktRx.R, Radio.PktRx.G, Radio.PktRx.B);
                    if(FClr != lsqBinding[0].Color or State != bndNear) {
                        Led.Stop();
                        lsqBinding[2].Time_ms = BND_BLINK_PERIOD_MS;
                        lsqBinding[0].Color = FClr;
                        Led.StartSequence(lsqBinding);
                        Vibro.StartSequence(vsqBrr);
                        // Transmit same color
                        FClr.ToRGB(&Radio.PktTx.R, &Radio.PktTx.G, &Radio.PktTx.B);
                    }
                    State = bndNear;
                    Uart.Printf("bndNear2\r");
                } // if not ignore
            }
            // He is far
            else {
                if(State != bndFar) {   // Ignore weak pkts if already in Far state
                    State = bndFar;
                    Uart.Printf("bndFar\r");
                    // Start red blink
                    Led.Stop();
                    lsqBinding[2].Time_ms = BND_FAR_BLINK_PERIOD_MS;
                    lsqBinding[0].Color = clRed;
                    Led.StartSequence(lsqBinding);
                    // Init variables and start timer for vibro
                    Pind = FarIndication;
//                    Vibro.StartSequence(Pind->Vsq);
                    TmrInd.Start(S2ST(Pind->Interval));
                }
            }
            break;

        case bevtTmrIndTick:
            if(State == bndFar) {
                Pind++; // switch to next indication chunk
                if(Pind->Interval == 0) { // this one was last
                    State = bndDeathOccured;
                    Uart.Printf("bndDeathOccured\r");
                    Led.StartSequence(lsqDeath);
                    Vibro.StartSequence(vsqDeath);
                }
                else { // Not last
                    Vibro.StartSequence(Pind->Vsq);
                    TmrInd.Start(S2ST(Pind->Interval));
                }
            }
            break;

        case bevtTmrLost:
            if(State == bndNear or State == bndFar) {
                State = bndLost;
                Uart.Printf("bndLost\r");
                Led.StartSequence(lsqLost);
                Vibro.StartSequence(vsqLost);
            }
            break;

        case bevtBtn:
            if(State == bndNear or State == bndColorSwitch) {
                State = bndColorSwitch;
                TmrMoodSwitch.Restart();
                // Show next mood
                MoodIndx++;
                if(MoodIndx >= MOOD_CNT) MoodIndx = 0;
                Led.Stop();
                Led.SetColor(MoodClr[MoodIndx]);
            }
            else if(State == bndDeathOccured) {
                Vibro.Stop();   // switch off vibro by btn
            }
            break;

        case bevtTmrMood:   // Transmit choosen mood after iteration done
            // After btn finally released
            if(State == bndColorSwitch) {
                Led.Stop();
                lsqBinding[2].Time_ms = BND_BLINK_PERIOD_MS;
                lsqBinding[0].Color = MoodClr[MoodIndx];
                Led.StartSequence(lsqBinding);
                Vibro.StartSequence(vsqBrr);
                // Send choosen mood
                MoodClr[MoodIndx].ToRGB(&Radio.PktTx.R, &Radio.PktTx.G, &Radio.PktTx.B);
                // Ignore received color for a while
                State = bndRxMoodIgnore;
                TmrMoodSwitch.Restart();
            }
            // Time to react back to neighbor's mood
            else if(State == bndRxMoodIgnore) {
                State = bndNear;
            }
            break;
    } // switch
}
#endif

void ReadAndSetupMode() {
    static uint8_t OldDipSettings = 0xFF;
    uint8_t b = GetDipSwitch();
    if(b == OldDipSettings) return;
    // Something has changed
    Radio.MustSleep = true; // Sleep until we decide what to do
    // Reset everything
    Vibro.Stop();
    Led.Stop();
    Binding.ProcessEvt(bevtStop);
    // Analyze switch
    OldDipSettings = b;
    Uart.Printf("Dip: %02X\r", b);
    // Get mode
    if(App.ID == 0) {
        App.Mode = modeNone;
        Led.StartSequence(lsqFailure);
    }
    else if(App.ID == 1 or App.ID == 3 or App.ID == 5 or App.ID == 7) {
        App.Mode = modeDetectorTx;
        Led.StartSequence(lsqDetectorTxMode);
        Radio.MustSleep = false;
    }
    else if(App.ID == 2 or App.ID == 4 or App.ID == 6 or App.ID == 8) {
        App.Mode = modeDetectorRx;
        Led.StartSequence(lsqDetectorRxMode);
        Radio.MustSleep = false;
    }
    else {
        App.Mode = modeBinding;
        Led.StartSequence(lsqBindingMode);
        chThdSleepMilliseconds(999);
        Binding.ProcessEvt(bevtStart);
    }
    // ==== Setup TX power ====
    uint8_t pwrIndx = (b & 0b000111);
    chSysLock();
    CC.SetTxPower(CCPwrTable[pwrIndx]);
    chSysUnlock();
}


#if UART_RX_ENABLED // ================= Command processing ====================
void App_t::OnCmd(Shell_t *PShell) {
	Cmd_t *PCmd = &PShell->Cmd;
    __attribute__((unused)) int32_t dw32 = 0;  // May be unused in some configurations
    Uart.Printf("%S\r", PCmd->Name);
    // Handle command
    if(PCmd->NameIs("Ping")) {
        PShell->Ack(OK);
    }

    else if(PCmd->NameIs("GetID")) PShell->Reply("ID", App.ID);

    else if(PCmd->NameIs("SetID")) {
        if(PCmd->GetNextNumber(&dw32) != OK) { PShell->Ack(CMD_ERROR); return; }
        uint8_t r = ISetID(dw32);
        PShell->Ack(r);
    }

    else if(PCmd->NameIs("RPkt")) {
        if(PCmd->GetNextNumber(&dw32) != OK) { PShell->Ack(CMD_ERROR); return; }
        Radio.Rssi = dw32;
        if(PCmd->GetNextNumber(&dw32) != OK) { PShell->Ack(CMD_ERROR); return; }
        Radio.PktRx.R = dw32;
        if(PCmd->GetNextNumber(&dw32) != OK) { PShell->Ack(CMD_ERROR); return; }
        Radio.PktRx.G = dw32;
        if(PCmd->GetNextNumber(&dw32) != OK) { PShell->Ack(CMD_ERROR); return; }
        Radio.PktRx.B = dw32;
        SignalEvt(EVT_RADIO);
//        PShell->Ack(OK);
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
