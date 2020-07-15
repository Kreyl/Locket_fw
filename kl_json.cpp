/*
 * json_kl.cpp
 *
 *  Created on: 16 ����. 2018 �.
 *      Author: Kreyl
 */

#include <kl_json.h>
#include <malloc.h>

static const JsonObj_t EmptyNode;

#if JSON_USE_FILE
#define JBUFSZ      10000 // Max sz of buf to read file to
#include "kl_fs_utils.h"
#endif

void JsonObj_t::Clear() {
    Name = nullptr;
    Value = nullptr;
    NameLen = 0;
    ValueLen = 0;
    ContType = jcontNone;
}

bool JsonObj_t::EndOfBuf() { return (PS - IParser->IBuf) >= IParser->BufSz; }

// Find node of next lvl with specified name
JsonObj_t JsonObj_t::operator[](const char* AName) {
    if(ContType == jcontNone) return EmptyNode;
    JsonObj_t Node;
    Node.PS = PS;
    Node.IParser = IParser;
    int32_t FDeepLvl = 0; // The only needed lvl is 0
    while(true) {
        TokenParseRslt_t Rslt = Node.IParseNext();
        switch(Rslt) {
            case tprContainer:
                if(FDeepLvl == 0) { // Check name
                    if(Node.Name and strncasecmp(AName, Node.Name, Node.NameLen) == 0) return Node;
                }
                FDeepLvl++;
                break;

            case tprValue:
                if(FDeepLvl == 0) { // Check name
                    if(Node.Name and strncasecmp(AName, Node.Name, Node.NameLen) == 0) return Node;
                }
                break;

            case tprEndOfContainer:
                FDeepLvl--;
                if(FDeepLvl < 0) return EmptyNode; // Out of our container
                break;

            case tprFail:
            case tprBufEnd:
                return EmptyNode;
                break;
        } // switch
    } // while true
}

JsonObj_t JsonObj_t::ArrayItem(const int32_t Indx) const {
    if(ContType == jcontNone) return EmptyNode;
    JsonObj_t Node;
    Node.PS = PS;
    Node.IParser = IParser;
    int32_t FDeepLvl = 0; // The only needed lvl is 0
    int32_t Cnt = 0;
    while(true) {
        TokenParseRslt_t Rslt = Node.IParseNext();
        switch(Rslt) {
            case tprContainer:
                if(FDeepLvl == 0) {
                    if(Cnt++ == Indx) return Node;
                }
                FDeepLvl++;
                break;

            case tprValue:
                if(FDeepLvl == 0) { // Check name
                    if(Cnt++ == Indx) return Node;
                }
                break;

            case tprEndOfContainer:
                FDeepLvl--;
                if(FDeepLvl < 0) return EmptyNode; // Out of our container
                break;

            case tprFail:
            case tprBufEnd:
                return EmptyNode;
                break;
        } // switch
    } // while true
}

#if !JSON_READONLY
uint8_t JsonObj_t::SetNewValue(const char* NewValue) {
    int32_t NewLen = strlen(NewValue);
    bool NeedToAddQuotation = (*(Value - 1) != '"');
    if(NeedToAddQuotation) NewLen += 2;
    // Check if bufsz is enough
    int32_t NewBufSz = IParser->BufSz - ValueLen + NewLen;
    if(NewBufSz > IParser->AllocBufSz) return retvFail;
    // Move remainder of buffer
    char* End = Value + ValueLen;
    char* NewEnd = Value + NewLen;
    size_t MoveLen = (IParser->IBuf + IParser->BufSz) - End;
    memmove(NewEnd, End, MoveLen);
    IParser->BufSz = NewBufSz;
    // Add quotation if needed
    if(NeedToAddQuotation) {
        *Value = '"';  //12"456"89
        *(Value + NewLen - 1) = '"';
        Value++; // Next to quotation
    }
    // Put new value
    ValueLen = NeedToAddQuotation? (NewLen-2) : NewLen;
    memcpy(Value, NewValue, ValueLen);
    return retvOk;
}

uint8_t JsonObj_t::SetNewValue(int32_t NewValue) {
    char S[33];
    return SetNewValue(itoa(NewValue, S, 10));
}

uint8_t JsonObj_t::SetNewValue(bool NewValue) {
    if(NewValue) return SetNewValue("true");
    else return SetNewValue("false");
}

