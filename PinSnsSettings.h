/* ================ Documentation =================
 * There are several (may be 1) groups of sensors (say, buttons and USB connection).
 *
 */

#pragma once

#include "SimpleSensors.h"

#ifndef SIMPLESENSORS_ENABLED
#define SIMPLESENSORS_ENABLED   FALSE
#endif

#if SIMPLESENSORS_ENABLED
#define SNS_POLL_PERIOD_MS      72

// Button handler
extern void ProcessButtons(PinSnsState_t *PState, uint32_t Len);

const PinSns_t PinSns[] = {
        // Buttons
        {BTN1_PIN, ProcessButtons},
        {BTN2_PIN, ProcessButtons},
        {BTN3_PIN, ProcessButtons},
};
#define PIN_SNS_CNT     countof(PinSns)

#endif  // if enabled
