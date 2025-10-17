/*
 * Sequences.h
 *
 *  Created on: 2015
 *      Author: Kreyl
 */

#ifndef SEQUENCES_H__
#define SEQUENCES_H__

#include "ChunkTypes.h"


#if 0 // ============================ LED blink ================================
const LedChunk_t lsqIdle[] = {
        {csSetup, 0, clBlack},
        {csEnd}
};

const LedChunk_t lsqError[] = {
        {csSetup, 0, clRed},
        {csWait, 4005},
        {csSetup, 0, clBlack},
        {csEnd}
};

// ======= Adding / removing IDs ========
// ==== Access ====
#define LSQ_ACCESS_ADD_CLR      clGreen
#define LSQ_ACCESS_REMOVE_CLR   clRed
const LedChunk_t lsqAddingAccessWaiting[] = {
        {csSetup, 0, LSQ_ACCESS_ADD_CLR},
        {csEnd}
};
const LedChunk_t lsqAddingAccessNew[] = {
        {csSetup, 0, clBlack},
        {csWait, 180},
        {csSetup, 0, LSQ_ACCESS_ADD_CLR},
        {csEnd}
};
const LedChunk_t lsqAddingAccessError[] = {
        {csSetup, 0, clRed},
        {csWait, 99},
        {csSetup, 0, clBlack},
        {csWait, 99},
        {csSetup, 0, clRed},
        {csWait, 99},
        {csSetup, 0, clBlack},
        {csWait, 99},
        {csSetup, 0, clRed},
        {csWait, 99},
        {csSetup, 0, clBlack},
        {csWait, 99},
        {csSetup, 0, LSQ_ACCESS_ADD_CLR},
        {csEnd}
};

const LedChunk_t lsqRemovingAccessWaiting[] = {
        {csSetup, 0, LSQ_ACCESS_REMOVE_CLR},
        {csEnd}
};
const LedChunk_t lsqRemovingAccessNew[] = {
        {csSetup, 0, clBlack},
        {csWait, 180},
        {csSetup, 0, LSQ_ACCESS_REMOVE_CLR},
        {csEnd}
};

// ==== Adder ====
#define LSQ_ADDER_ADD_CLR       clBlue
#define LSQ_ADDER_REMOVE_CLR    clMagenta
const LedChunk_t lsqAddingAdderWaiting[] = {
        {csSetup, 0, LSQ_ADDER_ADD_CLR},
        {csEnd}
};
const LedChunk_t lsqAddingAdderNew[] = {
        {csSetup, 0, clBlack},
        {csWait, 180},
        {csSetup, 0, LSQ_ADDER_ADD_CLR},
        {csEnd}
};
const LedChunk_t lsqAddingAdderError[] = {
        {csSetup, 0, clRed},
        {csWait, 99},
        {csSetup, 0, clBlack},
        {csWait, 99},
        {csSetup, 0, clRed},
        {csWait, 99},
        {csSetup, 0, clBlack},
        {csWait, 99},
        {csSetup, 0, clRed},
        {csWait, 99},
        {csSetup, 0, clBlack},
        {csWait, 99},
        {csSetup, 0, LSQ_ADDER_ADD_CLR},
        {csEnd}
};

const LedChunk_t lsqRemovingAdderWaiting[] = {
        {csSetup, 0, LSQ_ADDER_REMOVE_CLR},
        {csEnd}
};
const LedChunk_t lsqRemovingAdderNew[] = {
        {csSetup, 0, clBlack},
        {csWait, 180},
        {csSetup, 0, LSQ_ADDER_REMOVE_CLR},
        {csEnd}
};