uint8_t JsonObj_t::AddRight(const char* AName, const char* AValue) {
    int32_t NameLen = strlen(AName);
    int32_t ValLen = strlen(AValue);
    char *WhereTo;
    if(ContType == jcontNone) { // Value
        WhereTo = PS;
    }
    else {  // Container
        JsonObj_t Node;
        Node.PS = PS;
        Node.IParser = IParser;
        int32_t FDeepLvl = 0;
        while(FDeepLvl >= 0) {
            TokenParseRslt_t Rslt = Node.IParseNext();
            switch(Rslt) {
                case tprContainer: FDeepLvl++; break;
                case tprEndOfContainer: FDeepLvl--; break;
                case tprFail:
                case tprBufEnd:
                    FDeepLvl = -9; // Get out
                    break;
                default: break;
            } // switch
        } // while true
        WhereTo = Node.PS;
    }

    int32_t NewLen = NameLen + ValLen + 2 + 2 + 1 + 1; // ,"Name":"Value"
    // Check if bufsz is enough
    int32_t NewBufSz = IParser->BufSz + NewLen;
    if(NewBufSz > IParser->AllocBufSz) return retvFail;
    // Move remainder of buffer
    size_t MoveLen = (IParser->IBuf + IParser->BufSz) - WhereTo;
    char* End = WhereTo + NewLen;
    memmove(End, WhereTo, MoveLen);
    IParser->BufSz = NewBufSz;
    // Put new Node
    *WhereTo++ = ',';
    *WhereTo++ = '"';
    memcpy(WhereTo, AName, NameLen);
    WhereTo += NameLen;
    *WhereTo++ = '"';
    *WhereTo++ = ':';
    *WhereTo++ = '"';
    memcpy(WhereTo, AValue, ValLen);
    WhereTo += ValLen;
    *WhereTo++ = '"';
    return retvOk;
}

void JsonObj_t::Delete() {
    if(IsEmpty()) return;
    char *Start = (*(Name-1) == '"')? (Name-1) : Name;
    // Find End
    char* End = PS;
    if(ContType != jcontNone) {
        JsonObj_t Node;
        Node.PS = PS;
        Node.IParser = IParser;
        int32_t FDeepLvl = 0;
        while(FDeepLvl >= 0) { // while not out of our container
            TokenParseRslt_t Rslt = Node.IParseNext();
            switch(Rslt) {
                case tprContainer: FDeepLvl++; break;
                case tprValue: break;
                case tprEndOfContainer: FDeepLvl--; break;

                case tprFail:
                case tprBufEnd:
                    FDeepLvl = -9; // Get out
                    break;
            } // switch
        } // while true
        End = Node.PS;
    }
    if(*End == ',') End++;
    size_t MoveLen = (IParser->IBuf + IParser->BufSz) - End;
    memmove(Start, End, MoveLen);
    int32_t DeletedSz = End - Start;
    IParser->BufSz -= DeletedSz;
    Clear();
}
#endif

#if 1 // ==== Values ====
int32_t JsonObj_t::ArrayCnt() const {
    if(ContType != jcontArray) return 0;
    int32_t Cnt = 0;
    JsonObj_t Node;
    Node.PS = PS;
    Node.IParser = IParser;
    int32_t FDeepLvl = 0; // The only needed lvl is 0
    while(true) {
        TokenParseRslt_t Rslt = Node.IParseNext();
        switch(Rslt) {
            case tprContainer:
                if(FDeepLvl == 0) Cnt++;
                FDeepLvl++;
                break;

            case tprValue:
                if(FDeepLvl == 0) Cnt++;
                break;

            case tprEndOfContainer:
                FDeepLvl--;
                if(FDeepLvl < 0) return Cnt; // Out of our container
                break;

            case tprFail:
            case tprBufEnd:
                return Cnt;
                break;
        } // switch
    } // while true
}

// Try to convert
uint8_t JsonObj_t::ToInt(int32_t *POut) const {
    if(Value) {
        char *p;
        int32_t Rslt = strtol(Value, &p, 0);
        if((p - Value) == ValueLen) {
            *POut = Rslt;
            return retvOk;
        }
        else return retvNotANumber;
    }
    return retvFail;
}

uint8_t JsonObj_t::ToUint(uint32_t *POut) const {
    if(Value) {
        char *p;
        uint32_t Rslt = strtoul(Value, &p, 0);
        if((p - Value) == ValueLen) {
            *POut = Rslt;
            return retvOk;
        }
        else return retvNotANumber;
    }
    return retvFail;
}

uint8_t JsonObj_t::ToByte(uint8_t *POut) const {
    int32_t tmp;
    if(ToInt(&tmp) == retvOk) {
        *POut = (uint8_t)tmp;
        return retvOk;
    }
    else return retvFail;
}

