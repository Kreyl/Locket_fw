/*
 * types.h
 *
 *  Version: 20260810
 *      Author: Kreyl Laurelindo
 */

#pragma once

#include <cstdint>
#include <type_traits>

// ==== Simple types ====
struct RGBW32 { int32_t R, G, B, W; };
struct MinMaxI32 { int32_t min, max; };
union Int32OrPChar {
    int32_t i32;
    char *pchar;
};

union U32U16U8 {
    uint32_t u32;
    uint16_t u16[2];
    uint8_t u8[4];
};
static_assert(sizeof(U32U16U8) == 4, "U32U16U8 size is not 4");

struct Time_t {
    uint8_t h, m, s;
    void Reset() { h = m = s = 0; }
    void IncH() { if(h+1 > 23) h = 0; else h++; }
    void IncM() {
        if(m+1 > 59) {
            m = 0;
            IncH();
        }
        else m++;
    }
    void IncS() {
        if(s+1 > 59) {
            s = 0;
            IncM();
        }
        else s++;
    }
};
static_assert(std::is_trivially_copyable<Time_t>::value, "Time_t is not trivially copyable");
static_assert(std::is_trivial<Time_t>::value, "Time_t is not trivial");

// ==== TBuf ====
template <typename T>
class TBuf {
public:
    uint32_t len;
    T *ptr;
    TBuf() : len(0), ptr(nullptr) {}
    TBuf(T* abuf, uint32_t alen) : len(alen), ptr(abuf) {}
    // Iterator
    uint32_t size() const noexcept { return len; }
    T* begin() const { return ptr; } // First element
    T* end() const { return begin() + size(); } // One past the last element
};

using TBufBool = TBuf<bool>;
using TBufU8 = TBuf<uint8_t>;

// ==== Return values ====
struct retv {
public:
    enum Enum {
        Ok =             0,
        Fail =           1,
        Timeout =        2,
        Reset =          3,
        Busy =           4,
        InProgress =     5,
        CmdError =       6,
        CmdUnknown =     7,
        BadValue =       8,
        New =            9,
        Same =           10,
        Last =           11,
        Empty =          12,
        Overflow =       13,
        NotANumber =     14,
        WriteProtect =   15,
        WriteError =     16,
        EndOfFile =      17,
        NotFound =       18,
        BadState =       19,
        Disconnected =   20,
        Collision =      21,
        CRCError =       22,
        NACK =           23,
        NoAnswer =       24,
        OutOfMemory =    25,
        NotAuthorised =  26,
        NoChanges =      27,
        Stop =           28,
        Continue =       29,
        Ended =          30,
        NotCalibrated =  31,
        Unsupported =    32,
        Idle =           33,
        NotInitialized = 34,
    } rslt;
    constexpr retv() : rslt(Enum::Ok) {}
    constexpr retv(Enum value) : rslt(value) {}
    explicit operator bool() const = delete; // Prevent usage: if(Retv)
    constexpr operator Enum() const { return rslt; } // Enable switch usage
    // Resolve comparison ambiguity
    constexpr bool operator== (Enum e) const { return rslt == e; }
    constexpr bool operator!= (Enum e) const { return rslt != e; }
    constexpr bool operator== (retv r) const { return rslt == r.rslt; }
    constexpr bool operator!= (retv r) const { return rslt != r.rslt; }
    // Helper methods
    constexpr bool IsOk()      const { return rslt == Ok; }
    constexpr bool NotOk()     const { return rslt != Ok; }
    constexpr bool IsTimeout() const { return rslt == Timeout; }
    constexpr bool IsEmpty()   const { return rslt == Empty; }
    constexpr bool IsEnded()   const { return rslt == Ended; }
    constexpr uint8_t ToU8()   const { return rslt; }
    constexpr const char* ToString() const {
        switch(rslt) {
            case Ok:             return "Ok";
            case Fail:           return "Fail";
            case Timeout:        return "Timeout";
            // case Reset:          return "Reset";
            // case Busy:           return "Busy";
            // case InProgress:     return "InProgress";
            case CmdError:       return "CmdError";
            // case CmdUnknown:     return "CmdUnknown";
            case BadValue:       return "BadValue";
            // case New:            return "New";
            // case Same:           return "Same";
            // case Last:           return "Last";
            // case Empty:          return "Empty";
            case Overflow:       return "Overflow";
            // case NotANumber:     return "NotANumber";
            // case WriteProtect:   return "WriteProtect";
            // case WriteError:     return "WriteError";
            // case EndOfFile:      return "EndOfFile";
            case NotFound:       return "NotFound";
            // case BadState:       return "BadState";
            // case Disconnected:   return "Disconnected";
            // case Collision:      return "Collision";
            case CRCError:       return "CRCError";
            // case NACK:           return "NACK";
            // case NoAnswer:       return "NoAnswer";
            // case OutOfMemory:    return "OutOfMemory";
            // case NotAuthorised:  return "NotAuthorised";
            // case NoChanges:      return "NoChanges";
            // case Stop:           return "Stop";
            // case Continue:       return "Continue";
            // case Ended:          return "Ended";
            // case NotCalibrated:  return "NotCalibrated";
            // case Unsupported:    return "Unsupported";
            // case Idle:           return "Idle";
            // case NotInitialized: return "NotInitialized";
            default:             return "Unknown";
        }
    }
};

