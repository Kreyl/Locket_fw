/*
 * kl_sd.cpp
 *
 *  Created on: 13.02.2013
 *      Author: kreyl
 */

#include "kl_sd.h"
#include "hal_sdc.h"
#include "hal_sdc_lld.h"
#include <string.h>
#include <stdlib.h>
#include "kl_lib.h"
#include "shell.h"

sd_t SD;

// Using ltm32L476, do not forget to enable SDMMC Clock somewhere else
void sd_t::Init() {
    IsReady = FALSE;
    // Bus pins
    PinSetupAlterFunc(SD_DAT0);
    PinSetupAlterFunc(SD_DAT1);
    PinSetupAlterFunc(SD_DAT2);
    PinSetupAlterFunc(SD_DAT3);
    PinSetupAlterFunc(SD_CLK);
    PinSetupAlterFunc(SD_CMD);
    // Power pin
    chThdSleepMilliseconds(1);    // Let power to stabilize
    PinSetupOut(SD_PWR_PIN, omPushPull);
    PinSetLo(SD_PWR_PIN); // Power on
    chThdSleepMilliseconds(4);    // Let power to stabilize

    FRESULT err;
    sdcInit();
    sdcStart(&SDCD1, NULL);
    if(sdcConnect(&SDCD1)) {
        Printf("SD connect error\r");
        return;
    }
    else {
        Printf("SD capacity: %u\r", SDCD1.capacity);
    }
    err = f_mount(&SDC_FS, "", 0);
    if(err != FR_OK) {
        Printf("SD mount error\r");
        sdcDisconnect(&SDCD1);
        return;
    }
    IsReady = true;
}

void sd_t::Standby() {
    PinSetHi(SD_PWR_PIN); // Power off
}

uint8_t sd_t::Reconnect() {
    PinSetHi(SD_PWR_PIN); // Power off
    chThdSleepMilliseconds(45);    // Let power to stabilize
    sdcDisconnect(&SDCD1);
    sdcStop(&SDCD1);
    PinSetLo(SD_PWR_PIN); // Power on
    chThdSleepMilliseconds(45);    // Let power to stabilize
    sdcStart(&SDCD1, NULL);
    if(sdcConnect(&SDCD1)) {
        Printf("SD connect error\r");
        return retvFail;
    }
    if(f_mount(&SDC_FS, "", 0) != FR_OK) {
        Printf("SD mount error\r");
        sdcDisconnect(&SDCD1);
        return retvFail;
    }
    IsReady = true;
    Printf("SD Reconnected\r");
    return retvOk;
}

void sd_t::Resume() {
    PinSetLo(SD_PWR_PIN); // Power on
    chThdSleepMilliseconds(45);    // Let power to stabilize
}

extern "C"
void SDSignalError() {
    Printf("SD error\r");
    SD.IsReady = false;
}

// ============================== Hardware =====================================
extern "C" {

bool sdc_lld_is_card_inserted(SDCDriver *sdcp) { return TRUE; }
bool sdc_lld_is_write_protected(SDCDriver *sdcp) { return FALSE; }

}
