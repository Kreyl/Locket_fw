/*
 * main.h
 *
 *  Created on: 6 марта 2018 г.
 *      Author: Kreyl
 */

#pragma once

#define ID_MIN                  1
#define ID_MAX                  16
#define ID_DEFAULT              ID_MIN
extern int32_t ID;

class Presser_t {
private:
    systime_t TimeOfPress;
    sysinterval_t TimeAfterPress;
    bool WasPressed = false;
    bool WasGet = true;
public:
    void Reset();
    int32_t GetTimeAfterPress();
    void IrqHandler();
};

extern Presser_t Presser;
