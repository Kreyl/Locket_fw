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

App_t App;
static const DipSwitch_t DipSwitch {DIP_SW1, DIP_SW2, DIP_SW3, DIP_SW4, DIP_SW5, DIP_SW6};

static TmrKL_t TmrCheckPill {MS2ST(450), EVT_PILL_CHECK, tktPeriodic};
static TmrKL_t TmrCheckBtn  {MS2ST(54),  EVT_BUTTONS, tktPeriodic};
static TmrKL_t TmrEverySecond {MS2ST(100), EVT_EVERY_SECOND, tktPeriodic};

static const PinInput_t Btn (BTN_PIN);

//Vibro_t Vibro {VIBRO_PIN};
Beeper_t Beeper {BEEPER_PIN};
LedRGBwPower_t Led { LED_R_PIN, LED_G_PIN, LED_B_PIN, LED_EN_PIN };

int main(void) {
    // ==== Init Vcore & clock system ====
    SetupVCore(vcore1V5);
//    Clk.SetMSI4MHz();
    Clk.UpdateFreqValues();

    // Init OS
    halInit();
    chSysInit();
    App.InitThread();

    // ==== Init hardware ====
    Uart.Init(115200, UART_GPIO, UART_TX_PIN, UART_GPIO, UART_RX_PIN);
    Uart.Printf("\r%S %S\r", APP_NAME, BUILD_TIME);
//    Uart.Printf("ID: %X %X %X\r", GetUniqID1(), GetUniqID2(), GetUniqID3());
//    if(Sleep::WasInStandby()) {
//        Uart.Printf("WasStandby\r");
//        Sleep::ClearStandbyFlag();
//    }
    Clk.PrintFreqs();
    RandomSeed(GetUniqID3());   // Init random algorythm with uniq ID

    i2c1.Init();
    PillMgr.Init();

    Led.Init();
    Led.SetupSeqEndEvt(chThdGetSelfX(), EVT_LED_SEQ_END);
    Led.StartSequence(lsqStart);
    //Led.SetColor(clWhite);
//    Vibro.Init();
//    Vibro.StartSequence(vsqBrr);

    Beeper.Init();
    Beeper.StartSequence(bsqBeepBeep);
    chThdSleepMilliseconds(702);    // Let it complete the show

    Btn.Init();
    DipSwitch.Init();

    // Timers
    TmrCheckBtn.InitAndStart();
    TmrCheckPill.InitAndStart();
    TmrEverySecond.InitAndStart();
    App.SignalEvt(EVT_EVERY_SECOND); // check it now

//    if(Radio.Init() != OK) {
//        Led.StartSequence(lsqFailure);
//        chThdSleepMilliseconds(2700);
//    }
//    else
//    Led.StartSequence(lsqOn);

    // Main cycle
    App.ITask();
}

