/*
 * Config.cpp
 *
 *  Created on: Jul 4, 2020
 *      Author: layst
 */

#include "Config.h"
#include "shell.h"
#include "kl_json.h"
#include "ChunkTypes.h"

#include "JsonStrings.h"

Config_t Cfg;

void Config_t::Read() {
//    Printf("aga\r");
    JsonParser_t JParser;
    uint32_t Len = strlen(JsonCfg);
    if(JParser.StartReadFromBuf((char*)JsonCfg, Len, 0) != retvOk) {
        Printf("Read cfg fail\r");
        return;
    }
    int32_t Cnt;
    JsonObj_t NObjs;
#if 1 // ==== Reactions ====
    NObjs = JParser.Root["Reactions"];
    Cnt = NObjs.ArrayCnt();
    Printf("ReaLen: %u\r", Cnt);
    Rctns.resize(Cnt);
    for(int32_t i=0; i<Cnt; i++) {
        JsonObj_t NRea = NObjs[i];  // Src
        Reaction_t *PRea = &Rctns[i]; // Destination
        // Name
        NRea["Name"].ToString(PRea->Name);
        // Vibro
        JsonObj_t NBrr = NRea["VibroType"];
        PRea->VibroType = vbrNone;
        if(NBrr.ValueIsEqualTo(VIBRO_TYPE_BRR)) PRea->VibroType = vbrBrr;
        else if(NBrr.ValueIsEqualTo(VIBRO_TYPE_BRRBRR)) PRea->VibroType = vbrBrrBrr;
        else if(NBrr.ValueIsEqualTo(VIBRO_TYPE_BRRBRRBRR)) PRea->VibroType = vbrBrrBrrBrr;
        // LED sequence
        LedSeq_t *Seq = &PRea->Seq;
        Seq->clear();
        JsonObj_t NSeq = NRea["Sequence"];
        int32_t SeqCnt = NSeq.ArrayCnt(); // May be 0
        for(int32_t j=0; j<SeqCnt; j++) {
            JsonObj_t NItem = NSeq[j];
            LedRGBChunk_t Chunk(csEnd); // no matter what
            JsonObj_t NSubitem;
            // Is it csSetup?
            NSubitem = NItem["Color"];
            if(!NSubitem.IsEmpty()) { // Really color!
                Chunk.ChunkSort = csSetup;
                if(NSubitem.ToColor(&Chunk.Color) == retvOk) {
                    // Todo: smooth
                    Seq->push_back(Chunk);
                    continue;
                }
            }
            // Is it csWait?
            NSubitem = NItem["Wait"];
            if(!NSubitem.IsEmpty()) { // Really wait!
                Chunk.ChunkSort = csWait;
                if(NSubitem.ToUint(&Chunk.Time_ms) == retvOk) {
                    Seq->push_back(Chunk);
                    continue;
                }
            }
        } // for seq items
        // Finalize seq if not empty TODO Really required?
        if(Seq->size()) Seq->push_back(LedRGBChunk_t(csEnd));
        // Done
        PRea->Print();
    } // For reactions
#endif
#if 1 // ==== Locket types ====


#endif

}
