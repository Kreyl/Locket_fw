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


Reaction_t* Config_t::GetReactByName(std::string &S) {
    if(S.empty()) return nullptr;
    for(Reaction_t& Rea : Rctns) {
        if(S == Rea.Name) return &Rea;
    }
    return nullptr;
}

uint32_t Config_t::GetLktTypeByName(std::string &S) {
    if(S.empty()) return 0;
    for(LocketInfo_t &Lkt : Lockets) {
        if(S == Lkt.Name) return Lkt.Type;
    }
    return 0;
}

void Config_t::SetSelfType(uint32_t Type) {
    for(auto &Lkt : Lockets) {
        if(Lkt.Type == Type) {
            SelfInfo = &Lkt;
            if(Lkt.Receive.size() == 0) {
                MustTxFar = true;
                MustTxInEachOther = false;
            }
            else {
                MustTxFar = false;
                MustTxInEachOther = true;
            }
            return;
        }
    }
    SelfInfo = nullptr;
    MustTxFar = false;
    MustTxInEachOther = false;
    Printf("BadType: %u\r", Type);
}

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
        // Finalize seq if not empty
        if(Seq->size()) Seq->push_back(LedRGBChunk_t(csEnd));
        // Done
        PRea->Print();
    } // For reactions
#endif
#if 1 // ==== Locket types ====
    std::string S;
    NObjs = JParser.Root["Lockets"];
    Cnt = NObjs.ArrayCnt();
    Printf("LocketLen: %u\r", Cnt);
    Lockets.resize(Cnt);
    for(int32_t i=0; i<Cnt; i++) {
        JsonObj_t NLkt = NObjs[i];  // Src
        LocketInfo_t *PLkt = &Lockets[i]; // Destination

        // ==== Name & Type ====
        NLkt["Name"].ToString(PLkt->Name);
        NLkt["Type"].ToByte(&PLkt->Type);

        // ==== Reaction on button ====
        NLkt["FirstButton"].ToString(S);
        PLkt->ReactOnBtns[0] = GetReactByName(S);
        NLkt["SecondButton"].ToString(S);
        PLkt->ReactOnBtns[1] = GetReactByName(S);
        NLkt["ThirdButton"].ToString(S);
        PLkt->ReactOnBtns[2] = GetReactByName(S);
        // All buttons
        NLkt["Button"].ToString(S);
        if(!S.empty()) {
            PLkt->ReactOnBtns[0] = GetReactByName(S);
            PLkt->ReactOnBtns[1] = PLkt->ReactOnBtns[0];
            PLkt->ReactOnBtns[2] = PLkt->ReactOnBtns[0];
        }

        // ==== Reaction on PwrOn ====
        NLkt["ReactOnPwrOn"].ToString(S);
        PLkt->ReactOnPwrOn = GetReactByName(S);
    } // for

    // ==== Receive ====
    for(int32_t i=0; i<Cnt; i++) {
        JsonObj_t NLkt = NObjs[i];  // Src
        LocketInfo_t *PLkt = &Lockets[i]; // Destination
        JsonObj_t NRcv = NLkt["Receive"];
        uint32_t RcvCnt = NRcv.ArrayCnt();
        PLkt->Receive.resize(RcvCnt);
        for(uint32_t j=0; j<RcvCnt; j++) {
            JsonObj_t NRcvItem = NRcv[j];
            RcvItem_t *PRcv = &PLkt->Receive[j];
            NRcvItem["Reaction"].ToString(S);
            PRcv->React = GetReactByName(S);
            NRcvItem["Distance"].ToInt(&PRcv->Distance);
            // Get Type
            NRcvItem["Source"].ToString(S);
            PRcv->Src = GetLktTypeByName(S);
        }
        PLkt->Print();
        chThdSleepMilliseconds(18);
    }

//    for(uint32_t j=0; j<RcvCnt; j++) {
//        JsonObj_t NRcvItem = NRcv[j];
//        RcvItem_t *PRcv = &PLkt->Receive[j];
//        NRcvItem["Source"].ToString(PRcv->SrcStr);
//
////            NRcvItem["Reaction"].ToString(S);
////            PRcv->React = GetReactByName(S);
//    }

    // Find Lkt types
//    for(LocketInfo_t &Lkt : Lockets) {
//        for(RcvItem_t &Rcv : Lkt.Receive) {
//            for(LocketInfo_t &Lkt4Name : Lockets) {
//                if(Rcv.SrcStr == Lkt4Name.Name) {
//                    Rcv.Src = Lkt4Name.Type;
//                    break;
//                }
//            }
//        }
//        Lkt.Print();
//    }
#endif


}
