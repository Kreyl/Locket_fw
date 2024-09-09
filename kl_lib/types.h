/*
 * retval.h
 *
 *  Created on: 29 авг. 2023 г.
 *      Author: laurelindo
 */

#ifndef LIB_TYPES_H_
#define LIB_TYPES_H_

#include <inttypes.h>

// ==== Return values ====
enum class retv : uint8_t {
    Ok =            0,
    Fail =          1,
    Timeout =       2,
    Reset =         3,
    Busy =          4,
    InProgress =    5,
    CmdError =      6,
    CmdUnknown =    7,
    BadValue =      8,
    New =           9,
    Same =          10,
    Last =          11,
    Empty =         12,
    Overflow =      13,
    NotANumber =    14,
    WriteProtect =  15,
    WriteError =    16,
    EndOfFile =     17,
    NotFound =      18,
    BadState =      19,
    Disconnected =  20,
    Collision =     21,
    CRCError =      22,
    NACK =          23,
    NoAnswer =      24,
    OutOfMemory =   25,
    NotAuthorised = 26,
    NoChanges =     27,
};

/* ==== Retval with some payload ====
Example:
using CO2Rslt_t = StatusOr<SnsData_t>;
SCD4x_t::CO2Rslt_t r = Co2Sns.read_measurement();
if(r.Ok()) Printf("%d %d %d\r", r->CO2, r->Temp, r->RH);
*/

template <typename T>
struct StatusOr {
    StatusOr() : rslt(retv::Ok) {}
    StatusOr(retv arslt) : rslt(arslt) {}
    StatusOr(retv arslt, T av) : rslt(arslt) { v = av; }
    retv rslt;
    T v;
    bool Ok() { return rslt == retv::Ok; }
    T* operator ->() { return &v; }
    T& operator *() { return v; }
};

using StatusOrU8  = StatusOr<uint8_t>;
using StatusOrU16 = StatusOr<uint16_t>;
using StatusOrU32 = StatusOr<uint32_t>;
using StatusOrPChar = StatusOr<char*>;

// ==== Functional types ====
typedef void (*ftVoidVoid)(void);
typedef retv (*ftRetvVoid_t)(void);
typedef void (*ftVoidU8)(uint8_t);
typedef void (*ftVoidU32)(uint32_t);
typedef void (*ftVoidPVoid)(void*p);
typedef void (*ftVoidPVoidW32)(void*p, uint32_t W32);

enum Inverted_t {invNotInverted, invInverted};
enum class BitOrder {MSB, LSB};
enum class RiseFall {None, Rising, Falling, Both};

// ==== Conversion ====
namespace Convert {

static inline uint16_t BuildU16(uint16_t bLsb, uint16_t bMsb) {
    return (bMsb << 8) | bLsb;
}

static inline uint32_t BuildU132(uint32_t bLsb, uint32_t bL1, uint32_t bM1, uint32_t bMsb) {
    return (bMsb << 24) | (bM1 << 16) | (bL1 << 8) | bLsb;
}

} // namespace

#endif /* LIB_TYPES_H_ */
