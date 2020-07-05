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

// Locket settings
struct ReactItem_t {
    int32_t Source;     // Type to react to
    Reaction_t *PReact; // What to do
    int32_t Distance;   // Min dist when react
};

struct LocketType_t {
    Reaction_t *ReactOnPwrOn;
    std::vector<ReactItem_t> React;
};

class Config_t {
private:

public:
    std::vector<Reaction_t> Rctns;
    std::vector<LocketType_t> LcktType;
    void Read();
};

extern Config_t Cfg;
