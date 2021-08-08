/*
 * EvtMsgIDs.h
 *
 *  Created on: 21 ���. 2017 �.
 *      Author: Kreyl
 */

#pragma once

enum EvtMsgId_t {
    evtIdNone = 0, // Always

    // Pretending to eternity
    evtIdShellCmd,
    evtIdEverySecond,
    evtIdAdcRslt,

    evtIdLedSeqDone,
    evtIdVibroSeqDone,

    // Not eternal
    evtIdButtons,
    evtIdRadioCmd,
    evtIdCheckRxTable,
};
