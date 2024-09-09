/*
 * vibro.h
 *
 *  Created on: 26-04-2015 �.
 *      Author: Kreyl
 */

#ifndef VIBRO_H__
#define VIBRO_H__

#include "kl_lib.h"
#include "ChunkTypes.h"
#include "board.h"

template <uint32_t que_len = 0>
class Vibro_t : public BaseSequencer_t<BaseChunk_t, que_len> {
private:
    const PinOutputPWM_t IPin;
    void ISwitchOff() { IPin.Set(0); }
    SequencerLoopTask_t ISetup() {
        IPin.Set(this->curr_chunk->Volume);
        this->curr_chunk++;   // Always goto next
        return sltProceed;  // Always proceed
    }
public:
    Vibro_t(PwmSetup_t APin) : BaseSequencer_t<BaseChunk_t, que_len>(), IPin(APin) {}
    void Init() { IPin.Init(); }
};


#endif //VIBRO_H__
