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

static LedRGBChunk lsqOneImmortal[] = {
        {csSetup, 0, clBlue},  {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kPauseNextLsq},
        {csEnd},
};

static LedRGBChunk lsqTwoImmortals[] = {
        {csSetup, 0, clBlue},  {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kBlinkDuration},
        {csRepeat, 1},
        {csWait, kPauseNextLsq},
        {csEnd},
};

static LedRGBChunk lsqManyImmortals[] = {
        {csSetup, 0, clBlue},  {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kBlinkDuration},
        {csRepeat, 2},
        {csWait, kPauseNextLsq},
        {csEnd},
};

static LedRGBChunk lsqPreImmortal[] = {
        {csSetup, 0, clGreen},  {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kPauseNextLsq},
        {csEnd},
};

static LedRGBChunk lsqDischarged[] = {
        {csSetup, 0, clRed},  {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kPauseNextLsq},
        {csEnd},
};


#pragma region // ==== Start-up indication ====
const LedRGBChunk lsqPlacePlus1[] = {
        {csSetup, 0, clBlue},  {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kPauseNextLsq},
        {csEnd},
};
const LedRGBChunk lsqPlacePlus2[] = {
        {csSetup, 0, clBlue},  {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kBlinkDuration},
        {csRepeat, 1},
        {csWait, kPauseNextLsq},
        {csEnd},
};
const LedRGBChunk lsqPlacePlus3[] = {
        {csSetup, 0, clBlue},  {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kBlinkDuration},
        {csRepeat, 2},
        {csWait, kPauseNextLsq},
        {csEnd},
};

const LedRGBChunk lsqMaster[] = {
        {csSetup, 0, clWhite},  {csWait, kBlinkDuration},
        {csSetup, 0, {1,1,1}},
        {csEnd},
};

const LedRGBChunk lsqPlaceMinus1[] = {
        {csSetup, 0, clRed},   {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kPauseNextLsq},
        {csEnd},
};
const LedRGBChunk lsqPlaceMinus2[] = {
        {csSetup, 0, clRed},   {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kBlinkDuration},
        {csRepeat, 1},
        {csWait, kPauseNextLsq},
        {csEnd},
};
const LedRGBChunk lsqPlaceMinus3[] = {
        {csSetup, 0, clRed},   {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kBlinkDuration},
        {csRepeat, 2},
        {csWait, kPauseNextLsq},
        {csEnd},
};

const LedRGBChunk lsqArtifact[] = {
        {csSetup, 0, clMagenta},  {csWait, kBlinkDuration},
        {csSetup, 0, clBlack},    {csWait, kPauseNextLsq},
        {csEnd},
};

const LedRGBChunk lsqBeast[] = {
        {csSetup, 0, clCyan},  {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kPauseNextLsq},
        {csEnd},
};

const LedRGBChunk lsqPlaceMinus1Magic[] = {
        {csSetup, 0, clRed},   {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kBlinkDuration},
        {csSetup, 0, clGreen}, {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kPauseNextLsq},
        {csEnd},
};
const LedRGBChunk lsqPlaceMinus2Magic[] = {
        {csSetup, 0, clRed},   {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kBlinkDuration},
        {csRepeat, 1},
        {csSetup, 0, clGreen},  {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kPauseNextLsq},
        {csEnd},
};
const LedRGBChunk lsqPlaceMinus3Magic[] = {
        {csSetup, 0, clRed},   {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kBlinkDuration},
        {csRepeat, 2},
        {csSetup, 0, clGreen},  {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kPauseNextLsq},
        {csEnd},
};

const LedRGBChunk lsqParticle[] = {
        {csSetup, 0, clYellow},  {csWait, kBlinkDuration},
        {csSetup, 0, clBlack},   {csWait, kPauseNextLsq},
        {csEnd},
};

const LedRGBChunk lsqPath[] = {
        {csSetup, 0, clGreen}, {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kPauseNextLsq},
        {csEnd},
};
#pragma endregion

#pragma region // ==== In-work indication ====
#define DARK_VALUE      4
#define DARK_RED        (Color_t){DARK_VALUE,0,0}
#define DARK_GREEN      (Color_t){0,DARK_VALUE,0}
#define DARK_BLUE       (Color_t){0,0,DARK_VALUE}

const LedRGBChunk lsqPlacePlus1_inwork[] = {
        {csSetup, 0, DARK_BLUE}, {csWait, kBlinkDuration},
        {csSetup, 0, clBlack},   {csWait, kPauseNextLsq},
        {csEnd},
};
const LedRGBChunk lsqPlacePlus2_inwork[] = {
        {csSetup, 0, DARK_BLUE}, {csWait, kBlinkDuration},
        {csSetup, 0, clBlack},   {csWait, kBlinkDark},
        {csRepeat, 1},
        {csWait, kPauseNextLsq},
        {csEnd},
};
const LedRGBChunk lsqPlacePlus3_inwork[] = {
        {csSetup, 0, DARK_BLUE}, {csWait, kBlinkDuration},
        {csSetup, 0, clBlack},   {csWait, kBlinkDark},
        {csRepeat, 2},
        {csWait, kPauseNextLsq},
        {csEnd},
};

