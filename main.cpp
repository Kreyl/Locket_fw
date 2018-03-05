/*
 * main.cpp
 *
 *  Created on: 20 февр. 2014 г.
 *      Author: g.kruglov
 */

#include "kl_lib.h"
#include "shell.h"
#include "MsgQ.h"
#include "uart.h"
#include "kl_i2c.h"

EvtMsgQ_t<EvtMsg_t, MAIN_EVT_Q_LEN> EvtQMain;
extern CmdUart_t Uart;
//void OnCmd(Shell_t *PShell);
//void ITask();

int main(void) {
    // ==== Init Vcore & clock system ====
    SetupVCore(vcore1V2);
    Clk.UpdateFreqValues();

    // === Init OS ===
    halInit();
    chSysInit();

    Uart.Init(115200);
    Printf("\rLocket\r");
    Clk.PrintFreqs();

    // ==== Init hardware ====
    PinSetupOut(GPIOB, 5, omPushPull);
    PinSetupInput(GPIOA, 0, pudPullDown);

    // Main cycle
    while(true) {
        if(PinIsHi(GPIOA, 0)) {
            PinSetHi(GPIOB, 5);
            chThdSleepMilliseconds(99);
            PinSetLo(GPIOB, 5);
            chThdSleepMilliseconds(99);
        }
        else chThdSleepMilliseconds(99);
    }
}
