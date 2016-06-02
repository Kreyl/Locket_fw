/*
 * main.h
 *
 *  Created on: 15 ����. 2014 �.
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
    DeviceType_t DevType;
    // Master device
    bool BtnWasHi = false;
    PillType_t MasterMode = ptVitamin;
    void OnPillConnMaster();
    // Player device
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
