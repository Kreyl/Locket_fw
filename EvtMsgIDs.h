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
    evtIdShellCmd = 1,
    evtIdEverySecond = 2,
    evtIdAdcRslt = 3,

    // Not eternal
    evtIdButtons = 15,
    evtIdRadioCmd = 18,
};
