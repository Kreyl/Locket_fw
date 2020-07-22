/*
 * Config.cpp
 *
 *  Created on: Jul 4, 2020
 *      Author: layst
 */

#include "Config.h"
#include "shell.h"
#include "ChunkTypes.h"

Config_t Cfg;

void Config_t::SetSelfType(uint8_t AType) {
    if(AType > TYPE_SOUTH_CURSED) {
        Printf("BadType: %u\r", Type);
        return;
    }
    Type = AType;
    if(!IsAriKaesu()) MustTxInEachOther = true;
}
