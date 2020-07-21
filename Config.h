/*
 * Config.h
 *
 *  Created on: Jul 4, 2020
 *      Author: layst
 */

#pragma once

#include <vector>
#include "ChunkTypes.h"
#include <string>
#include "shell.h"

#define ID_MIN                  1
#define ID_MAX                  54
#define ID_DEFAULT              ID_MIN

enum VibroType_t { vbrNone = 0, vbrBrr, vbrBrrBrr, vbrBrrBrrBrr };
#define VIBRO_TYPE_BRR          "Brr"
#define VIBRO_TYPE_BRRBRR       "BrrBrr"
#define VIBRO_TYPE_BRRBRRBRR    "BrrBrrBrr"

using LedSeq_t = std::vector<LedRGBChunk_t>;

// How to react
struct Reaction_t {
    std::string Name;
    LedSeq_t Seq;
    VibroType_t VibroType = vbrNone;
    bool MustStopTx = false;
    void Print() {
        Printf("Name:%S; Vibro:%u; StopTX:%u; {", Name.c_str(), VibroType, MustStopTx);
        for(auto &Item : Seq) {
            switch(Item.ChunkSort) {
                case csSetup:  Printf("{Setup {%u %u %u}}", Item.Color.R, Item.Color.G, Item.Color.B); break;
                case csWait:   Printf("{Wait %u}", Item.Time_ms); break;
                case csGoto:   Printf("{Goto %u}", Item.Value); break;
                case csRepeat: Printf("{Repeat %u}", Item.RepeatCnt); break;
                case csEnd:    Printf("{End}"); break;
            }
        }
        Printf("}\r");
    }
};

// ==== Locket settings ====
struct RcvItem_t {
    uint32_t Src;      // Type to react to
    Reaction_t* React; // What to do
    int32_t Distance;  // Min dist to react
    void Print() {
        Printf("\r{t=%u; r=%S; d=%u}", Src, (React? React->Name.c_str() : ""), Distance);
//        Printf("{%S; d=%u}", SrcStr.c_str(), Distance);
    }
};

struct LocketInfo_t {
    std::string Name;
    uint8_t Type;
    Reaction_t* ReactOnBtns[BTN_CNT] = {nullptr};
    Reaction_t* ReactOnPwrOn = nullptr;
    std::vector<RcvItem_t> Receive;

    void Print() {
        Printf("Name:%S; Type:%u;", Name.c_str(), Type);
        if(ReactOnPwrOn) Printf("PwrOn: %S; ", ReactOnPwrOn->Name.c_str());
        for(int i=0; i<BTN_CNT; i++) {
            if(ReactOnBtns[i]) Printf(" Btn%u:%S;", i+1, ReactOnBtns[i]->Name.c_str());
        }
        for(RcvItem_t &Rcv : Receive) Rcv.Print();
        Printf("\r");
    }
};

class Config_t {
private:
    Reaction_t* GetReactByName(std::string &S);
    uint32_t GetLktTypeByName(std::string &S);
    LocketInfo_t *SelfInfo = nullptr;
public:
    int32_t ID = ID_MIN;
    std::vector<Reaction_t> Rctns;
    std::vector<LocketInfo_t> Lockets;
    void SetSelfType(uint32_t Type);
    uint32_t GetSelfType() { return (SelfInfo == nullptr)? 0 : SelfInfo->Type; }
    void Read();
    bool MustTxFar = false;
    bool MustTxInEachOther = false;
    uint8_t TxPower = 0;
};

extern Config_t Cfg;
