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

enum DeviceType_t {devUnknown, devtPlayer, devtMaster};

enum Condition_t {
    cndBlue,
    cndGreen,
    cndYellow,
    cndRed,
    cndVBRed,
    cndViolet, cndWhite
};

class CondTimer_t {
private:
    int VBDuration;
    int Duration, VBTime1, VBTime2;
public:
    void Init(int ADuration, int AVBTime1, int AVBTime2) {
        Duration = ADuration;
        VBTime1 = AVBTime1;
        VBTime2 = AVBTime2;
        VBDuration = -1;
    }
    void OnNewSecond();
};

// Durations
#define DURATION_BLUE_S         (15 * 60)
#define DURATION_GYR_MIN_S      (10 * 60)
#define DURATION_GYR_MAX_S      (15 * 60)
#define DURATION_VBRED_S        (3  * 60)
#define DURATION_BLINK_VIBRO_S  30

class App_t {
private:
    thread_t *PThread;
    DeviceType_t DevType = devUnknown;
    // Master device
    bool BtnWasHi = false;
    PillType_t MasterMode = ptVitamin;
    void OnPillConnMaster();
    // Player device
    bool NeedToRestoreIndication = false;
    CondTimer_t CndTmr;
    Condition_t Condition;
    void SetCondition(Condition_t NewCondition);
    void OnPillConnPlayer();
    uint32_t BlinkVibroCnt;
    // Common
    void ShowPillOk();
    void ShowPillBad();
public:
    void OnNextCondition();
    void DoVibroBlink();
    void StopVibroBlink();
    void SetupConditionIndication();
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
    PinInput_t Pin[DIP_SW_CNT];
public:
    DipSwitch_t(PinInput_t Pin1, PinInput_t Pin2, PinInput_t Pin3,
            PinInput_t Pin4, PinInput_t Pin5, PinInput_t Pin6) {
        Pin[0] = Pin1;
        Pin[1] = Pin2;
        Pin[2] = Pin3;
        Pin[3] = Pin4;
        Pin[4] = Pin5;
        Pin[5] = Pin6;
    }
    void Init() const {
        for(int i=0; i<DIP_SW_CNT; i++) {
            Pin[i].Init();
            Pin[i].Standby();
        }
    }
    uint8_t GetValue() const {
        uint8_t Rslt = 0;
        for(int i=0; i<DIP_SW_CNT; i++) {
            Pin[i].Resume();
            __NOP(); __NOP(); __NOP(); __NOP();
            if(!Pin[i].IsHi()) Rslt |= (1 << i);
            Pin[i].Standby();
        }
        return Rslt;
    }
};

extern App_t App;