// ==== Remover ====
#define LSQ_REMOVER_ADD_CLR     clCyan
#define LSQ_REMOVER_REMOVE_CLR  clYellow
const LedChunk_t lsqAddingRemoverWaiting[] = {
        {csSetup, 0, LSQ_REMOVER_ADD_CLR},
        {csEnd}
};
const LedChunk_t lsqAddingRemoverNew[] = {
        {csSetup, 0, clBlack},
        {csWait, 180},
        {csSetup, 0, LSQ_REMOVER_ADD_CLR},
        {csEnd}
};
const LedChunk_t lsqAddingRemoverError[] = {
        {csSetup, 0, clRed},
        {csWait, 99},
        {csSetup, 0, clBlack},
        {csWait, 99},
        {csSetup, 0, clRed},
        {csWait, 99},
        {csSetup, 0, clBlack},
        {csWait, 99},
        {csSetup, 0, clRed},
        {csWait, 99},
        {csSetup, 0, clBlack},
        {csWait, 99},
        {csSetup, 0, LSQ_REMOVER_ADD_CLR},
        {csEnd}
};

const LedChunk_t lsqRemovingRemoverWaiting[] = {
        {csSetup, 0, LSQ_REMOVER_REMOVE_CLR},
        {csEnd}
};
const LedChunk_t lsqRemovingRemoverNew[] = {
        {csSetup, 0, clBlack},
        {csWait, 180},
        {csSetup, 0, LSQ_REMOVER_REMOVE_CLR},
        {csEnd}
};

// ==== Erase all ====
const LedChunk_t lsqEraseAll[] = {
        {csSetup, 0, clRed},
        {csWait, 180},
        {csSetup, 0, clBlack},
        {csWait, 180},
        {csSetup, 0, clRed},
        {csWait, 180},
        {csSetup, 0, clBlack},
        {csWait, 180},
        {csSetup, 0, clRed},
        {csWait, 180},
        {csSetup, 0, clBlack},
        {csWait, 180},
        {csSetup, 0, clRed},
        {csWait, 180},
        {csSetup, 0, clBlack},
        {csEnd}
};

// General
const LedChunk_t lsqBlinkGreen[] = {
        {csSetup, 0, clGreen},
        {csWait, 180},
        {csSetup, 0, clBlack},
        {csEnd}
};

const LedChunk_t lsqBlinkGreenX2[] = {
        {csSetup, 0, clGreen},
        {csWait, 180},
        {csSetup, 0, clBlack},
        {csWait, 180},
        {csSetup, 0, clGreen},
        {csWait, 180},
        {csSetup, 0, clBlack},
//        {csWait, 999},
//        {csGoto, 0}
        {csEnd}
};
#endif

#if 1 // ============================ LED RGB ==================================
inline constexpr const unsigned long kBlinkDuration = 108, kBlinkDark = 180, kPauseNextLsq = 450;

// Self
static const LedRGBChunk lsqLvl1[] = {
    {csSetup, 0, {2, 2, 0} }, {csEnd},
};
static const LedRGBChunk lsqLvl2[] = {
    {csSetup, 0, {2, 0, 2} }, {csEnd},
};

static const LedRGBChunk lsqDeadLvl1[] = {
    {csSetup, 0, clRed},    {csWait, kBlinkDuration},
    {csSetup, 0, clBlack},  {csWait, kBlinkDark},
    {csSetup, 0, clYellow}, {csWait, kBlinkDuration},
    {csSetup, 0, clBlack},  {csWait, 3600U},
    {csGoto, 0}
};

static const LedRGBChunk lsqDeadLvl2[] = {
    {csSetup, 0, clRed},     {csWait, kBlinkDuration},
    {csSetup, 0, clBlack},   {csWait, kBlinkDark},
    {csSetup, 0, clMagenta}, {csWait, kBlinkDuration},
    {csSetup, 0, clBlack},   {csWait, 3600U},
    {csGoto, 0}
};

// Near
static const LedRGBChunk lsqLocketIsNear[] = {
    {csSetup, 0, clBlue},   {csWait, kBlinkDuration},
    {csSetup, 0, clBlack},
    {csWait, kPauseNextLsq},
    {csEnd},
};
static const LedRGBChunk lsqVoteAccepted[] = {
    {csSetup, 0, clGreen},   {csWait, kBlinkDuration},
    {csSetup, 0, clBlack},
    {csWait, kPauseNextLsq},
    {csEnd},
};

static const LedRGBChunk lsqSpeaksWthWH[] = {
    {csSetup, 0, {0, 36, 36} }, {csEnd},
};