const LedRGBChunk lsqMaster_inwork[] = {
        {csSetup, 0, {1,1,1}},
        {csEnd},
};

const LedRGBChunk lsqPlaceMinus1_inwork[] = {
        {csSetup, 0, DARK_RED}, {csWait, kBlinkDuration},
        {csSetup, 0, clBlack},  {csWait, kPauseNextLsq},
        {csEnd},
};
const LedRGBChunk lsqPlaceMinus2_inwork[] = {
        {csSetup, 0, DARK_RED}, {csWait, kBlinkDuration},
        {csSetup, 0, clBlack},  {csWait, kBlinkDark},
        {csRepeat, 1},
        {csWait, kPauseNextLsq},
        {csEnd},
};
const LedRGBChunk lsqPlaceMinus3_inwork[] = {
        {csSetup, 0, DARK_RED}, {csWait, kBlinkDuration},
        {csSetup, 0, clBlack},  {csWait, kBlinkDark},
        {csRepeat, 2},
        {csWait, kPauseNextLsq},
        {csEnd},
};

const LedRGBChunk lsqArtifact_inwork[] = {
        {csSetup, 0, {DARK_VALUE, 0, DARK_VALUE}}, {csWait, kBlinkDuration},
        {csSetup, 0, clBlack},                     {csWait, kPauseNextLsq},
        {csEnd},
};

#define BEAST_VALUE 27
const LedRGBChunk lsqBeast1_inwork[] = {
        {csSetup, 0, {0, 2, 2}},
        {csEnd},
};
const LedRGBChunk lsqBeast2_inwork[] = {
        {csSetup, 0, {0, BEAST_VALUE, BEAST_VALUE}}, {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kPauseNextLsq},
        {csEnd},
};
const LedRGBChunk lsqBeast3_inwork[] = {
        {csSetup, 0, {0, BEAST_VALUE, BEAST_VALUE}},  {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kBlinkDark},
        {csRepeat, 1},
        {csWait, kPauseNextLsq},
        {csEnd},
};
const LedRGBChunk lsqBeast4_inwork[] = {
        {csSetup, 0, {0, BEAST_VALUE, BEAST_VALUE}},  {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kBlinkDark},
        {csRepeat, 2},
        {csWait, kPauseNextLsq},
        {csEnd},
};
const LedRGBChunk lsqBeastMadness[] = {
        {csSetup, 0, clCyan},
        {csEnd},
};

const LedRGBChunk lsqPlaceMinus1Magic_inwork[] = {
        {csSetup, 0, DARK_RED},   {csWait, kBlinkDuration},
        {csSetup, 0, clBlack},    {csWait, kBlinkDark},
        {csSetup, 0, DARK_GREEN}, {csWait, kBlinkDuration},
        {csSetup, 0, clBlack},    {csWait, kPauseNextLsq},
        {csEnd},
};
const LedRGBChunk lsqPlaceMinus2Magic_inwork[] = {
        {csSetup, 0, DARK_RED},   {csWait, kBlinkDuration},
        {csSetup, 0, clBlack},    {csWait, kBlinkDark},
        {csRepeat, 1},
        {csSetup, 0, DARK_GREEN}, {csWait, kBlinkDuration},
        {csSetup, 0, clBlack},    {csWait, kPauseNextLsq},
        {csEnd},
};
const LedRGBChunk lsqPlaceMinus3Magic_inwork[] = {
        {csSetup, 0, DARK_RED},   {csWait, kBlinkDuration},
        {csSetup, 0, clBlack},    {csWait, kBlinkDark},
        {csRepeat, 2},
        {csSetup, 0, DARK_GREEN}, {csWait, kBlinkDuration},
        {csSetup, 0, clBlack},    {csWait, kPauseNextLsq},
        {csEnd},
};

const LedRGBChunk lsqPathFadeIn[] = {
        {csSetup, 720, clGreen},
        {csEnd},
};
const LedRGBChunk lsqPathFadeOut[] = {
        {csSetup, 720, clBlack},
        {csEnd},
};
#pragma endregion