uint8_t JsonObj_t::ToBool(bool *POut) const {
    if(!POut) return retvFail;
    if(Value) {
        if(Value[0] == 0 or Value[0] == '0' or Value[0] == 'f' or Value[0] == 'F') *POut = false;
        else *POut = true;
        return retvOk;
    }
    else return retvNotANumber;
}

// Returns false if false or fail
bool JsonObj_t::ToBool() const {
    if(Value) {
        if(Value[0] == 0 or Value[0] == '0' or Value[0] == 'f' or Value[0] == 'F') return false;
        else return true;
    }
    else return false;
}

uint8_t JsonObj_t::ToFloat(float *POut) const {
    if(Value) {
        char *p;
        float Rslt = strtof(Value, &p);
        if((p - Value) == ValueLen) {
            *POut = Rslt;
            return retvOk;
        }
    }
    return retvNotANumber;
}

// ==== Strings ====
bool JsonObj_t::NameIsEqualTo(const char* S) const {
    if(!Name or *Name == 0) return false;
    return (strncasecmp(Name, S, NameLen) == 0);
}
bool JsonObj_t::ValueIsEqualTo(const char* S) const {
    if(!Value or *Value == 0) return false;
    return (strncasecmp(Value, S, ValueLen) == 0);
}

uint8_t JsonObj_t::CopyValueIfNotEmpty(char **ptr) const {
    if(!Value or *Value == 0) return retvEmpty;
    char *p = (char*)malloc(ValueLen+1);
    if(p == nullptr) return retvFail;
    strncpy(p, Value, ValueLen);
    p[ValueLen] = 0;
    *ptr = p;
    return retvOk;
}

// SzMax includes final \0
uint8_t JsonObj_t::CopyValueIfNotEmpty(char *PBuf, int32_t SzMax) const {
    if(!Value or *Value == 0) return retvEmpty;
    if(PBuf == nullptr) return retvFail;
    if((ValueLen+1) > SzMax) return retvOverflow;
    strncpy(PBuf, Value, ValueLen);
    PBuf[ValueLen] = 0;
    return retvOk;
}

uint8_t JsonObj_t::CopyValueOrNullIfEmpty(char **ptr) const {
    if(!Value or *Value == 0) {
        *ptr = nullptr;
        return retvOk;
    }
    else return CopyValueIfNotEmpty(ptr);
}

void JsonObj_t::ToString(std::string &Dst) const {
    Dst.clear();
    if(ValueLen > 0) {
        Dst.assign(Value, ValueLen);
    }
}

// ==== Special case ====
uint8_t JsonObj_t::ToColor(Color_t *PClr) const {
    // Check if random or off
    if(ValueIsEqualTo("random")) PClr->BeRandom();
    else if(ValueIsEqualTo("Off")) *PClr = clBlack;
    // Not random or off
    else {
        int32_t Cnt = ArrayCnt();
        if(Cnt < 3) return retvFail;
        JsonObj_t NItem = ArrayItem(0);
        if(NItem.ToByte(&PClr->R) != retvOk) return retvFail;
        NItem = ArrayItem(1);
        if(NItem.ToByte(&PClr->G) != retvOk) return retvFail;
        NItem = ArrayItem(2);
        if(NItem.ToByte(&PClr->B) != retvOk) return retvFail;
        if(Cnt >= 4) {
            NItem = ArrayItem(3);
            if(NItem.ToByte(&PClr->W) != retvOk) return retvFail;
        }
        else PClr->Brt = 0; // not random
    }
    return retvOk;
}

uint8_t JsonObj_t::ToTwoInts(int32_t *PInt1, int32_t *PInt2) const {
    JsonObj_t Node;
    Node.PS = PS;
    Node.IParser = IParser;
    int32_t n = 0;
    int32_t FDeepLvl = 0; // The only needed lvl is 0
    while(true) {
        TokenParseRslt_t Rslt = Node.IParseNext();
        switch(Rslt) {
            case tprContainer:
                FDeepLvl++;
                break;

            case tprValue:
                if(FDeepLvl == 0) {
                    if(n == 0) {
                        if(Node.ToInt(PInt1) != retvOk) return retvFail;
                        n++;
                    }
                    else { // n == 1
                        return Node.ToInt(PInt2);
                    }
                }
                break;

            case tprEndOfContainer:
                FDeepLvl--;
                if(FDeepLvl < 0) return retvFail; // Out of our container
                break;

            case tprFail:
            case tprBufEnd:
                return retvFail;
                break;
        } // switch
    } // while true
}