static LedRGBChunk lsqDieNow[] = {
    {csSetup, 0, clRed},
    {csWait,  3006},
    {csSetup, 0, clBlack},
    {csWait, kPauseNextLsq},
    {csEnd},
};


static LedRGBChunk lsqDischarged[] = {
        {csSetup, 0, clWhite},  {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kPauseNextLsq},
        {csEnd},
};


const LedRGBChunk lsqStart[] = {
        {csSetup, 0, clRed},   {csWait, 450},
        {csSetup, 0, clGreen}, {csWait, 450},
        {csSetup, 0, clBlue},  {csWait, 450},
        {csSetup, 0, clBlack},
        // {csSetup, 0, {0,1,0}},
        {csEnd},
};

const LedRGBChunk lsqFailure[] = {
        {csSetup, 0, clRed},
        {csWait, 45},
        {csSetup, 0, clBlack},
        {csWait, 45},
        {csRepeat, 1},
        {csEnd}
};

const LedRGBChunk lsqBlink[] = {
        {csSetup, 0, clGreen}, {csWait, 63},
//        {csSetup, 0, clBlack},
        {csSetup, 0, {0,1,0}},
        {csEnd},
};

#pragma region // ==== Pill indication ====
inline constexpr const unsigned long kPillBlinkDur = 630, kAfterPillDelay = 2007;
const LedRGBChunk lsqPillBad[] = {
        {csSetup, 0, clRed},   {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kBlinkDark},
        {csRepeat, 2},
        {csWait, kAfterPillDelay},
        {csEnd}
};

const LedRGBChunk lsqPillLvl1[] = {
        {csSetup, 0, clYellow}, {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kBlinkDark},
        {csRepeat, 2},
        {csWait, kAfterPillDelay},
        {csEnd}
};
const LedRGBChunk lsqPillLvl2[] = {
        {csSetup, 0, clMagenta}, {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kBlinkDark},
        {csRepeat, 2},
        {csWait, kAfterPillDelay},
        {csEnd}
};
#pragma endregion

#endif

#if 0 // =========================== LED Smooth ================================
#define LED_TOP_BRIGHTNESS  255

const LedSmoothChunk_t lsqFadeIn[] = {
        {csSetup, 630, LED_TOP_BRIGHTNESS},
        {csEnd}
};
const LedSmoothChunk_t lsqFadeOut[] = {
        {csSetup, 630, 0},
        {csEnd}
};
const LedSmoothChunk_t lsqEnterActive[] = {
        {csSetup, 0, LED_TOP_BRIGHTNESS},
        {csEnd}
};
const LedSmoothChunk_t lsqEnterIdle[] = {
        {csSetup, 360, 0},
        {csEnd}
};

#endif

#if 0 // ============================= Beeper ==================================
#define BEEP_VOLUME     2   // Maximum 10

#if 1 // ==== Notes ====
#define La_2    880

#define Do_3    1047
#define Do_D_3  1109
#define Re_3    1175
#define Re_D_3  1245
#define Mi_3    1319
#define Fa_3    1397
#define Fa_D_3  1480
#define Sol_3   1568
#define Sol_D_3 1661
#define La_3    1720
#define Si_B_3  1865
#define Si_3    1976

#define Do_4    2093
#define Do_D_4  2217
#define Re_4    2349
#define Re_D_4  2489
#define Mi_4    2637
#define Fa_4    2794
#define Fa_D_4  2960
#define Sol_4   3136
#define Sol_D_4 3332
#define La_4    3440
#define Si_B_4  3729
#define Si_4    3951

// Length
#define OneSixteenth    90
#define OneEighth       (OneSixteenth * 2)
#define OneFourth       (OneSixteenth * 4)
#define OneHalfth       (OneSixteenth * 8)
#define OneWhole        (OneSixteenth * 16)
#endif

// Type, BEEP_VOLUME, freq
const BeepChunk_t bsqOn[] = {
        {csSetup, 10, 7000},
        {csEnd}
};

