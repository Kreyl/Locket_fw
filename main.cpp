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
static TmrKL_t TmrCheckPill {MS2ST(504), EVT_PILL_CHECK, tktPeriodic};
static TmrKL_t TmrCheckBtn  {MS2ST(54),  EVT_BUTTONS, tktPeriodic};
//static TmrKL_t TmrOff      {MS2ST(254), EVT_OFF, tktOneShot};
static PinInput_t Btn {BTN_PIN};

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
//    if(Sleep::WasInStandby()) {
//        Uart.Printf("WasStandby\r");
//        Sleep::ClearStandbyFlag();
//    }
    Clk.PrintFreqs();

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

    Btn.Init();
    TmrCheckBtn.InitAndStart(chThdGetSelfX());
//    TmrOff.InitAndStart(chThdGetSelfX());
    TmrCheckPill.InitAndStart(chThdGetSelfX());

//    PinSensors.Init();
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
    DevType = devtMaster;
    while(true) {
        __unused eventmask_t Evt = chEvtWaitAny(ALL_EVENTS);
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
                    Uart.Printf("Pill Conn\r");
                    if(DevType == devtMaster) OnPillConnMaster();
                    break;
                case pillJustDisconnected: Uart.Printf("Pill Discon\r"); break;
                default: break;
            }
        }
#endif

#if 1 // ==== Led sequence end ====
        if(Evt & EVT_LED_SEQ_END) {
            if(DevType == devtMaster) {
                Led.SetColor(GetPillColor(MasterMode));
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
    Uart.Printf("PillBefore: %d %d\r", PillMgr.Pill.TypeInt32, PillMgr.Pill.ChargeCnt);
    if(MasterMode == ptTest) {
        if(PillMgr.Pill.ChargeCnt != 0) {
            if(PillMgr.Pill.Type != ptMaster) PillMgr.Pill.ChargeCnt--; // Decrease charges
            if(PillMgr.WritePill() == OK) { // Write pill
                ShowPillOk();
                Uart.Printf("PillAfter: %d %d\r", PillMgr.Pill.TypeInt32, PillMgr.Pill.ChargeCnt);
            } // if write pill ok
            else ShowPillBad(); // Write failure
        } // if charge cnt != 0
        else ShowPillBad(); // ChargeCnt == 0
    } // if test
    else {
        // Prepare to write pill
        PillMgr.Pill.Type = MasterMode;
        if(MasterMode == ptVitamin or MasterMode == ptCure) PillMgr.Pill.ChargeCnt = 1;
        else PillMgr.Pill.ChargeCnt = 5;    // Panacea or energetic
        // Write pill
        if(PillMgr.WritePill() == OK) {
            ShowPillOk();
            Uart.Printf("PillAfter: %d %d\r", PillMgr.Pill.TypeInt32, PillMgr.Pill.ChargeCnt);
        }
        else ShowPillBad(); // Write failure
    } // if not test
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
}

//void ProcessButton(PinSnsState_t *PState, uint32_t Len) {
//    if(*PState == pssRising) {
//        Led.StartSequence(lsqOn);   // LED on
//        Vibro.Stop();
//        App.MustTransmit = true;
//        Uart.Printf("\rTx on");
//        App.TmrTxOff.Stop();    // do not stop tx after repeated btn press
//    }
//    else if(*PState == pssFalling) {
//        Led.StartSequence(lsqOff);
//        App.TmrTxOff.Start();
//    }
//}

#if UART_RX_ENABLED // ======================= Command processing ============================
void App_t::OnCmd(Shell_t *PShell) {
	Cmd_t *PCmd = &PShell->Cmd;
    __attribute__((unused)) int32_t dw32 = 0;  // May be unused in some configurations
    Uart.Printf("%S\r", PCmd->Name);
    // Handle command
    if(PCmd->NameIs("Ping")) {
        PShell->Ack(OK);
    }

    else PShell->Ack(CMD_UNKNOWN);
}
#endif

