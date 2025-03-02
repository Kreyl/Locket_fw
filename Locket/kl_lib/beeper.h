/*
 * beeper.h
 *
 *  Created on: 2015
 *      Author: Kreyl
 */

#ifndef BEEPER_H__
#define BEEPER_H__

#include "ChunkTypes.h"
#include "kl_lib.h"

template <uint32_t que_len = 0>
class Beeper_t : public BaseSequencer_t<BeepChunk_t, que_len> {
private:
    const PinOutputPWM_t ipin;
    void ISwitchOff() { ipin.Set(0); }
    SequencerLoopTask_t ISetup() {
        if(this->curr_chunk->freq_Hz > 0) ipin.SetFrequencyHz(this->curr_chunk->freq_Hz);
        ipin.Set(this->curr_chunk->volume);
        this->curr_chunk++; // Always goto next
        return sltProceed;  // Always proceed
    }
public:
    Beeper_t(const PwmSetup_t apin) : BaseSequencer_t<BeepChunk_t, que_len>(), ipin(apin) {}
    void Init() { ipin.Init(); }
    void Beep(uint32_t freq_Hz, uint8_t volume) {
        ipin.SetFrequencyHz(freq_Hz);
        ipin.Set(volume);
    }
    void Off() { ipin.Set(0); }
};

#endif //BEEPER_H__