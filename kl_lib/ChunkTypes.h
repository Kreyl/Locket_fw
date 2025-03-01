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
#include "kl_buf.h"

enum ChunkSort_t {csSetup, csWait, csGoto, csEnd, csRepeat};

// The beginning of any sort of chunk. Everyone must contain it.
#define BaseChunk_Vars \
    ChunkSort_t ChunkSort;          \
    union {                         \
        uint32_t Value;             \
        uint32_t volume;            \
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
    uint16_t freq_Hz;
} __attribute__((packed));


#if 1 // ====================== Base sequencer class ===========================
enum SequencerLoopTask_t {sltProceed, sltBreak};

template <class TChunk, uint32_t que_len>
class BaseSequencer_t : private IrqHandler_t {
private:
    CircBuf_t<const TChunk*, que_len> seq_que;
protected:
    virtual_timer_t itmr;
    const TChunk *start_chunk = nullptr, *curr_chunk = nullptr;
    int32_t repeat_cntr = -1;
    EvtMsg_t on_end_evt_msg;
    virtual void ISwitchOff() = 0;
    virtual SequencerLoopTask_t ISetup() = 0;
    void SetupDelay(uint32_t ms) { chVTSetI(&itmr, TIME_MS2I(ms), TmrKLCallback, this); }

    // Process sequence
    void IIrqHandler() {
        if(chVTIsArmedI(&itmr)) chVTResetI(&itmr);  // Reset timer
        while(true) {   // Process the sequence
            switch(curr_chunk->ChunkSort) {
                case csSetup: // setup now and exit if required
                    if(ISetup() == sltBreak) return;
                    break;

                case csWait: { // Start timer, pointing to next chunk
                        uint32_t Delay = curr_chunk->Time_ms;
                        curr_chunk++;
                        if(Delay != 0) {
                            SetupDelay(Delay);
                            return;
                        }
                    }
                    break;

                case csRepeat:
                    if(repeat_cntr == -1) repeat_cntr = curr_chunk->RepeatCnt;
                    if(repeat_cntr == 0) {    // All was repeated, goto next
                        repeat_cntr = -1;     // reset counter
                        curr_chunk++;
                    }
                    else {  // repeating in progress
                        curr_chunk = start_chunk;  // Always from beginning
                        repeat_cntr--;
                    }
                    break;

                case csGoto:
                    curr_chunk = start_chunk + curr_chunk->ChunkToJumpTo;
                    if(on_end_evt_msg.id != EvtId::None) evt_q_main.SendNowOrExitI(on_end_evt_msg);
                    SetupDelay(1);
                    return;
                    break;

                case csEnd:
                    if(on_end_evt_msg.id != EvtId::None) evt_q_main.SendNowOrExitI(on_end_evt_msg);
                    if(seq_que.GetI(&start_chunk) == retv::Ok) { // There is something next
                        curr_chunk = start_chunk;
                        repeat_cntr = -1;
                    }
                    else { // There is nothing next
                        start_chunk = nullptr;
                        curr_chunk = nullptr;
                        return;
                    }
                    break;
            } // switch
        } // while
    } // IProcessSequenceI
public:
    BaseSequencer_t() {}
    void SetupSeqEndEvt(EvtMsg_t AEvtMsg) { on_end_evt_msg = AEvtMsg; }

    void StartOrRestartI(const TChunk *pchunk) {
        repeat_cntr = -1;
        start_chunk = pchunk;   // Save first chunk
        curr_chunk = pchunk;
        seq_que.Flush();
        IIrqHandler();
    }

    void StartOrRestart(const TChunk *pchunk) {
        chSysLock();
        StartOrRestartI(pchunk);
        chSysUnlock();
    }

    void StartOrContinue(const TChunk *PChunk) {
        if(PChunk == start_chunk) return; // Same sequence
        else StartOrRestart(PChunk);
    }

    void StartIfIdle(const TChunk *PChunk) {
        if(IsIdle()) StartOrRestart(PChunk);
    }

    void Stop() {
        if(start_chunk != nullptr) {
            chSysLock();
            if(chVTIsArmedI(&itmr)) chVTResetI(&itmr);
            start_chunk = nullptr;
            curr_chunk = nullptr;
            seq_que.Flush();
            chSysUnlock();
        }
        ISwitchOff();
    }
    const TChunk* GetCurrentSequence() { return start_chunk; }

    // Next sequence will be started after current ends
    retv StartOrAddToQueueI(const TChunk *pchunk) {
        if(IsIdle() and pchunk != nullptr) {
            StartOrRestartI(pchunk);
            return retv::Ok;
        }
        else return seq_que.PutIfNotOverflow(pchunk);
    }
    void StartOrAddToQueue(const TChunk *pchunk) {
        chSysLock();
        StartOrAddToQueueI(pchunk);
        chSysUnlock();
    }

    bool IsIdle() { return (start_chunk == nullptr); }
};
#endif

#endif //CHUNKTYPES_H__
