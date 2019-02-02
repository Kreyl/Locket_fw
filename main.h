/*
 * main.h
 *
 *  Created on: 6 марта 2018 г.
 *      Author: Kreyl
 */

#pragma once

#define ID_MIN                  1
#define ID_MAX                  15
#define ID_DEFAULT              ID_MIN
extern int32_t ID;

enum AppState_t {appstIdle = 0, appstTx1 = 1, appstTx2 = 2};
extern AppState_t AppState;
