/*
 * led_rgb.h
 *
 *  Created on: 31 ���. 2014 �.
 *      Author: Kreyl
 */

#ifndef LED_H__
#define LED_H__

#include "hal.h"
#include "color.h"
#include "ChunkTypes.h"
#include "uart.h"
#include "kl_lib.h"

#if 1 // ==================== LED on/off, no sequences =========================
class LedOnOff_t {
protected:
    PinOutput_t IChnl;
public:
    LedOnOff_t(GPIO_TypeDef *APGPIO, uint16_t APin, PinOutMode_t AOutputType) :
        IChnl(APGPIO, APin, AOutputType) {}
    void Init() { IChnl.Init(); Off(); }
    void On()  { IChnl.SetHi(); }
    void Off() { IChnl.SetLo(); }
};
#endif

#if 1 // ========================= Simple LED blinker ==========================
template <uint32_t que_len = 0>
class LedBlinker_t : public BaseSequencer_t<BaseChunk_t, que_len>, public LedOnOff_t {
protected:
    void ISwitchOff() { Off(); }
    SequencerLoopTask_t ISetup() {
        IChnl.Set(this->curr_chunk->Value);
        this->curr_chunk++; // Always increase
        return sltProceed;  // Always proceed
    }
public:
    LedBlinker_t(GPIO_TypeDef *APGPIO, uint16_t APin, PinOutMode_t AOutputType) :
        BaseSequencer_t<BaseChunk_t, que_len>(), LedOnOff_t(APGPIO, APin, AOutputType) {}
};
#endif

#if 1 // ======================== Single Led Smooth ============================
template <uint32_t que_len = 0>
class LedSmooth_t : public BaseSequencer_t<LedSmoothChunk_t, que_len> {
private:
    const PinOutputPWM_t IChnl;
    uint8_t ICurrentBrightness;
    const uint32_t PWMFreq;
    void ISwitchOff() { SetBrightness(0); }
    SequencerLoopTask_t ISetup() {
        if(ICurrentBrightness != this->curr_chunk->Brightness) {
            if(this->curr_chunk->Value == 0) {     // If smooth time is zero,
                SetBrightness(this->curr_chunk->Brightness); // set color now,
                ICurrentBrightness = this->curr_chunk->Brightness;
                this->curr_chunk++;                // and goto next chunk
            }
            else {
                if     (ICurrentBrightness < this->curr_chunk->Brightness) ICurrentBrightness++;
                else if(ICurrentBrightness > this->curr_chunk->Brightness) ICurrentBrightness--;
                SetBrightness(ICurrentBrightness);
                // Check if completed now
                if(ICurrentBrightness == this->curr_chunk->Brightness) this->curr_chunk++;
                else { // Not completed
                    // Calculate time to next adjustment
                    uint32_t Delay = ClrCalcDelay(ICurrentBrightness, this->curr_chunk->Value);
                    this->SetupDelay(Delay);
                    return sltBreak;
                } // Not completed
            } // if time > 256
        } // if color is different
        else this->curr_chunk++; // Color is the same, goto next chunk
        return sltProceed;
    }
public:
    LedSmooth_t(const PwmSetup_t APinSetup, const uint32_t AFreq = 0xFFFFFFFF) :
        BaseSequencer_t<BaseChunk_t, que_len>(), IChnl(APinSetup), ICurrentBrightness(0), PWMFreq(AFreq) {}
    void Init() {
        IChnl.Init();
        IChnl.SetFrequencyHz(PWMFreq);
        SetBrightness(0);
    }
    void SetBrightness(uint8_t ABrightness) { IChnl.Set(ABrightness); }
};
#endif


