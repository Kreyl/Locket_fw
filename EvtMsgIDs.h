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
    evtIdAdcRslt,

    evtIdLedSeqDone,
    evtIdVibroSeqDone,

    // Not eternal
    evtIdButtons,
    evtIdRadioCmd,
    evtIdCheckRxTable,
};