const BeepChunk_t bsqButton[] = {
        {csSetup, 1, 1975},
        {csWait, 54},
        {csSetup, 0},
        {csEnd}
};
const BeepChunk_t bsqBeepBeep[] = {
        {csSetup, BEEP_VOLUME, 1975},
        {csWait, 99},
        {csSetup, 0, 0},
        {csWait, 99},
        {csSetup, BEEP_VOLUME, 1975},
        {csWait, 99},
        {csSetup, 0, 0},
        {csEnd}
};

#if 1 // ==== Extensions ====
// Pill
const BeepChunk_t bsqBeepPillOk[] = {
        {csSetup, BEEP_VOLUME, Si_3},
        {csWait, 180},
        {csSetup, BEEP_VOLUME, Re_D_4},
        {csWait, 180},
        {csSetup, BEEP_VOLUME, Fa_D_4},
        {csWait, 180},
        {csSetup, 0},
        {csEnd}
};

const BeepChunk_t bsqBeepPillBad[] = {
        {csSetup, BEEP_VOLUME, Fa_4},
        {csWait, 180},
        {csSetup, BEEP_VOLUME, Re_4},
        {csWait, 180},
        {csSetup, BEEP_VOLUME, Si_3},
        {csWait, 180},
        {csSetup, 0},
        {csEnd}
};
#endif // ext
#endif // beeper

#if 1 // ============================== Vibro ==================================
#define VIBRO_VOLUME    100  // 1 to 100

#define VIBRO_SHORT_MS          126
#define VIBRO_LONG_MS           360
#define VIBRO_REPEAT_PERIOD     360

const BaseChunk_t vsqBrr[] = {
        {csSetup, VIBRO_VOLUME},
        {csWait, VIBRO_SHORT_MS},
        {csSetup, 0},
        {csWait, VIBRO_REPEAT_PERIOD},
        {csEnd}
};

const BaseChunk_t vsqBrrBrr[] = {
        {csSetup, VIBRO_VOLUME},
        {csWait, VIBRO_SHORT_MS},
        {csSetup, 0},
        {csWait, 99},
        {csSetup, VIBRO_VOLUME},
        {csWait, VIBRO_SHORT_MS},
        {csSetup, 0},
        {csWait, VIBRO_REPEAT_PERIOD},
        {csEnd}
};

const BaseChunk_t vsqBrrBrrBrr[] = {
        {csSetup, VIBRO_VOLUME},
        {csWait, VIBRO_SHORT_MS},
        {csSetup, 0},
        {csWait, 99},
        {csSetup, VIBRO_VOLUME},
        {csWait, VIBRO_SHORT_MS},
        {csSetup, 0},
        {csWait, 99},
        {csSetup, VIBRO_VOLUME},
        {csWait, VIBRO_SHORT_MS},
        {csSetup, 0},
        {csWait, VIBRO_REPEAT_PERIOD},
        {csEnd}
};


const BaseChunk_t vsqLongBrr[] = {
        {csSetup, VIBRO_VOLUME},
        {csWait, VIBRO_LONG_MS},
        {csSetup, 0},
        {csWait, VIBRO_REPEAT_PERIOD},
        {csEnd}
};

const BaseChunk_t vsqBrrForever[] = {
        {csSetup, VIBRO_VOLUME}, {csWait, VIBRO_SHORT_MS},
        {csSetup, 0},            {csWait, 2007},
        {csGoto, 0}
};

const BaseChunk_t vsqBtn[] = {
        {csSetup, VIBRO_VOLUME},
        {csWait, 81},
        {csSetup, 0},
        {csWait, VIBRO_REPEAT_PERIOD},
        {csEnd}
};

const BaseChunk_t vsqDieNow[] = {
    {csSetup, VIBRO_VOLUME},
    {csWait, 2007},
    {csSetup, 0},
    {csWait, VIBRO_REPEAT_PERIOD},
    {csEnd}
};

const BaseChunk_t vsqRegistered[] = {
    {csSetup, VIBRO_VOLUME},
    {csWait, 900},
    {csSetup, 0},
    {csWait, VIBRO_REPEAT_PERIOD},
    {csEnd}
};
#endif

#endif //SEQUENCES_H__
