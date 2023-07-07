/*
 * vibro.h
 *
 *  Created on: 26-04-2015 ã.
 *      Author: Kreyl
 */

#ifndef VIBRO_H__
#define VIBRO_H__

#include "kl_lib.h"
#include "ChunkTypes.h"
#include "board.h"

class Vibro_t : public BaseSequencer_t<BaseChunk_t> {
private:
    const PinOutputPWM_t IPin;
    void ISwitchOff() { IPin.Set(0); }
    SequencerLoopTask_t ISetup() {
        IPin.Set(IPCurrentChunk->Volume);
        IPCurrentChunk++;   // Always goto next
        return sltProceed;  // Always proceed
    }
public:
    Vibro_t(PwmSetup_t APin) : BaseSequencer_t(), IPin(APin) {}
    void Init() { IPin.Init(); }
};

#endif //VIBRO_H__