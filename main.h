#pragma once

#define ID_MIN                  1
#define ID_MAX                  15
#define ID_DEFAULT              ID_MIN

extern int32_t ID;

enum PillType_t {
    pilltNone = 0,
    pilltParalyzer = 1,
    pilltRegen = 2,
    pilltAccelerator = 3,
    pilltLight = 4,
    pilltBlow = 5
};

enum DevType_t {
    devtNone = 0,
    devtParalyzer = 1,
    devtRegen = 2,
    devtAccelerator = 3,
    devtLight = 4,
    devtMaster = 9
};
