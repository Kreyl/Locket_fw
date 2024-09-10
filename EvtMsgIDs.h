/*
 * EvtMsgIDs.h
 *
 *  Created on: 21 ���. 2017 �.
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

    // Not eternal
    Buttons,
    RadioCmd,
    CheckRxTable,
};

#endif //EVTMSGIDS_H__
