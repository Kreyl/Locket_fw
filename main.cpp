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

#define SNSADDR1    0x48
#define SNSADDR2    0x49
#define SNSADDR3    0x4A

#define T_CONST     31250L

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
    Clk.PrintFreqs();

    // ==== Init hardware ====
    PinSetupOut(GPIOA, 7, omPushPull);

    // Main cycle
    while(true) {
        PinSetHi(GPIOA, 7);
        chThdSleepMilliseconds(99);
        PinSetLo(GPIOA, 7);
        chThdSleepMilliseconds(99);
    }
}
