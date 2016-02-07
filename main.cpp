/*
 * main.cpp
 *
 *  Created on: 20 февр. 2014 г.
 *      Author: g.kruglov
 */

#include "main.h"
#include "SimpleSensors.h"
#include "board.h"
#include "clocking.h"
#include "led.h"
#include "vibro.h"
#include "Sequences.h"
#include "LSM9DS0.h"

App_t App;

//Vibro_t Vibro(VIBRO_GPIO, VIBRO_PIN, VIBRO_TIM, VIBRO_CH);
LedRGB_t Led { {LED_GPIO, LED_R_PIN, LED_TIM, LED_R_CH}, {LED_GPIO, LED_G_PIN, LED_TIM, LED_G_CH}, {LED_GPIO, LED_B_PIN, LED_TIM, LED_B_CH} };

int main(void) {
    // ==== Init Vcore & clock system ====
    SetupVCore(vcore1V5);
    Clk.SetMSI4MHz();
    Clk.UpdateFreqValues();

    // Init OS
    halInit();
    chSysInit();
    App.InitThread();

    // ==== Init hardware ====
    Uart.Init(256000, UART_GPIO, UART_TX_PIN);//, UART_GPIO, UART_RX_PIN);
    Uart.Printf("\r%S %S\r", APP_NAME, APP_VERSION);
    Clk.PrintFreqs();

    Led.Init();
    Led.StartSequence(lsqOn);

    Acc.Init();

//    Vibro.Init();
//    Vibro.StartSequence(vsqBrr);
//    if(Radio.Init() != OK) {
//        Led.StartSequence(lsqFailure);
//        chThdSleepMilliseconds(2700);
//    }
//    PinSensors.Init();
    // Main cycle
    App.ITask();
}

__attribute__ ((__noreturn__))
void App_t::ITask() {
    while(true) {
        __unused eventmask_t Evt = chEvtWaitAny(ALL_EVENTS);
        if(Evt & EVT_NEW_9D) {
            Uart.Printf("G: %d %d %d; A: %d %d %d; M: %d %d %d\r",
                    Acc.IPRead->Gyro.x, Acc.IPRead->Gyro.y, Acc.IPRead->Gyro.z,
                    Acc.IPRead->Acc.x, Acc.IPRead->Acc.y, Acc.IPRead->Acc.z,
                    Acc.IPRead->Magnet.x, Acc.IPRead->Magnet.y, Acc.IPRead->Magnet.z);
        }

#if UART_RX_ENABLED
        if(EvtMsk & EVTMSK_UART_NEW_CMD) {
            OnCmd((Shell_t*)&Uart);
            Uart.SignalCmdProcessed();
        }
#endif

    } // while true
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

