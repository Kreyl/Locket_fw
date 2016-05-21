#ifndef L3G_h
#define L3G_h

#include "inttypes.h"

// device types

#define L3G_DEVICE_AUTO 0
#define L3G4200D_DEVICE 1
#define L3GD20_DEVICE 2


// SA0 states

#define L3G_SA0_LOW 0
#define L3G_SA0_HIGH 1
#define L3G_SA0_AUTO 2

// register addresses

#define L3G_WHO_AM_I 0x0F

#define L3G_CTRL_REG1 0x20
#define L3G_CTRL_REG2 0x21
#define L3G_CTRL_REG3 0x22
#define L3G_CTRL_REG4 0x23
#define L3G_CTRL_REG5 0x24
#define L3G_REFERENCE 0x25
#define L3G_OUT_TEMP 0x26
#define L3G_STATUS_REG 0x27

#define L3G_OUT_X_L 0x28
#define L3G_OUT_X_H 0x29
#define L3G_OUT_Y_L 0x2A
#define L3G_OUT_Y_H 0x2B
#define L3G_OUT_Z_L 0x2C
#define L3G_OUT_Z_H 0x2D

#define L3G_FIFO_CTRL_REG 0x2E
#define L3G_FIFO_SRC_REG 0x2F

#define L3G_INT1_CFG 0x30
#define L3G_INT1_SRC 0x31
#define L3G_INT1_THS_XH 0x32
#define L3G_INT1_THS_XL 0x33
#define L3G_INT1_THS_YH 0x34
#define L3G_INT1_THS_YL 0x35
#define L3G_INT1_THS_ZH 0x36
#define L3G_INT1_THS_ZL 0x37
#define L3G_INT1_DURATION 0x38
#define L3G_LOW_ODR 0x39

class L3G
{
  public:
    bool init(uint8_t device = L3G_DEVICE_AUTO, uint8_t sa0 = L3G_SA0_AUTO);

    void enableDefault(void);

    void writeReg(uint8_t reg, uint8_t value);
    uint8_t readReg(uint8_t reg);

    void read(int16_t *pX, int16_t *pY, int16_t *pZ);


  private:
      uint8_t _device; // chip type (4200D or D20)
      uint8_t address;

      bool autoDetectAddress(void);
};

#endif