#if 0 // ==================== RGB blinker (no smooth switch) ===================
#define LED_RGB_BLINKER
class LedRgbBlinker_t : public BaseSequencer_t<LedRGBChunk_t> {
protected:
    PinOutputPushPull_t R, G, B;
    void ISwitchOff() { SetColor(clBlack); }
    SequencerLoopTask_t ISetup() {
        SetColor(curr_chunk->Color);
        curr_chunk++;   // Always increase
        return sltProceed;  // Always proceed
    }
public:
    LedRgbBlinker_t(const PinOutputPushPull_t ARed, const PinOutputPushPull_t AGreen, const PinOutputPushPull_t ABlue) :
        BaseSequencer_t(), R(ARed), G(AGreen), B(ABlue) {}
    void Init() {
        R.Init();
        G.Init();
        B.Init();
        SetColor(clBlack);
    }
    void SetColor(Color_t AColor) {
        R.Set(AColor.R);
        G.Set(AColor.G);
        B.Set(AColor.B);
    }
};
#endif

#if 1 // =========================== LedRGB Parent =============================
template <uint32_t que_len = 0>
class LedRGBParent_t : public BaseSequencer_t<LedRGBChunk_t, que_len> {
protected:
    const PinOutputPWM_t  R, G, B;
    const uint32_t PWMFreq;
    Color_t ICurrColor;
    void ISwitchOff() {
        SetColor(clBlack);
        ICurrColor = clBlack;
    }
    SequencerLoopTask_t ISetup() {
        if(ICurrColor != this->curr_chunk->Color) {
            if(this->curr_chunk->Value == 0) {     // If smooth time is zero,
                SetColor(this->curr_chunk->Color); // set color now,
                ICurrColor = this->curr_chunk->Color;
                this->curr_chunk++;                // and goto next chunk
            }
            else {
                ICurrColor.Adjust(this->curr_chunk->Color);
                SetColor(ICurrColor);
                // Check if completed now
                if(ICurrColor == this->curr_chunk->Color) this->curr_chunk++;
                else { // Not completed
                    // Calculate time to next adjustment
                    uint32_t Delay = ICurrColor.DelayToNextAdj(this->curr_chunk->Color, this->curr_chunk->Value);
                    this->SetupDelay(Delay);
                    return sltBreak;
                } // Not completed
            } // if time > 256
        } // if color is different
        else this->curr_chunk++; // Color is the same, goto next chunk
        return sltProceed;
    }
public:
    LedRGBParent_t(
            const PwmSetup_t ARed,
            const PwmSetup_t AGreen,
            const PwmSetup_t ABlue,
            const uint32_t APWMFreq) :
        BaseSequencer_t<LedRGBChunk_t, que_len>(), R(ARed), G(AGreen), B(ABlue), PWMFreq(APWMFreq) {}
    void Init() {
        R.Init();
        R.SetFrequencyHz(PWMFreq);
        G.Init();
        G.SetFrequencyHz(PWMFreq);
        B.Init();
        B.SetFrequencyHz(PWMFreq);
        SetColor(clBlack);
    }
    bool IsOff() { return (ICurrColor == clBlack) and this->IsIdle(); }
    virtual void SetColor(Color_t AColor) {}
};
#endif

#if 1 // ============================== LedRGB =================================
template <uint32_t que_len = 0>
class LedRGB_t : public LedRGBParent_t<que_len> {
public:
    LedRGB_t(
            const PwmSetup_t ARed,
            const PwmSetup_t AGreen,
            const PwmSetup_t ABlue,
            const uint32_t AFreq = 0xFFFFFFFF) :
                LedRGBParent_t<que_len>(ARed, AGreen, ABlue, AFreq) {}

    void SetColor(Color_t AColor) {
        this->R.Set(AColor.R);
        this->G.Set(AColor.G);
        this->B.Set(AColor.B);
    }
};
#endif

#if 1 // =========================== RGB LED with power ========================
template <uint32_t que_len = 0>
class LedRGBwPower_t : public LedRGBParent_t<que_len> {
private:
    const PinOutput_t PwrPin;
public:
    LedRGBwPower_t(
            const PwmSetup_t ARed,
            const PwmSetup_t AGreen,
            const PwmSetup_t ABlue,
            const PinOutput_t APwrPin,
            const uint32_t AFreq = 0xFFFFFFFF) :
                LedRGBParent_t<que_len>(ARed, AGreen, ABlue, AFreq), PwrPin(APwrPin) {}
    void Init() {
        PwrPin.Init();
        LedRGBParent_t<que_len>::Init();
    }
    void SetColor(Color_t AColor) {
        if(AColor == clBlack) PwrPin.SetLo();
        else PwrPin.SetHi();
        this->R.Set(AColor.R);
        this->G.Set(AColor.G);
        this->B.Set(AColor.B);
    }
};
#endif

