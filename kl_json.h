/*
 * json_kl.h
 *
 *  Created on: 16 ����. 2018 �.
 *      Author: Kreyl
 */

#pragma once

#include "kl_lib.h"
#include "color.h"
#include <string>

#define JSON_USE_FILE   FALSE
#define JSON_READONLY   TRUE

#if JSON_USE_FILE
#include "ff.h"
#endif

#define JSON_DEEPLVL_MAX    9L

enum TokenParseRslt_t { tprFail, tprBufEnd, tprContainer, tprValue, tprEndOfContainer };

class JsonParser_t;

enum JsonContainerType_t {jcontNone, jcontArray, jcontContainer};

class JsonObj_t {
private:
    friend class JsonParser_t;
    JsonParser_t *IParser = nullptr;
    void MoveToNextChar() { if(!EndOfBuf()) PS++; }
    bool EndOfBuf();
    char *PS = nullptr;
    uint8_t SkipCommentsAndWhiteSpaces();
    TokenParseRslt_t IParseNext();
    JsonObj_t ArrayItem(const int32_t Indx) const;
    // Payload
    char *Name = nullptr;
    char *Value = nullptr;
    uint32_t NameLen = 0;
    JsonContainerType_t ContType = jcontNone;
public:
    int32_t ValueLen = 0;
    JsonObj_t() {} // Needed for const EmptyNode
    void Clear();

    bool IsEmpty() const { return (!Name and !Value and ContType == jcontNone); }
    JsonObj_t operator[](const char* AName);
    JsonObj_t operator[](const int32_t Indx) { return ArrayItem(Indx); }
    JsonObj_t& operator =(const JsonObj_t& Right) {
        IParser = Right.IParser;
        PS = Right.PS;
        Name = Right.Name;
        Value = Right.Value;
        NameLen = Right.NameLen;
        ContType = Right.ContType;
        ValueLen = Right.ValueLen;
        return *this;
    }
#if !JSON_READONLY
    uint8_t AddRight(const char* AName, const char* AValue);
    void Delete();
#endif
    int32_t ArrayCnt() const;

    // ==== Values ====
    uint8_t ToInt(int32_t *POut) const;
    uint8_t ToUint(uint32_t *POut) const;
    uint8_t ToByte(uint8_t *POut) const;
    uint8_t ToBool(bool *POut) const;
    bool ToBool() const;
    uint8_t ToFloat(float *POut) const;
#if !JSON_READONLY
    uint8_t SetNewValue(const char* NewValue);
    uint8_t SetNewValue(int32_t NewValue);
    uint8_t SetNewValue(bool NewValue);
#endif
    // Strings
    bool NameIsEqualTo(const char* S) const;
    bool ValueIsEmpty() { return (ValueLen == 0); }
    char* GetValue() { return Value; } // Not null-terminated
    bool ValueIsEqualTo(const char* S) const;
    uint8_t CopyValueIfNotEmpty(char** ptr) const;
    uint8_t CopyValueIfNotEmpty(char *PBuf, int32_t SzMax) const;
    uint8_t CopyValueOrNullIfEmpty(char** ptr) const;
    void ToString(std::string &Dst) const;
    // Special case
    uint8_t ToColor(Color_t *PClr) const;
    uint8_t ToTwoInts(int32_t *PInt1, int32_t *PInt2) const;
    uint8_t ToByteArray(uint8_t *PArr, int32_t Len) const;
};

class JsonParser_t {
private:
    int32_t BufSz = 0, AllocBufSz = 0;
    char *IBuf = nullptr;
    bool MustFreeBuf = true;
public:
    JsonObj_t Root;
    uint8_t GetNextRoot(); // Read new object from file
    uint8_t StartReadFromBuf(char* ABuf, uint32_t ALen, uint32_t AAllocLen);
#if JSON_USE_FILE
    uint8_t StartReadFromFile(const char* AFName); // Call this to start reading
    uint8_t SaveToSameFile();
    uint8_t Save(const char* AFName);
#endif

    JsonParser_t() { Root.IParser = this; }
    ~JsonParser_t();
    friend class JsonObj_t;
};
