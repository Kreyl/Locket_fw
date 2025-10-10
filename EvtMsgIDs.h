/*
 * EvtMsgIDs.h
 *
 *  Created on: 2017
 *      Author: Kreyl
 */

#pragma once

enum class EvtId : uint8_t {
    None = 0, // Always

    // Pretending to eternity
    UartCheckTime,
    EverySecond,
    AdcRslt,

    LedSeqDone,
    VibroSeqDone,

    // Pill
    CheckPill,
    PillConnected,
    PillDisconnected,

    // Not eternal
    Buttons,
    RadioCmd,
    CheckRxTable,
};
