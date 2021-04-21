/*
 * Config.h
 *
 *  Created on: Jul 4, 2020
 *      Author: layst
 */

#pragma once

#include <vector>
#include "ChunkTypes.h"
#include <string>
#include "shell.h"
#include "Sequences.h"

#define ID_BAD                  0xFF
#define ID_MIN                  0
#define ID_MAX                  49 // Defines max count of lockets in the system
#define ID_DEFAULT              ID_MIN

enum Type_t : uint8_t { typeBlue = 0, typeGreen = 1, typeRed = 2, typeViolet = 3, typeOrange = 4, typeWhite = 5, typeYellow = 6 };

//struct Locket_t {
//    Type_t Type;
//    Color_t ColorActive, ColorIdle
//};

static Color_t ActivClr[] = { clCyan,       clGreen,       clRed,       clMagenta,      {255, 72, 0},  clWhite,       clYellow };
static Color_t  IdleClr[] = { clCyan,       clGreen,       clRed,       clMagenta,      {2, 2, 0},      clWhite,       {2, 2, 0} };

class Config_t {
private:
public:
    uint8_t ID = ID_DEFAULT;
    Type_t Type = typeBlue;
};

extern Config_t Cfg;