__noreturn
void App_t::ITask() {
    while(true) {
        __unused eventmask_t Evt = chEvtWaitAny(ALL_EVENTS);
        if(Evt & EVT_EVERY_SECOND) {
            // Switch role if needed
            uint8_t Dip = DipSwitch.GetValue();
//            Uart.Printf("Dip: %X\r", Dip);
            if(Dip == 1) {
                if(DevType == devtPlayer) {
                    Uart.Printf("Master\r");
                    DevType = devtMaster;
                    Led.Stop();
                    Beeper.Stop();
                    Led.SetColor(GetPillColor(MasterMode));
                }
            }
            else if(DevType == devtMaster) {
                Uart.Printf("Player\r");
                DevType = devtPlayer;
                SetCondition(cndGreen);
            }

            if(DevType == devtPlayer) CndTmr.OnNewSecond();
        }

#if 1 // ==== Button ====
        if(Evt & EVT_BUTTONS) {
            if(DevType == devtMaster) {
                if(Btn.IsHi() and !BtnWasHi) {    // Pressed
                    BtnWasHi = true;
                    Beeper.StartSequence(bsqButton);
                    // Switch mode
                    if(MasterMode == ptTest) MasterMode = ptVitamin;
                    else MasterMode = (PillType_t)((uint32_t)MasterMode + 1);
                    Led.SetColor(GetPillColor(MasterMode));
                }
                else if(!Btn.IsHi() and BtnWasHi) {
                    BtnWasHi = false;
                }
            }
        } // EVTMSK_BTN_PRESS
#endif

#if 1 // ==== Pill ====
        if(Evt & EVT_PILL_CHECK) {
            PillMgr.Check();
            switch(PillMgr.State) {
                case pillJustConnected:
                    Uart.Printf("Pill: %d %d\r", PillMgr.Pill.TypeInt32, PillMgr.Pill.ChargeCnt);
                    if(DevType == devtMaster) OnPillConnMaster();
                    else OnPillConnPlayer();
                    break;
//                case pillJustDisconnected: Uart.Printf("Pill Discon\r"); break;
                default: break;
            }
        }
#endif

#if 1 // ==== Led sequence end ====
        if(Evt & EVT_LED_SEQ_END) {
            if(DevType == devtMaster) {
                Led.SetColor(GetPillColor(MasterMode));
            }
            else if(NeedToRestoreIndication) { // Indicate condition back
                NeedToRestoreIndication = false;
                SetupConditionIndication();
            }
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
}

void App_t::OnPillConnMaster() {
    if(MasterMode == ptTest) {
        if(PillMgr.Pill.IsOk()) {
            if(PillMgr.Pill.Type == ptMaster) ShowPillOk();
            else { // not master
                if(PillMgr.Pill.ChargeCnt > 0) {
                    PillMgr.Pill.ChargeCnt--; // Decrease charges
                    if(PillMgr.WritePill() == OK) { // Write pill
                        ShowPillOk();
                        Uart.Printf("PillAfter: %d %d\r", PillMgr.Pill.TypeInt32, PillMgr.Pill.ChargeCnt);
                    } // if write pill ok
                    else ShowPillBad(); // Write failure
                } // if charge cnt != 0
                else ShowPillBad(); // ChargeCnt == 0
            } // not master
        } // if is ok
        else ShowPillBad(); // Pill not ok
    } // if test
    else {
        // Prepare to write pill
        PillMgr.Pill.Type = MasterMode;
        if(MasterMode == ptVitamin or MasterMode == ptCure) PillMgr.Pill.ChargeCnt = 1;
        else PillMgr.Pill.ChargeCnt = 5;    // Panacea or energetic (or master, which is don't care)
        // Write pill
        if(PillMgr.WritePill() == OK) {
            ShowPillOk();
            Uart.Printf("PillAfter: %d %d\r", PillMgr.Pill.TypeInt32, PillMgr.Pill.ChargeCnt);
        }
        else ShowPillBad(); // Write failure
    } // if not test
}

#if 1 // ============================ Player ===================================
void App_t::OnNextCondition() {
    Uart.Printf("%S\r", __FUNCTION__);
    switch(Condition) {
        case cndBlue:   SetCondition(cndGreen);  break;
        case cndGreen:  SetCondition(cndYellow); break;
        case cndYellow: SetCondition(cndRed);    break;
        case cndRed:    SetCondition(cndVBRed);  break;
        case cndVBRed:  SetCondition(cndViolet); break;
        default: break;
    }
}

void App_t::OnPillConnPlayer() {
    if(!PillMgr.Pill.IsOk()) { ShowPillBad(); return; }  // Errorneous pill
    // Check and remove charges
    if(PillMgr.Pill.Type != ptMaster) {
        if(PillMgr.Pill.ChargeCnt == 0) { ShowPillBad(); return; }  // No charge
        PillMgr.Pill.ChargeCnt--; // Decrease charges
        // Write pill
        if(PillMgr.WritePill() != OK) { ShowPillBad(); return; } // Write failure
        Uart.Printf("PillAfter: %d %d\r", PillMgr.Pill.TypeInt32, PillMgr.Pill.ChargeCnt);
    }
    ShowPillOk();
    // Wait for indication to complete
    chThdSleepMilliseconds(1008);
    // Process pill
    switch(PillMgr.Pill.Type) {
        case ptVitamin:
            switch(Condition) {
                case cndBlue:   SetCondition(cndBlue);   break;
                case cndGreen:  SetCondition(cndBlue);   break;
                case cndYellow: SetCondition(cndGreen);  break;
                case cndRed:    SetCondition(cndYellow); break;
                case cndVBRed:  SetCondition(cndRed);    break;
                default: NeedToRestoreIndication = true; break;
            } // switch(Condition)
            break;

        case ptCure:
            switch(Condition) {
                case cndBlue:   SetCondition(cndGreen);  break;
                case cndGreen:  SetCondition(cndYellow); break;
                case cndYellow: SetCondition(cndRed);    break;
                case cndRed:    SetCondition(cndVBRed);  break;
                case cndVBRed:  SetCondition(cndWhite);  break;
                default: NeedToRestoreIndication = true; break;
            } // switch(Condition)
            break;

        case ptPanacea: SetCondition(cndWhite); break;

        case ptEnergetic:
            if(Condition != cndViolet and Condition != cndWhite) SetCondition(cndBlue);
            else NeedToRestoreIndication = true;
            break;

        case ptMaster:
            switch(Condition) {
                case cndBlue:   SetCondition(cndGreen);  break;
                case cndGreen:  SetCondition(cndYellow); break;
                case cndYellow: SetCondition(cndRed);    break;
                case cndRed:    SetCondition(cndVBRed);  break;
                case cndVBRed:  SetCondition(cndWhite);  break;
                case cndWhite:  SetCondition(cndViolet); break;
                case cndViolet: SetCondition(cndBlue);   break;
            } // switch(Condition)
            break;
        default: break;
    } // switch pill type
}

void App_t::DoVibroBlink() {
    Uart.Printf("%S\r", __FUNCTION__);
    switch(Condition) {
        case cndGreen:
            Led.StartSequence(lsqVibroBlinkGreen);
            break;
        case cndYellow:
            Led.StartSequence(lsqVibroBlinkYellow);
            break;
        case cndRed:
            Led.StartSequence(lsqVibroBlinkRed);
            break;
        default: break;
    }
}

void App_t::StopVibroBlink() {
    Uart.Printf("%S\r", __FUNCTION__);
//    Led.Stop();
    switch(Condition) {
        case cndGreen:  Led.StartSequence(lsqCondGreen); break;
        case cndYellow: Led.StartSequence(lsqCondYellow); break;
        case cndRed:    Led.StartSequence(lsqCondRed); break;
        default: break;
    }
}

void App_t::SetCondition(Condition_t NewCondition) {
    Condition = NewCondition;
    int Duration, VBTime1, VBTime2;
    SetupConditionIndication();
    Beeper.StartSequence(bsqBeepBeep);
    switch(Condition) {
        case cndBlue:
            Uart.Printf("Blue\r");
            CndTmr.Init(DURATION_BLUE_S, -1, -1);
            break;

        case cndGreen:
            RandomSeed(chVTGetSystemTime());    // Reinit random generator
            Duration = Random(DURATION_GYR_MIN_S, DURATION_GYR_MAX_S);
            VBTime1 = Random(60, Duration-60);
            Uart.Printf("Green: d=%u; vb=%u\r", Duration, VBTime1);
            CndTmr.Init(Duration, VBTime1, -1);
            break;

        case cndYellow:
            Duration = Random(DURATION_GYR_MIN_S, DURATION_GYR_MAX_S);
            VBTime1 = Random(60, Duration-(2*60));
            VBTime2 = Random(VBTime1+60, Duration-60);
            Uart.Printf("Y: d=%u; vb1=%u; vb2=%u\r", Duration, VBTime1, VBTime2);
            // Start timers
            CndTmr.Init(Duration, VBTime1, VBTime2);
            break;

        case cndRed:
            Duration = Random(DURATION_GYR_MIN_S, DURATION_GYR_MAX_S);
            VBTime1 = Random(60, Duration-(2*60));
            VBTime2 = Random(VBTime1+60, Duration-60);
            Uart.Printf("R: d=%u; vb1=%u; vb2=%u\r", Duration, VBTime1, VBTime2);
            // Start timers
            CndTmr.Init(Duration, VBTime1, VBTime2);
            break;

        case cndVBRed:
            Uart.Printf("VBRed\r");
            CndTmr.Init(DURATION_VBRED_S, -1, -1);
            break;

        case cndViolet:
            Uart.Printf("Violet\r");
            CndTmr.Init(-1, -1, -1);
            break;

        case cndWhite:
            Uart.Printf("White\r");
            CndTmr.Init(-1, -1, -1);
            break;
    } // switch
}
#endif

void App_t::SetupConditionIndication() {
    switch(Condition) {
        case cndBlue:   Led.StartSequence(lsqCondBlue);   break;
        case cndGreen:  Led.StartSequence(lsqCondGreen);  break;
        case cndYellow: Led.StartSequence(lsqCondYellow); break;
        case cndRed:    Led.StartSequence(lsqCondRed);    break;
        case cndVBRed:  Led.StartSequence(lsqCondVBRed);  break;
        case cndViolet: Led.StartSequence(lsqCondViolet); break;
        case cndWhite:  Led.StartSequence(lsqCondWhite); break;
    }
}

void App_t::ShowPillOk() {
    switch(PillMgr.Pill.Type) {
        case ptVitamin:   Led.StartSequence(lsqPillVitamin); break;
        case ptCure:      Led.StartSequence(lsqPillCure); break;
        case ptPanacea:   Led.StartSequence(lsqPillPanacea); break;
        case ptEnergetic: Led.StartSequence(lsqPillEnergetic); break;
        case ptMaster:    Led.StartSequence(lsqPillMaster); break;
        default:
            ShowPillBad();  // Unknown pill
            return;
            break;
    } // switch
    Beeper.StartSequence(bsqBeepPillOk);
}
void App_t::ShowPillBad() {
    Beeper.StartSequence(bsqBeepPillBad);
    Led.StartSequence(lsqPillBad);
    NeedToRestoreIndication = true;
}

// Low resolution timer: 16-bit hardware timer allows only 6.5s delays
void CondTimer_t::OnNewSecond() {
    // Duration
    if(Duration > 0) {
        Duration--;
        if(Duration == 0) App.OnNextCondition();
    }
    // VibroBlink
    if(VBTime1 > 0) {
        VBTime1--;
        if(VBTime1 == 0) {
            VBTime1 = -1;   // Disable future changes
            // Start vibration and blinking
            VBDuration = DURATION_BLINK_VIBRO_S;
            App.DoVibroBlink();
        }
    }
    if(VBTime2 > 0) {
        VBTime2--;
        if(VBTime2 == 0) {
            VBTime2 = -1;   // Disable future changes
            // Start vibration and blinking
            VBDuration = DURATION_BLINK_VIBRO_S;
            App.DoVibroBlink();
        }
    }
    // VibroBlink
    if(VBDuration > 0) {
        VBDuration--;
        if(VBDuration == 0) {
            VBDuration = -1;
            // Stop vibration and blinking
            App.StopVibroBlink();
        }
    }
//    if(Duration % 10 == 0) Uart.Printf("D: %d; VBT1: %d; VBT2: %d; VBD: %d\r", Duration, VBTime1, VBTime2, VBDuration);
}

#if UART_RX_ENABLED // ======================= Command processing ============================
void App_t::OnCmd(Shell_t *PShell) {
	Cmd_t *PCmd = &PShell->Cmd;
    __attribute__((unused)) int32_t dw32 = 0;  // May be unused in some configurations
    Uart.Printf("%S\r", PCmd->Name);
    // Handle command
    if(PCmd->NameIs("Ping")) {
        PShell->Ack(OK);
    }

    else if(PCmd->NameIs("B")) {
        SetCondition(cndBlue);
    }
    else if(PCmd->NameIs("G")) {
        SetCondition(cndGreen);
    }
    else if(PCmd->NameIs("Y")) {
        SetCondition(cndYellow);
    }
    else if(PCmd->NameIs("R")) {
        SetCondition(cndRed);
    }
    else if(PCmd->NameIs("VBR")) {
        SetCondition(cndVBRed);
    }
    else if(PCmd->NameIs("V")) {
        SetCondition(cndViolet);
    }

    else PShell->Ack(CMD_UNKNOWN);
}
#endif

