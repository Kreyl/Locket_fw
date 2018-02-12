/*
 * main.cpp
 *
 *  Created on: 20 февр. 2014 г.
 *      Author: g.kruglov
 */

#include "kl_lib.h"

int main(void) {
    // ==== Init Vcore & clock system ====
    SetupVCore(vcore1V2);
    Clk.UpdateFreqValues();

    // === Init OS ===
    halInit();
    chSysInit();

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
