/*
 * ChunkTypes.h
 *
 *  Created on: 08 ���. 2015 �.
 *      Author: Kreyl
 */

#ifndef CHUNKTYPES_H__
#define CHUNKTYPES_H__

#include "color.h"
#include "ch.h"
#include "MsgQ.h"

enum ChunkSort_t {csSetup, csWait, csGoto, csEnd, csRepeat};

// The beginning of any sort of chunk. Everyone must contain it.
#define BaseChunk_Vars \
    ChunkSort_t ChunkSort;          \
    union {                         \
        uint32_t Value;             \
        uint32_t Volume;            \
        uint32_t Time_ms;           \
        uint32_t ChunkToJumpTo;     \
        int32_t RepeatCnt;          \
    }

// ==== Different types of chunks ====
struct BaseChunk_t {
    BaseChunk_Vars;
};

// RGB LED chunk
struct LedRGBChunk_t {
    BaseChunk_Vars;
    Color_t Color;
    LedRGBChunk_t(ChunkSort_t ASort, uint32_t AValue, Color_t AColor) : ChunkSort(ASort), Value(AValue), Color(AColor) {}
    LedRGBChunk_t(ChunkSort_t ASort, uint32_t AValue) : ChunkSort(ASort), Value(AValue), Color(0,0,0) {}
    LedRGBChunk_t(ChunkSort_t ASort) : ChunkSort(ASort), Value(0), Color(0,0,0) {}
} __attribute__((packed));

// HSV LED chunk
struct LedHSVChunk_t {
    BaseChunk_Vars;
    ColorHSV_t Color;
} __attribute__((packed));

// LED Smooth
struct LedSmoothChunk_t {
    BaseChunk_Vars;
    uint8_t Brightness;
} __attribute__((packed));

// Beeper
struct BeepChunk_t {   // Value == Volume
    BaseChunk_Vars;
    uint16_t Freq_Hz;
} __attribute__((packed));


#if 1 // ====================== Base sequencer class ===========================
enum SequencerLoopTask_t {sltProceed, sltBreak};

template <class TChunk>
class BaseSequencer_t : private IrqHandler_t {
protected:
    virtual_timer_t ITmr;
    const TChunk *IPStartChunk, *IPCurrentChunk, *INextChunk = nullptr;
    int32_t RepeatCounter = -1;
    EvtMsg_t IEvtMsg;
    virtual void ISwitchOff() = 0;
    virtual SequencerLoopTask_t ISetup() = 0;
    void SetupDelay(uint32_t ms) { chVTSetI(&ITmr, TIME_MS2I(ms), TmrKLCallback, this); }

    // Process sequence
    void IIrqHandler() {
        if(chVTIsArmedI(&ITmr)) chVTResetI(&ITmr);  // Reset timer
        while(true) {   // Process the sequence
            switch(IPCurrentChunk->ChunkSort) {
                case csSetup: // setup now and exit if required
                    if(ISetup() == sltBreak) return;
                    break;

                case csWait: { // Start timer, pointing to next chunk
                        uint32_t Delay = IPCurrentChunk->Time_ms;
                        IPCurrentChunk++;
                        if(Delay != 0) {
                            SetupDelay(Delay);
                            return;
                        }
                    }
                    break;

                case csRepeat:
                    if(RepeatCounter == -1) RepeatCounter = IPCurrentChunk->RepeatCnt;
                    if(RepeatCounter == 0) {    // All was repeated, goto next
                        RepeatCounter = -1;     // reset counter
                        IPCurrentChunk++;
                    }
                    else {  // repeating in progress
                        IPCurrentChunk = IPStartChunk;  // Always from beginning
                        RepeatCounter--;
                    }
                    break;

                case csGoto:
                    IPCurrentChunk = IPStartChunk + IPCurrentChunk->ChunkToJumpTo;
                    if(IEvtMsg.id != EvtId::None) EvtQMain.SendNowOrExitI(IEvtMsg);
                    SetupDelay(1);
                    return;
                    break;

                case csEnd:
                    if(IEvtMsg.id != EvtId::None) EvtQMain.SendNowOrExitI(IEvtMsg);
                    if(INextChunk == nullptr) { // There is nothing next
                        IPStartChunk = nullptr;
                        IPCurrentChunk = nullptr;
                        return;
                    }
                    else { // There is something next
                        RepeatCounter = -1;
                        IPStartChunk = INextChunk;
                        IPCurrentChunk = INextChunk;
                        INextChunk = nullptr;
                    }
                    break;
            } // switch
        } // while
    } // IProcessSequenceI
public:
    void SetupSeqEndEvt(EvtMsg_t AEvtMsg) { IEvtMsg = AEvtMsg; }

    void StartOrRestartI(const TChunk *pchunk) {
        RepeatCounter = -1;
        IPStartChunk = pchunk;   // Save first chunk
        IPCurrentChunk = pchunk;
        INextChunk = nullptr;
        IIrqHandler();
    }

    void StartOrRestart(const TChunk *pchunk) {
        chSysLock();
        StartOrRestartI(pchunk);
        chSysUnlock();
    }

    void StartOrContinue(const TChunk *PChunk) {
        if(PChunk == IPStartChunk) return; // Same sequence
        else StartOrRestart(PChunk);
    }

    void StartIfIdle(const TChunk *PChunk) {
        if(IsIdle()) StartOrRestart(PChunk);
    }

    void Stop() {
        if(IPStartChunk != nullptr) {
            chSysLock();
            if(chVTIsArmedI(&ITmr)) chVTResetI(&ITmr);
            IPStartChunk = nullptr;
            IPCurrentChunk = nullptr;
            INextChunk = nullptr;
            chSysUnlock();
        }
        ISwitchOff();
    }
    const TChunk* GetCurrentSequence() { return IPStartChunk; }

    // Next sequence will be started after current ends
    void SetNextSequenceI(const TChunk *pchunk) {
        if(IsIdle() and pchunk != nullptr) StartOrRestartI(pchunk);
        else INextChunk = pchunk;
    }
    void SetNextSequence(const TChunk *pchunk) {
        chSysLock();
        SetNextSequenceI(pchunk);
        chSysUnlock();
    }

    bool IsIdle() { return (IPStartChunk == nullptr and IPCurrentChunk == nullptr); }
};
#endif

#endif //CHUNKTYPES_H__
