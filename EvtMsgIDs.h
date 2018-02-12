/*
 * EvtMsgIDs.h
 *
 *  Created on: 21 апр. 2017 г.
 *      Author: Kreyl
 */

#pragma once

enum EvtMsgId_t {
    evtIdNone = 0, // Always

    // Pretending to eternity
    evtIdShellCmd,
    evtIdEverySecond,

    // Audio
    evtIdSoundPlayStop,
    evtIdSoundFileEnd,

    // Usb
    evtIdUsbConnect,
    evtIdUsbDisconnect,
    evtIdUsbReady,
    evtIdUsbNewCmd,
    evtIdUsbInDone,
    evtIdUsbOutDone,

    // Misc periph
    evtIdButtons,
    evtIdAcc,
    evtIdPwrOffTimeout,

    // App specific
    evtIdLedDone,
    evtIdClash,
    evtIdSwingStart,
    evtIdSpinStart,
    evtIdStab,
    evtIdScrew,
};