template<retv::Enum E>
static constexpr const char* RetvToString() { return "Unknown"; }
template<> constexpr const char* RetvToString<retv::Ok>() { return "Ok"; }
template<> constexpr const char* RetvToString<retv::Fail>() { return "Fail"; }

// Semaphore states
#define SEM_TAKEN       true
#define SEM_NOT_TAKEN   false

/* ==== Retval with some payload ====
Example:
using CO2Rslt = RetvVal<SnsData>;
CO2Rslt r = Co2Sns.read_measurement();
if(r.IsOk()) Printf("%d %d %d\r", r->CO2, r->Temp, r->RH);
*/

template <typename T>
struct RetvVal : public retv {
    T v;
    RetvVal() : retv(), v{} {}
    RetvVal(retv arslt) : retv(arslt), v{} {}
    RetvVal(retv arslt, T av) : retv(arslt), v(av) {}
    constexpr RetvVal(retv::Enum e) : retv(e), v{} {}
    constexpr RetvVal& operator = (retv aretv) { this->rslt = aretv.rslt; return *this; }
    constexpr RetvVal& operator = (RetvVal<T> aretvval) {
        this->rslt = aretvval.rslt;
        this->v = aretvval.v;
        return *this;
    }
    T* operator ->() { return &v; }
    T& operator *() { return v; }
};

using RetvValU8  = RetvVal<uint8_t>;
using RetvValChar = RetvVal<char>;
using RetvValU16 = RetvVal<uint16_t>;
using RetvValI16 = RetvVal<int16_t>;
using RetvValU32 = RetvVal<uint32_t>;
using RetvValI32 = RetvVal<int32_t>;
using RetvValFloat = RetvVal<float>;
using RetvValPChar = RetvVal<char*>;
using RetvValU16x2 = RetvVal<uint16_t[2]>;
using RetvValI32x2 = RetvVal<int32_t[2]>;
using RetvValTBufBool = RetvVal<TBufBool>;
using RetvValTBufU8 = RetvVal<TBufU8>;
using RetvValInt32OrPChar = RetvVal<Int32OrPChar>;

// ==== Functional types ====
typedef void (*ftVoidVoid)(void);
typedef retv (*ftRetvVoid_t)(void);
typedef void (*ftVoidU8)(uint8_t);
typedef void (*ftVoidU32)(uint32_t);
typedef void (*ftVoidPVoid)(void*);
typedef void (*ftVoidPU16)(uint16_t*);
typedef void (*ftVoidPVoidW32)(void*, uint32_t);
typedef void (*ftVoidU8U16)(uint8_t, uint16_t);
typedef void (*ftVoidU32U32)(uint32_t, uint32_t);
typedef void (*ftVoidPU8PU32)(uint8_t*, uint32_t*);
typedef retv (*ftRetvU32U32)(uint32_t, uint32_t);
typedef retv (*ftRetvPU8U32)(uint8_t*, uint32_t);

#define NAME2VOIDFUNC(name) void name(void)

enum class Inv {NotInverted, Inverted};
enum class BitOrder {MSB, LSB};
enum class RiseFall {None, Rising, Falling, Both};

#ifndef countof
#define countof(A)  (sizeof(A)/sizeof(A[0]))
#endif

template <typename T>
constexpr T Clamp(const T& value, const T& min_val, const T& max_val) {
    return (value < min_val) ? min_val : (value > max_val) ? max_val : value;
}

template<typename T>
constexpr T Proportion(T x_min, T x_max, T y_min, T y_max, T x) {
    if (x_min == x_max) return y_min;
    return y_min + (x - x_min) * (y_max - y_min) / (x_max - x_min);
}

// ==== Conversion ====
namespace Convert {

static inline uint16_t BuildU16(uint16_t bLsb, uint16_t bMsb) {
    return (bMsb << 8) | bLsb;
}

static inline uint32_t BuildU132(uint32_t bLsb, uint32_t bL1, uint32_t bM1, uint32_t bMsb) {
    return (bMsb << 24) | (bM1 << 16) | (bL1 << 8) | bLsb;
}

} // namespace