uint8_t JsonObj_t::ToByteArray(uint8_t *PArr, int32_t Len) const {
    int32_t Cnt = 0;
//    JsonObj_t *Node = Child;
//    while(Node and Cnt < Len) {
//        if(Node->ToByte(&PArr[Cnt]) != retvOk) return retvFail;
//        Cnt++;
//        Node = Node->Neighbor;
//    }
    return (Cnt == Len)? retvOk : retvFail;
}
#endif

#if 1 // ==== Parsing ====
uint8_t JsonObj_t::SkipCommentsAndWhiteSpaces() {
    bool IsInsideSingleComent = false;
    bool IsInsideLongComment = false;
    bool WasStar = false;
    bool WasSlash = false;
    while(true) {
        char c = *PS;
        if(IsInsideSingleComent) { // Ends with line
            // Check if EOL
            if(c == '\r' or c == '\n') IsInsideSingleComent = false;
        }
        else if(IsInsideLongComment) { // ends with star and slash
            if(WasStar and c == '/') IsInsideLongComment = false;
            else WasStar = (c == '*');
        }

        // Find comments start
        else if(c == '/') {
            if(WasSlash) {
                IsInsideSingleComent = true;
                WasSlash = false;
            }
            else WasSlash = true;
        }
        else if(c == '*' and WasSlash) {
            IsInsideLongComment = true;
        }
        // Not a comment, not a whitespace
        else if(c > ' ') return retvOk;

        MoveToNextChar();
        if(EndOfBuf()) return retvEmpty;
    }
}

TokenParseRslt_t JsonObj_t::IParseNext() {
    bool IsReadingName = true;
    bool IsInsideQuotes = false;

    // Initial Name and Value
    Name = nullptr;
    Value = nullptr;
    NameLen = 0;
    ValueLen = 0;
    ContType = jcontNone;

    while(true) {
        if(!IsInsideQuotes) {
            uint8_t r = SkipCommentsAndWhiteSpaces();
            if(r == retvFail) return tprFail;
            else if(r == retvEmpty) return tprBufEnd;
        }
        char c = *PS;

        if(IsReadingName) {
            if(c == '\"') IsInsideQuotes = !IsInsideQuotes;
            else if(IsInsideQuotes) { // Add everything
                if(!Name) Name = PS; // Start of name
                NameLen++;
            }
            else if(c == ',') { // End of token
                if(NameLen != 0) { // This is not a pair, but a single value => node without name
                    // Save string to Value
                    Value = Name;
                    ValueLen = NameLen;
                    Name = nullptr;
                    NameLen = 0;
                    MoveToNextChar();
                    return tprValue;
                }
            }
            else if(c == ']' or c == '}') {
                if(NameLen == 0) {
                    MoveToNextChar();
                    Name = nullptr;
                    return tprEndOfContainer;
                }
                else { // This is not a pair, but a single value => node without name
                    // Save string to Value
                    Value = Name;
                    ValueLen = NameLen;
                    Name = nullptr;
                    NameLen = 0;
                    return tprValue;
                }
            }
            else if(c == '{') { // Container start
                ContType = jcontContainer;
                MoveToNextChar();
                return tprContainer;
            }
            else if(c == '[') { // Array start
                ContType = jcontArray;
                MoveToNextChar();
                return tprContainer;
            }
            else if(c == ':') { // seems like pair "name: value"
                IsReadingName = false;
            }
            // Just char, add it to name
            else {
                if(!Name) Name = PS; // Start of name
                NameLen++;
            }
        } // Reading name

        // Reading value
        else {
            if(c == '\"') {
                IsInsideQuotes = !IsInsideQuotes;
            }
            else if(IsInsideQuotes) { // Add everything
                if(!Value) Value = PS; // Start of name
                ValueLen++;
            }
            else if(c == ',') { // End of value
                return tprValue;
            }
            else if(c == ']' or c == '}') { // End of value
                return tprValue;
            }
            else if(c == '{') {
                ContType = jcontContainer;
                Value = nullptr;
                ValueLen = 0;
                MoveToNextChar();
                return tprContainer;
            }
            else if(c == '[') {
                ContType = jcontArray;
                Value = nullptr;
                ValueLen = 0;
                MoveToNextChar();
                return tprContainer;
            }
            // Just char, add it to value
            else {
                if(!Value) Value = PS; // Start of name
                ValueLen++;
            }
        }
        MoveToNextChar();
        if(EndOfBuf()) return tprBufEnd;
    } // while true
}
#endif

