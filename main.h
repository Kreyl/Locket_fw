/*
 * main.h
 *
 *  Created on: 15 сент. 2014 г.
 *      Author: g.kruglov
 */

#pragma once

#include "ch.h"
#include "kl_lib.h"
#include "uart.h"
#include "evt_mask.h"
#include "board.h"
#include "pill.h"

enum DeviceType_t {devtPlayer, devtMaster};

class App_t {
private:
    thread_t *PThread;
    DeviceType_t DevType;
    // Master device
    bool BtnWasHi = false;
    PillType_t MasterMode = ptVitamin;
    void OnPillConnMaster();
    void OnPillDisconnMaster();
    // Common
    void ShowPillOk();
    void ShowPillBad();
public:
    // Eternal methods
    void InitThread() { PThread = chThdGetSelfX(); }
    void SignalEvt(eventmask_t Evt) {
        chSysLock();
        chEvtSignalI(PThread, Evt);
        chSysUnlock();
    }
    void SignalEvtI(eventmask_t Evt) { chEvtSignalI(PThread, Evt); }
    void OnCmd(Shell_t *PShell);
    // Inner use
    void ITask();
};

extern App_t App;