#pragma region // ==== Master's indication ====
const LedRGBChunk lsqGreenEvil1[] = {
        {csSetup, 0, clGreen}, {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kPauseNextLsq},
        {csEnd},
};
const LedRGBChunk lsqGreenEvil2[] = {
        {csSetup, 0, clGreen}, {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kBlinkDark},
        {csRepeat, 1},
        {csWait, kPauseNextLsq},
        {csEnd},
};
const LedRGBChunk lsqGreenEvil3[] = {
        {csSetup, 0, clGreen}, {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kBlinkDark},
        {csRepeat, 2},
        {csWait, kPauseNextLsq},
        {csEnd},
};

const LedRGBChunk lsqArtifact1[] = {
        {csSetup, 0, clMagenta}, {csWait, kBlinkDuration},
        {csSetup, 0, clBlack},   {csWait, kPauseNextLsq},
        {csEnd},
};
const LedRGBChunk lsqArtifact2[] = {
        {csSetup, 0, clMagenta}, {csWait, kBlinkDuration},
        {csSetup, 0, clBlack},   {csWait, kBlinkDark},
        {csRepeat, 1},
        {csWait, kPauseNextLsq},
        {csEnd},
};
const LedRGBChunk lsqArtifact3[] = {
        {csSetup, 0, clMagenta}, {csWait, kBlinkDuration},
        {csSetup, 0, clBlack},   {csWait, kBlinkDark},
        {csRepeat, 2},
        {csWait, kPauseNextLsq},
        {csEnd},
};

const LedRGBChunk lsqCyanBeast1[] = {
        {csSetup, 0, clCyan},  {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kPauseNextLsq},
        {csEnd},
};
const LedRGBChunk lsqCyanBeast2[] = {
        {csSetup, 0, clCyan},  {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kBlinkDark},
        {csRepeat, 1},
        {csWait, kPauseNextLsq},
        {csEnd},
};
const LedRGBChunk lsqCyanBeast3[] = {
        {csSetup, 0, clCyan},  {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kBlinkDark},
        {csRepeat, 2},
        {csWait, kPauseNextLsq},
        {csEnd},
};

const LedRGBChunk lsqSearcher1[] = {
        {csSetup, 0, clYellow}, {csWait, kBlinkDuration},
        {csSetup, 0, clBlack},  {csWait, kPauseNextLsq},
        {csEnd},
};
const LedRGBChunk lsqSearcher2[] = {
        {csSetup, 0, clYellow}, {csWait, kBlinkDuration},
        {csSetup, 0, clBlack},  {csWait, kBlinkDark},
        {csRepeat, 1},
        {csWait, kPauseNextLsq},
        {csEnd},
};
const LedRGBChunk lsqSearcher3[] = {
        {csSetup, 0, clYellow}, {csWait, kBlinkDuration},
        {csSetup, 0, clBlack},  {csWait, kBlinkDark},
        {csRepeat, 2},
        {csWait, kPauseNextLsq},
        {csEnd},
};
#pragma endregion

#pragma region // ==== Pill indication ====
inline constexpr const unsigned long kPillBlinkDur = 630, kAfterPillDelay = 2007;
const LedRGBChunk lsqPillBad[] = {
        {csSetup, 0, clRed},   {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kBlinkDark},
        {csRepeat, 2},
        {csWait, kAfterPillDelay},
        {csEnd}
};

const LedRGBChunk lsqPillReset[] = {
        {csSetup, 0, clWhite}, {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kBlinkDark},
        {csRepeat, 2},
        {csWait, kAfterPillDelay},
        {csEnd}
};

const LedRGBChunk lsqPillGoodnessPlus[] = {
        {csSetup, 0, clBlue},  {csWait, kPillBlinkDur},
        {csSetup, 0, clBlack},
        {csWait, kAfterPillDelay},
        {csEnd}
};

const LedRGBChunk lsqPillGoodnessMinus[] = {
        {csSetup, 0, clRed},   {csWait, kPillBlinkDur},
        {csSetup, 0, clBlack},
        {csWait, kAfterPillDelay},
        {csEnd}
};

const LedRGBChunk lsqPillFixForever[] = {
        {csSetup, 0, clMagenta}, {csWait, kPillBlinkDur},
        {csSetup, 0, clBlack},
        {csWait, kAfterPillDelay},
        {csEnd}
};

const LedRGBChunk lsqPillFixTimed[] = {
        {csSetup, 0, clMagenta}, {csWait, kBlinkDuration},
        {csSetup, 0, clBlack},   {csWait, kBlinkDark},
        {csRepeat, 1},
        {csWait, kAfterPillDelay},
        {csEnd}
};

const LedRGBChunk lsqPillDisableFix[] = {
        {csSetup, 0, clMagenta}, {csWait, kBlinkDuration},
        {csSetup, 0, clBlack},   {csWait, kBlinkDark},
        {csSetup, 0, clRed},     {csWait, kBlinkDuration},
        {csSetup, 0, clBlack},
        {csWait, kAfterPillDelay},
        {csEnd}
};
#pragma endregion

