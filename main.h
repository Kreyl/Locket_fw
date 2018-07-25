/*
 * main.h
 *
 *  Created on: 6 марта 2018 г.
 *      Author: Kreyl
 */

#pragma once

#define ID_MIN                  1
#define ID_MAX                  50

// Types of signal
#define SIGN_SILMARIL       0b0001UL
#define SIGN_OATH           0b0010UL
#define SIGN_LIGHT          0b0100UL
#define SIGN_DARK           0b1000UL

extern uint8_t ID;