#if 1 // ====================== LedRGB with Luminocity =========================
template <uint32_t que_len = 0>
class LedRGBLum_t : public LedRGBParent_t<que_len> {
public:
    LedRGBLum_t(
            const PwmSetup_t ARed,
            const PwmSetup_t AGreen,
            const PwmSetup_t ABlue,
            const uint32_t AFreq = 0xFFFFFFFF) :
                LedRGBParent_t<que_len>(ARed, AGreen, ABlue, AFreq) {}

    void SetColor(Color_t AColor) {
        this->R.Set(AColor.R * AColor.Brt);
        this->G.Set(AColor.G * AColor.Brt);
        this->B.Set(AColor.B * AColor.Brt);
    }
};
#endif

#if 1 // ============================ LedHSV ===================================
template <uint32_t que_len = 0>
class LedHSV_t : public BaseSequencer_t<LedHSVChunk_t, que_len> {
protected:
    const PinOutputPWM_t  R, G, B;
    const uint32_t PWMFreq;
    ColorHSV_t ICurrColor;
    void ISwitchOff() {
        SetColor(clBlack);
        ICurrColor.V = 0;
    }
    SequencerLoopTask_t ISetup() {
        if(ICurrColor != this->curr_chunk->Color) {
            if(this->curr_chunk->Value == 0) {     // If smooth time is zero,
                SetColor(this->curr_chunk->Color); // set color now,
                ICurrColor = this->curr_chunk->Color;
                this->curr_chunk++;                // and goto next chunk
            }
            else {
                ICurrColor.Adjust(this->curr_chunk->Color);
                SetColor(ICurrColor);
                // Check if completed now
                if(ICurrColor == this->curr_chunk->Color) this->curr_chunk++;
                else { // Not completed
                    // Calculate time to next adjustment
                    uint32_t Delay = ICurrColor.DelayToNextAdj(this->curr_chunk->Color, this->curr_chunk->Value);
                    this->SetupDelay(Delay);
                    return sltBreak;
                } // Not completed
            } // if time > 256
        } // if color is different
        else this->curr_chunk++; // Color is the same, goto next chunk
        return sltProceed;
    }
public:
    LedHSV_t(
            const PwmSetup_t ARed,
            const PwmSetup_t AGreen,
            const PwmSetup_t ABlue,
            const uint32_t APWMFreq = 0xFFFFFFFF) :
        BaseSequencer_t<LedHSVChunk_t, que_len>(), R(ARed), G(AGreen), B(ABlue), PWMFreq(APWMFreq) {}
    void Init() {
        R.Init();
        R.SetFrequencyHz(PWMFreq);
        G.Init();
        G.SetFrequencyHz(PWMFreq);
        B.Init();
        B.SetFrequencyHz(PWMFreq);
        SetColor(clBlack);
    }
    bool IsOff() { return (ICurrColor == hsvBlack) and this->IsIdle(); }
    void SetColor(Color_t ColorRgb) {
        R.Set(ColorRgb.R);
        G.Set(ColorRgb.G);
        B.Set(ColorRgb.B);
    }
    void SetColor(ColorHSV_t ColorHsv) {
        SetColor(ColorHsv.ToRGB());
    }
    void SetColorAndMakeCurrent(ColorHSV_t ColorHsv) {
        SetColor(ColorHsv.ToRGB());
        ICurrColor = ColorHsv;
    }
    void SetCurrentH(uint16_t NewH) {
        ICurrColor.H = NewH;
        SetColor(ICurrColor);
    }
};
#endif

#endif //LED_H__