#if 1 // ============================ Json Parser ==============================
#if JSON_USE_FILE
uint8_t JsonParser_t::StartReadFromFile(const char* AFName) {
    // Open file
    if(IFile) free(IFile);
    IFile = (FIL*)malloc(sizeof(FIL));
    if(!IFile) return retvFail;
    uint8_t Rslt = TryOpenFileReadWrite(AFName, IFile);
    if(Rslt != retvOk) return Rslt;
    // Allocate buf for file
    BufSz = f_size(IFile);
    if(BufSz == 0) return retvEndOfFile;
    // Reserve some space for content changing.
    // Realloc is not good as all pointers become invalid.
    AllocBufSz = (BufSz * 3) / 2;
    TRIM_VALUE(AllocBufSz, JBUFSZ);
    IBuf = (char*)malloc(AllocBufSz);
    if(!IBuf) return retvFail;
    MustFreeBuf = true;
    // Read file to buf
    if(f_read(IFile, IBuf, BufSz, (UINT*)&BufSz) != FR_OK) return retvFail;
    if(BufSz == 0) return retvEndOfFile;
    return GetNextRoot();
}
#endif

uint8_t JsonParser_t::StartReadFromBuf(char* ABuf, uint32_t ALen, uint32_t AAllocLen) {
    IBuf = ABuf;
    BufSz = ALen;
    AllocBufSz = AAllocLen;
    MustFreeBuf = false;
    return GetNextRoot();
}

uint8_t JsonParser_t::GetNextRoot() {
    Root.Clear();
    Root.PS = IBuf;
    while(true) {
        TokenParseRslt_t ParseRslt = Root.IParseNext();
        if(ParseRslt == tprContainer or ParseRslt == tprValue) break;
        else if(ParseRslt == tprFail or ParseRslt == tprBufEnd) return retvFail;
    }
    // Get size of occupied buffer
    JsonObj_t Node;
    if(Root.Name) {
        Node.PS = Root.Name;
        // Check quotes
        if(Node.PS != IBuf) {
            if(*(Node.PS - 1) == '"') Node.PS--;
        }
    }
    else Node.PS = Root.PS - 1;
    Node.IParser = this;
    int32_t FDeepLvl = 0;
    do {
        TokenParseRslt_t Rslt = Node.IParseNext();
        switch(Rslt) {
            case tprContainer: FDeepLvl++; break;
            case tprValue: break;
            case tprEndOfContainer: FDeepLvl--; break;
            case tprFail: return retvFail; break;
            case tprBufEnd: FDeepLvl = -9; break; // get out
        } // switch
    } while(FDeepLvl > 0);
    BufSz = Node.PS - IBuf;
    return retvOk;
}

#if JSON_USE_FILE
uint8_t JsonParser_t::SaveToSameFile() {
    if(!IFile) return retvFail;
    if(f_lseek(IFile, 0) != FR_OK) return retvFail;
    if(f_write(IFile, IBuf, (UINT)BufSz, (UINT*)&BufSz) != FR_OK) return retvFail;
    if(f_truncate(IFile) != FR_OK) return retvFail; // Cut remainder of file
    if(f_sync(IFile) != FR_OK) return retvFail;
    return retvOk;
}

uint8_t JsonParser_t::Save(const char* AFName) {
    // Calculate size of occupied buffer
    JsonObj_t Node;
    Node.PS = IBuf;
    Node.IParser = this;
    int32_t FDeepLvl = 0; // The only needed lvl is 0
    do {
        TokenParseRslt_t Rslt = Node.IParseNext();
        switch(Rslt) {
            case tprContainer: FDeepLvl++; break;
            case tprValue: break;
            case tprEndOfContainer: FDeepLvl--; break;

            case tprFail:
            case tprBufEnd:
                return retvFail;
                break;
        } // switch
    } while(FDeepLvl > 0);
    uint32_t SzToSave = Node.PS - IBuf;
    // Save
    FIL *FFile = (FIL*)malloc(sizeof(FIL));
    if(!FFile) return retvFail;
    if(TryOpenFileRewrite(AFName, FFile) != retvOk) {
        free(FFile);
        return retvFail;
    }
    UINT dummy;
    FRESULT res = f_write(FFile, IBuf, SzToSave, &dummy);
    f_close(FFile);
    free(FFile);
    return (res == FR_OK)? retvOk : retvFail;
}
#endif

JsonParser_t::~JsonParser_t() {
#if JSON_USE_FILE
    if(IFile) {
        f_close(IFile);
        free(IFile);
    }
#endif
    if(IBuf and MustFreeBuf) free(IBuf);
}
#endif
