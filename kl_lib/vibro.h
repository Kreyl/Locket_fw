/*
 * vibro.h
 *
 *  Created on: 26-04-2015 ã.
 *      Author: Kreyl
 */

#ifndef KL_LIB_VIBRO_H_
#define KL_LIB_VIBRO_H_

#include <kl_lib.h>
#include "ChunkTypes.h"
#include "hal.h"

#define VIBRO_TOP_VALUE     22
#define VIBRO_FREQ_HZ       3600

class Vibro_t : public BaseSequencer_t<BaseChunk_t> {
private:
    PinOutputPWM_t<VIBRO_TOP_VALUE, invNotInverted, omPushPull> IPin;
    void ISwitchOff() { IPin.Set(0); }
    SequencerLoopTask_t ISetup() {
        IPin.Set(IPCurrentChunk->Volume);
        IPCurrentChunk++;   // Always goto next
        return sltProceed;  // Always proceed
    }
public:
    Vibro_t(GPIO_TypeDef *APGpio, uint16_t APin, TIM_TypeDef *APTimer, uint32_t ATmrChnl) :
        BaseSequencer_t(), IPin(APGpio, APin, APTimer, ATmrChnl) {}
    void Init() {
        IPin.Init();
        IPin.SetFrequencyHz(VIBRO_FREQ_HZ);
    }
};


#endif /* KL_LIB_VIBRO_H_ */
