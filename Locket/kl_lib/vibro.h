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
    const PinOutputPWM_t ipin;
    void ISwitchOff() { ipin.Set(0); }
    SequencerLoopTask_t ISetup() {
        ipin.Set(this->curr_chunk->volume);
        this->curr_chunk++;   // Always goto next
        return sltProceed;  // Always proceed
    }
public:
    Vibro_t(PwmSetup_t apin) : BaseSequencer_t<BaseChunk_t, que_len>(), ipin(apin) {}
    void Init() { ipin.Init(); }
};


#endif //VIBRO_H__
