/*
 * main.h
 *
 *  Created on: 6 марта 2018 г.
 *      Author: Kreyl
 */

#pragma once

#define ID_MIN                  1
#define ID_MAX                  20
#define ID_DEFAULT              ID_MIN
extern int32_t ID;

enum AppMode_t : uint8_t {appmCrystal = 0, appmKey = 1, appmButton = 2};

extern AppMode_t AppMode;
extern bool ButtonMustTx;
