/*
 * Sequences.h
 *
 *  Created on: 09 ���. 2015 �.
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
#define LOW_BRTNESS     4
#define SHOWTIME        180
#define PAUSETIME       270
#define SHORTPAUSETIME  72

#pragma region // ==== Start-up indication ====
const LedRGBChunk_t lsqSearcher[] = {
        {csSetup, 0, clYellow},  {csWait, SHOWTIME},
        {csSetup, 0, clBlack},   {csWait, SHOWTIME},
        {csRepeat, 1},
        {csWait, PAUSETIME},
        {csEnd},
};

const LedRGBChunk_t lsqPlacePlus1[] = {
        {csSetup, 0, clBlue},  {csWait, SHOWTIME},
        {csSetup, 0, clBlack}, {csWait, PAUSETIME},
        {csEnd},
};
const LedRGBChunk_t lsqPlacePlus2[] = {
        {csSetup, 0, clBlue},  {csWait, SHOWTIME},
        {csSetup, 0, clBlack}, {csWait, SHOWTIME},
        {csRepeat, 1},
        {csWait, PAUSETIME},
        {csEnd},
};
const LedRGBChunk_t lsqPlacePlus3[] = {
        {csSetup, 0, clBlue},  {csWait, SHOWTIME},
        {csSetup, 0, clBlack}, {csWait, SHOWTIME},
        {csRepeat, 2},
        {csWait, PAUSETIME},
        {csEnd},
};

const LedRGBChunk_t lsqMaster[] = {
        {csSetup, 0, clWhite},  {csWait, SHOWTIME},
        {csSetup, 0, {1,1,1}},
        {csEnd},
};

const LedRGBChunk_t lsqPlaceMinus1[] = {
        {csSetup, 0, clRed},   {csWait, SHOWTIME},
        {csSetup, 0, clBlack}, {csWait, PAUSETIME},
        {csEnd},
};
const LedRGBChunk_t lsqPlaceMinus2[] = {
        {csSetup, 0, clRed},   {csWait, SHOWTIME},
        {csSetup, 0, clBlack}, {csWait, SHOWTIME},
        {csRepeat, 1},
        {csWait, PAUSETIME},
        {csEnd},
};
const LedRGBChunk_t lsqPlaceMinus3[] = {
        {csSetup, 0, clRed},   {csWait, SHOWTIME},
        {csSetup, 0, clBlack}, {csWait, SHOWTIME},
        {csRepeat, 2},
        {csWait, PAUSETIME},
        {csEnd},
};

const LedRGBChunk_t lsqArtifact[] = {
        {csSetup, 0, clMagenta},  {csWait, SHOWTIME},
        {csSetup, 0, clBlack},    {csWait, PAUSETIME},
        {csEnd},
};

const LedRGBChunk_t lsqBeast[] = {
        {csSetup, 0, clCyan},  {csWait, SHOWTIME},
        {csSetup, 0, clBlack}, {csWait, PAUSETIME},
        {csEnd},
};

const LedRGBChunk_t lsqPlaceMinus1Magic[] = {
        {csSetup, 0, clRed},   {csWait, SHOWTIME},
        {csSetup, 0, clBlack}, {csWait, SHOWTIME},
        {csSetup, 0, clGreen}, {csWait, SHOWTIME},
        {csSetup, 0, clBlack}, {csWait, PAUSETIME},
        {csEnd},
};
const LedRGBChunk_t lsqPlaceMinus2Magic[] = {
        {csSetup, 0, clRed},   {csWait, SHOWTIME},
        {csSetup, 0, clBlack}, {csWait, SHOWTIME},
        {csRepeat, 1},
        {csSetup, 0, clGreen},  {csWait, SHOWTIME},
        {csSetup, 0, clBlack}, {csWait, PAUSETIME},
        {csEnd},
};
const LedRGBChunk_t lsqPlaceMinus3Magic[] = {
        {csSetup, 0, clRed},   {csWait, SHOWTIME},
        {csSetup, 0, clBlack}, {csWait, SHOWTIME},
        {csRepeat, 2},
        {csSetup, 0, clGreen},  {csWait, SHOWTIME},
        {csSetup, 0, clBlack}, {csWait, PAUSETIME},
        {csEnd},
};

const LedRGBChunk_t lsqParticle[] = {
        {csSetup, 0, clYellow},  {csWait, SHOWTIME},
        {csSetup, 0, clBlack},   {csWait, PAUSETIME},
        {csEnd},
};

const LedRGBChunk_t lsqPath[] = {
        {csSetup, 0, clGreen}, {csWait, SHOWTIME},
        {csSetup, 0, clBlack}, {csWait, PAUSETIME},
        {csEnd},
};
#pragma endregion

#pragma region // ==== In-work indication ====
#define DARK_VALUE      7
#define DARK_RED        (Color_t){DARK_VALUE,0,0}
#define DARK_GREEN      (Color_t){0,DARK_VALUE,0}
#define DARK_BLUE       (Color_t){0,0,DARK_VALUE}

const LedRGBChunk_t lsqPlacePlus1_inwork[] = {
        {csSetup, 0, DARK_BLUE}, {csWait, SHOWTIME},
        {csSetup, 0, clBlack},   {csWait, PAUSETIME},
        {csEnd},
};
const LedRGBChunk_t lsqPlacePlus2_inwork[] = {
        {csSetup, 0, DARK_BLUE}, {csWait, SHOWTIME},
        {csSetup, 0, clBlack},   {csWait, SHOWTIME},
        {csRepeat, 1},
        {csWait, PAUSETIME},
        {csEnd},
};
const LedRGBChunk_t lsqPlacePlus3_inwork[] = {
        {csSetup, 0, DARK_BLUE}, {csWait, SHOWTIME},
        {csSetup, 0, clBlack},   {csWait, SHOWTIME},
        {csRepeat, 2},
        {csWait, PAUSETIME},
        {csEnd},
};

const LedRGBChunk_t lsqMaster_inwork[] = {
        {csSetup, 0, clWhite},  {csWait, SHOWTIME},
        {csSetup, 0, {1,1,1}},
        {csEnd},
};

const LedRGBChunk_t lsqPlaceMinus1_inwork[] = {
        {csSetup, 0, DARK_RED}, {csWait, SHOWTIME},
        {csSetup, 0, clBlack},  {csWait, PAUSETIME},
        {csEnd},
};
const LedRGBChunk_t lsqPlaceMinus2_inwork[] = {
        {csSetup, 0, DARK_RED}, {csWait, SHOWTIME},
        {csSetup, 0, clBlack},  {csWait, SHOWTIME},
        {csRepeat, 1},
        {csWait, PAUSETIME},
        {csEnd},
};
const LedRGBChunk_t lsqPlaceMinus3_inwork[] = {
        {csSetup, 0, DARK_RED}, {csWait, SHOWTIME},
        {csSetup, 0, clBlack},  {csWait, SHOWTIME},
        {csRepeat, 2},
        {csWait, PAUSETIME},
        {csEnd},
};

const LedRGBChunk_t lsqArtifact_inwork[] = {
        {csSetup, 0, {0, DARK_VALUE, DARK_VALUE}}, {csWait, SHOWTIME},
        {csSetup, 0, clBlack}, {csWait, PAUSETIME},
        {csEnd},
};

#define BEAST_VALUE 27
const LedRGBChunk_t lsqBeast1_inwork[] = {
        {csSetup, 0, {0, 4, 4}},  {csWait, SHOWTIME},
        {csSetup, 0, clBlack},    {csWait, PAUSETIME},
        {csEnd},
};
const LedRGBChunk_t lsqBeast2_inwork[] = {
        {csSetup, 0, {0, BEAST_VALUE, BEAST_VALUE}}, {csWait, SHOWTIME},
        {csSetup, 0, clBlack}, {csWait, PAUSETIME},
        {csEnd},
};
const LedRGBChunk_t lsqBeast3_inwork[] = {
        {csSetup, 0, {0, BEAST_VALUE, BEAST_VALUE}},  {csWait, SHOWTIME},
        {csSetup, 0, clBlack}, {csWait, SHOWTIME},
        {csRepeat, 1},
        {csWait, PAUSETIME},
        {csEnd},
};
const LedRGBChunk_t lsqBeast4_inwork[] = {
        {csSetup, 0, {0, BEAST_VALUE, BEAST_VALUE}},  {csWait, SHOWTIME},
        {csSetup, 0, clBlack}, {csWait, SHOWTIME},
        {csRepeat, 2},
        {csWait, PAUSETIME},
        {csEnd},
};
const LedRGBChunk_t lsqBeastMadness[] = {
        {csSetup, 0, clCyan},
        {csEnd},
};

const LedRGBChunk_t lsqPlaceMinus1Magic_inwork[] = {
        {csSetup, 0, DARK_RED},   {csWait, SHOWTIME},
        {csSetup, 0, clBlack},    {csWait, SHOWTIME},
        {csSetup, 0, DARK_GREEN}, {csWait, SHOWTIME},
        {csSetup, 0, clBlack},    {csWait, PAUSETIME},
        {csEnd},
};
const LedRGBChunk_t lsqPlaceMinus2Magic_inwork[] = {
        {csSetup, 0, DARK_RED},   {csWait, SHOWTIME},
        {csSetup, 0, clBlack},    {csWait, SHOWTIME},
        {csRepeat, 1},
        {csSetup, 0, DARK_GREEN}, {csWait, SHOWTIME},
        {csSetup, 0, clBlack},    {csWait, PAUSETIME},
        {csEnd},
};
const LedRGBChunk_t lsqPlaceMinus3Magic_inwork[] = {
        {csSetup, 0, DARK_RED},   {csWait, SHOWTIME},
        {csSetup, 0, clBlack},    {csWait, SHOWTIME},
        {csRepeat, 2},
        {csSetup, 0, DARK_GREEN}, {csWait, SHOWTIME},
        {csSetup, 0, clBlack},    {csWait, PAUSETIME},
        {csEnd},
};

const LedRGBChunk_t lsqPathFadeIn[] = {
        {csSetup, 720, clGreen},
        {csEnd},
};
const LedRGBChunk_t lsqPathFadeOut[] = {
        {csSetup, 720, clBlack},
        {csEnd},
};
#pragma endregion

#pragma region // ==== Master's indication ====
const LedRGBChunk_t lsqGreenEvil1[] = {
        {csSetup, 0, clGreen}, {csWait, SHOWTIME},
        {csSetup, 0, clBlack}, {csWait, PAUSETIME},
        {csEnd},
};
const LedRGBChunk_t lsqGreenEvil2[] = {
        {csSetup, 0, clGreen}, {csWait, SHOWTIME},
        {csSetup, 0, clBlack}, {csWait, SHOWTIME},
        {csRepeat, 1},
        {csWait, PAUSETIME},
        {csEnd},
};
const LedRGBChunk_t lsqGreenEvil3[] = {
        {csSetup, 0, clGreen}, {csWait, SHOWTIME},
        {csSetup, 0, clBlack}, {csWait, SHOWTIME},
        {csRepeat, 2},
        {csWait, PAUSETIME},
        {csEnd},
};

const LedRGBChunk_t lsqArtifact1[] = {
        {csSetup, 0, clMagenta}, {csWait, SHOWTIME},
        {csSetup, 0, clBlack}, {csWait, PAUSETIME},
        {csEnd},
};
const LedRGBChunk_t lsqArtifact2[] = {
        {csSetup, 0, clMagenta}, {csWait, SHOWTIME},
        {csSetup, 0, clBlack}, {csWait, SHOWTIME},
        {csRepeat, 1},
        {csWait, PAUSETIME},
        {csEnd},
};
const LedRGBChunk_t lsqArtifact3[] = {
        {csSetup, 0, clMagenta}, {csWait, SHOWTIME},
        {csSetup, 0, clBlack}, {csWait, SHOWTIME},
        {csRepeat, 2},
        {csWait, PAUSETIME},
        {csEnd},
};

const LedRGBChunk_t lsqCyanBeast1[] = {
        {csSetup, 0, clCyan}, {csWait, SHOWTIME},
        {csSetup, 0, clBlack}, {csWait, PAUSETIME},
        {csEnd},
};
const LedRGBChunk_t lsqCyanBeast2[] = {
        {csSetup, 0, clCyan}, {csWait, SHOWTIME},
        {csSetup, 0, clBlack}, {csWait, SHOWTIME},
        {csRepeat, 1},
        {csWait, PAUSETIME},
        {csEnd},
};
const LedRGBChunk_t lsqCyanBeast3[] = {
        {csSetup, 0, clCyan}, {csWait, SHOWTIME},
        {csSetup, 0, clBlack}, {csWait, SHOWTIME},
        {csRepeat, 2},
        {csWait, PAUSETIME},
        {csEnd},
};

const LedRGBChunk_t lsqSearcher1[] = {
        {csSetup, 0, clYellow}, {csWait, SHOWTIME},
        {csSetup, 0, clBlack}, {csWait, PAUSETIME},
        {csEnd},
};
const LedRGBChunk_t lsqSearcher2[] = {
        {csSetup, 0, clYellow}, {csWait, SHOWTIME},
        {csSetup, 0, clBlack}, {csWait, SHOWTIME},
        {csRepeat, 1},
        {csWait, PAUSETIME},
        {csEnd},
};
const LedRGBChunk_t lsqSearcher3[] = {
        {csSetup, 0, clYellow}, {csWait, SHOWTIME},
        {csSetup, 0, clBlack}, {csWait, SHOWTIME},
        {csRepeat, 2},
        {csWait, PAUSETIME},
        {csEnd},
};
#pragma endregion

// === Goodness ===
const LedRGBChunk_t lsqGoodnessBlue[] = {
        {csSetup, 0, clBlue}, {csWait, SHOWTIME},
        {csSetup, 0, clBlack}, {csWait, PAUSETIME},
        {csEnd},
};
const LedRGBChunk_t lsqGoodnessYellow[] = {
        {csSetup, 0, clYellow}, {csWait, SHOWTIME},
        {csSetup, 0, clBlack}, {csWait, PAUSETIME},
        {csEnd},
};
const LedRGBChunk_t lsqGoodnessRed[] = {
        {csSetup, 0, clRed}, {csWait, SHOWTIME},
        {csSetup, 0, clBlack}, {csWait, PAUSETIME},
        {csEnd},
};

const LedRGBChunk_t lsqGoodnessBlueFrozen[] = {
        {csSetup, 0, clBlue}, {csWait, SHOWTIME},
        {csSetup, 0, clBlack}, {csWait, SHOWTIME},
        {csRepeat, 1},
        {csWait, PAUSETIME},
        {csEnd},
};
const LedRGBChunk_t lsqGoodnessYellowFrozen[] = {
        {csSetup, 0, clYellow}, {csWait, SHOWTIME},
        {csSetup, 0, clBlack}, {csWait, SHOWTIME},
        {csRepeat, 1},
        {csWait, PAUSETIME},
        {csEnd},
};
const LedRGBChunk_t lsqGoodnessRedFrozen[] = {
        {csSetup, 0, clRed}, {csWait, SHOWTIME},
        {csSetup, 0, clBlack}, {csWait, SHOWTIME},
        {csRepeat, 1},
        {csWait, PAUSETIME},
        {csEnd},
};

// === Magic ===
const LedRGBChunk_t lsqMagicGrenEvil[] = {
        {csSetup, 0, clGreen}, {csWait, SHOWTIME},
        {csSetup, 0, clBlack}, {csWait, PAUSETIME},
        {csEnd},
};
const LedRGBChunk_t lsqMagicArtifact[] = {
        {csSetup, 0, clMagenta}, {csWait, SHOWTIME},
        {csSetup, 0, clBlack}, {csWait, PAUSETIME},
        {csEnd},
};
const LedRGBChunk_t lsqMagicBeast[] = {
        {csSetup, 0, clCyan}, {csWait, SHOWTIME},
        {csSetup, 0, clBlack}, {csWait, PAUSETIME},
        {csEnd},
};


const LedRGBChunk_t lsqStart[] = {
        {csSetup, 0, clRed},   {csWait, 450},
        {csSetup, 0, clGreen}, {csWait, 450},
        {csSetup, 0, clBlue},  {csWait, 450},
//        {csSetup, 0, clBlack},
        {csSetup, 0, {0,1,0}},
        {csEnd},
};

const LedRGBChunk_t lsqFailure[] = {
        {csSetup, 0, clRed},
        {csWait, 45},
        {csSetup, 0, clBlack},
        {csWait, 45},
        {csRepeat, 1},
        {csEnd}
};

const LedRGBChunk_t lsqBlink[] = {
        {csSetup, 0, clGreen}, {csWait, 63},
//        {csSetup, 0, clBlack},
        {csSetup, 0, {0,1,0}},
        {csEnd},
};

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

#define VIBRO_SHORT_MS          99
#define VIBRO_LONG_MS           207
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
#endif

#endif //SEQUENCES_H__
