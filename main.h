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

enum Mode_t {modeTx, modeRx};

class App_t {
private:
    thread_t *PThread;
public:
    Mode_t Mode;
    // Eternal methods
    void InitThread() { PThread = chThdGetSelfX(); }
    uint8_t GetDipSwitch();
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

#define DIP_SW_CNT      6
class DipSwitch_t {
private:
    PinInputSetup_t Pin[DIP_SW_CNT];
public:
    DipSwitch_t(const PinInputSetup_t Pin1, const PinInputSetup_t Pin2, const PinInputSetup_t Pin3,
            const PinInputSetup_t Pin4, const PinInputSetup_t Pin5, const PinInputSetup_t Pin6) {
        Pin[0] = Pin1;
        Pin[1] = Pin2;
        Pin[2] = Pin3;
        Pin[3] = Pin4;
        Pin[4] = Pin5;
        Pin[5] = Pin6;
    }
    uint8_t GetValue() const {
        uint8_t Rslt = 0;
        for(int i=0; i<DIP_SW_CNT; i++) PinSetupInput(Pin[i].PGpio, Pin[i].Pin, Pin[i].PullUpDown);
        for(int i=0; i<DIP_SW_CNT; i++) {
            if(!PinIsHi(Pin[i].PGpio, Pin[i].Pin)) Rslt |= (1 << i);
            PinSetupAnalog(Pin[i].PGpio, Pin[i].Pin);
        }
        return Rslt;
    }
};

extern App_t App;