#pragma region // === Goodness ===
inline constexpr const unsigned long kGBrt = 45;
const LedRGBChunk lsqGoodnessBlue[] = {
        {csSetup, 0, {0,0,kGBrt}}, {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kPauseNextLsq},
        {csEnd},
};
const LedRGBChunk lsqGoodnessYellow[] = {
        {csSetup, 0, {kGBrt,kGBrt,0}}, {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kPauseNextLsq},
        {csEnd},
};
const LedRGBChunk lsqGoodnessRed[] = {
        {csSetup, 0, {kGBrt,0,0}}, {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kPauseNextLsq},
        {csEnd},
};

const LedRGBChunk lsqGoodnessBlueFrozen[] = {
        {csSetup, 0, {0,0,kGBrt}}, {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kBlinkDark},
        {csRepeat, 1},
        {csWait, kPauseNextLsq},
        {csEnd},
};
const LedRGBChunk lsqGoodnessYellowFrozen[] = {
        {csSetup, 0, {kGBrt,kGBrt,0}}, {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kBlinkDark},
        {csRepeat, 1},
        {csWait, kPauseNextLsq},
        {csEnd},
};
const LedRGBChunk lsqGoodnessRedFrozen[] = {
        {csSetup, 0, {kGBrt,0,0}}, {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kBlinkDark},
        {csRepeat, 1},
        {csWait, kPauseNextLsq},
        {csEnd},
};
#pragma endregion

#pragma region // === Magic ===
const LedRGBChunk lsqMagicGreenEvil[] = {
        {csSetup, 0, clGreen}, {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kPauseNextLsq},
        {csEnd},
};
const LedRGBChunk lsqMagicArtifact[] = {
        {csSetup, 0, clMagenta}, {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kPauseNextLsq},
        {csEnd},
};
const LedRGBChunk lsqMagicBeast[] = {
        {csSetup, 0, clCyan}, {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kPauseNextLsq},
        {csEnd},
};
#pragma endregion

#pragma region // === TX Power ===
const LedRGBChunk lsqTxPwrM15[] = {
        {csSetup, 0, clRed},   {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kBlinkDuration},
        {csRepeat, 3},
        {csSetup, 0, clBlack}, {csWait, kPauseNextLsq},
        {csEnd},
};
const LedRGBChunk lsqTxPwrM10[] = {
        {csSetup, 0, clRed},   {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kBlinkDuration},
        {csRepeat, 2},
        {csSetup, 0, clBlack}, {csWait, kPauseNextLsq},
        {csEnd},
};
const LedRGBChunk lsqTxPwrM6[] = {
        {csSetup, 0, clRed},   {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kBlinkDuration},
        {csRepeat, 1},
        {csSetup, 0, clBlack}, {csWait, kPauseNextLsq},
        {csEnd},
};

const LedRGBChunk lsqTxPwr0[] = {
        {csSetup, 0, clRed},   {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kBlinkDuration},
        {csSetup, 0, clGreen}, {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kPauseNextLsq},
        {csEnd},
};

const LedRGBChunk lsqTxPwrP5[] = {
        {csSetup, 0, clGreen}, {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kBlinkDuration},
        {csRepeat, 1},
        {csSetup, 0, clBlack}, {csWait, kPauseNextLsq},
        {csEnd},
};
const LedRGBChunk lsqTxPwrP7[] = {
        {csSetup, 0, clGreen}, {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kBlinkDuration},
        {csRepeat, 2},
        {csSetup, 0, clBlack}, {csWait, kPauseNextLsq},
        {csEnd},
};
const LedRGBChunk lsqTxPwrP10[] = {
        {csSetup, 0, clGreen}, {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kBlinkDuration},
        {csRepeat, 3},
        {csSetup, 0, clBlack}, {csWait, kPauseNextLsq},
        {csEnd},
};
const LedRGBChunk lsqTxPwrP12[] = {
        {csSetup, 0, clGreen}, {csWait, kBlinkDuration},
        {csSetup, 0, clBlack}, {csWait, kBlinkDuration},
        {csRepeat, 4},
        {csSetup, 0, clBlack}, {csWait, kPauseNextLsq},
        {csEnd},
};
#pragma endregion

const LedRGBChunk lsqStart[] = {
        {csSetup, 0, clRed},   {csWait, 450},
        {csSetup, 0, clGreen}, {csWait, 450},
        {csSetup, 0, clBlue},  {csWait, 450},
//        {csSetup, 0, clBlack},
        {csSetup, 0, {0,1,0}},
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

#if 1 // ============================= Beeper ==================================
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
#define VIBRO_LONG_MS           450
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
#endif

#endif //SEQUENCES_H__
