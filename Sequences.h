#pragma once

#include "ChunkTypes.h"
const LedRGBChunk_t lsqFailure[] = {
        {csSetup, 0, clRed},
        {csWait, 99},
        {csSetup, 0, clBlack},
        {csWait, 99},
        {csRepeat, 5},
        {csEnd}
};

const LedRGBChunk_t lsqRxGrp1[] = {
        {csSetup, 360, {0, 255, 0}},
        {csSetup, 360, {0, 4, 0}},
        {csEnd},
};
const LedRGBChunk_t lsqTxGrp1[] = {
        {csSetup, 360, {255, 0, 0}},
        {csSetup, 360, {4, 0, 0}},
        {csEnd},
};

const LedRGBChunk_t lsqRxGrp2[] = {
        {csSetup, 360, {255, 255, 0}},
        {csSetup, 360, {4, 4, 0}},
        {csEnd},
};
const LedRGBChunk_t lsqTxGrp2[] = {
        {csSetup, 360, {255, 0, 255}},
        {csSetup, 360, {4, 0, 4}},
        {csEnd},
};

const LedRGBChunk_t lsqFeelEachOther[] = {
        {csSetup, 360, {0, 255, 255}},
        {csSetup, 360, {0, 4, 4}},
        {csEnd},
};

#if 1 // ============================== Vibro ==================================
#define VIBRO_VOLUME    100  // 1 to 100

#define VIBRO_SHORT_MS          99
#define VIBRO_REPEAT_PERIOD     1008

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
        {csRepeat, 1},
        {csWait, VIBRO_REPEAT_PERIOD},
        {csEnd}
};

const BaseChunk_t vsqBrrBrrBrr[] = {
        {csSetup, VIBRO_VOLUME},
        {csWait, VIBRO_SHORT_MS},
        {csSetup, 0},
        {csWait, 99},
        {csRepeat, 2},
        {csWait, VIBRO_REPEAT_PERIOD},
        {csEnd}
};
#endif
