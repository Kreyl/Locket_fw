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
#include "Sequences.h"

#define ID_BAD                  (-1)
#define ID_MIN                  0
#define ID_MAX                  243
#define ID_DEFAULT              ID_MIN

#define TYPE_ARI                0
#define TYPE_KAESU              1
#define TYPE_NORTH              2
#define TYPE_NORTH_STRONG       3
#define TYPE_SOUTH              4
#define TYPE_SOUTH_STRONG       5
#define TYPE_NORTH_CURSED       6
#define TYPE_SOUTH_CURSED       7

#define TYPE_NORTH_ATTACK       8
#define TYPE_NORTH_RETREAT      9
#define TYPE_SOUTH_ATTACK       10
#define TYPE_SOUTH_RETREAT      11

#define TYPE_CNT                12 // Sitting in the dirt, feeling kinda hurt

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
    uint8_t Type = 0;
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
    const LocketInfo_t ZeroInfo;
public:
    LocketInfo_t *SelfInfo = (LocketInfo_t*)&ZeroInfo;
    bool IsNorth() { return (SelfInfo->Type == TYPE_NORTH or SelfInfo->Type == TYPE_NORTH_STRONG or SelfInfo->Type == TYPE_NORTH_CURSED); }
    bool IsSouth() { return (SelfInfo->Type == TYPE_SOUTH or SelfInfo->Type == TYPE_SOUTH_STRONG or SelfInfo->Type == TYPE_SOUTH_CURSED); }
    bool IsStrong() { return (SelfInfo->Type == TYPE_NORTH_STRONG or SelfInfo->Type == TYPE_SOUTH_STRONG); }

    uint32_t GetCountOfTypes() { return Lockets.size(); }
    int32_t ID = ID_MIN;
    std::vector<Reaction_t> Rctns;
    std::vector<LocketInfo_t> Lockets;

    void SetSelfType(uint32_t Type);
    void Read();
    bool MustTxFar = false;
    bool MustTxInEachOther = false;
    uint8_t TxPower = 0;
};

extern Config_t Cfg;
