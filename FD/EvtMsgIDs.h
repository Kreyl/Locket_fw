/*
 * EvtMsgIDs.h
 *
 *  Created on: 2017
 *      Author: Kreyl
 */

#ifndef EVTMSGIDS_H__
#define EVTMSGIDS_H__

enum class EvtId : uint8_t {
    None = 0, // Always

    // Pretending to eternity
    ShellCmd,
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

#endif //EVTMSGIDS_H__
