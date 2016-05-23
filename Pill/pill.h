/*
 * pill.h
 *
 *  Created on: 22 мая 2014 г.
 *      Author: g.kruglov
 */

#ifndef PILL_H_
#define PILL_H_

enum PillType_t {
    ptSetID = 1,
    ptCure = 9,
    ptDrug = 10,
    ptPanacea = 11,
    ptAutodoc = 12,
    ptSetDoseTop = 18,
    ptDiagnostic = 27,
    ptEmpBreaker = 31,
    ptEmpRepair = 32,
    ptElectrostation = 33,
    ptSetType = 99,
    ptUnknown = 108
};

struct Pill_t {
    union { // Type
        int32_t TypeInt32;      // offset 0
        PillType_t Type;// __attribute__((aligned(4)));    // offset 0
    };
    // Contains dose value after pill application
    int32_t DoseAfter;          // Offset 4
    union {
        int32_t DeviceID;       // offset 8
        // Cure / drug
        struct {
            int32_t ChargeCnt;  // offset 8
            int32_t Value;      // offset 12
        };
        // Set DoseTop / Diagnostic
        int32_t DoseTop;        // offset 8
        int32_t DeviceType;     // offset 8
        int32_t DeviceCapacity; // offset 8
        // Grenade's capacity
        uint32_t Capacity;      // offset 8
        // EmpMech's time to repair
        uint32_t TimeToRepair;  // offset 8
    }; // union
    int32_t Time_s;             // offset 16
} __attribute__ ((__packed__));
#define PILL_SZ     sizeof(Pill_t)
#define PILL_SZ32   (sizeof(Pill_t) / sizeof(int32_t))

// Inner Pill addresses
#define PILL_TYPE_ADDR      0
#define PILL_DOSEAFTER_ADDR 4
#define PILL_CHARGECNT_ADDR 8
#define PILL_DOSETOP_ADDR   8
#define PILL_VALUE_ADDR     12

// Data to save in EE and to write to pill
struct DataToWrite_t {
    uint32_t Sz32;
    int32_t Data[PILL_SZ32];
};

#endif /* PILL_H_ */
