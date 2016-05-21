#include "L3G.h"
#include "kl_lib.h"
#include "uart.h"

extern i2c_t i2cimu;

// Defines ////////////////////////////////////////////////////////////////
#define L3G4200D_ADDRESS_SA0_LOW (0xD0 >> 1)
#define L3G4200D_ADDRESS_SA0_HIGH (0xD2 >> 1)
#define L3GD20_ADDRESS_SA0_LOW (0xD4 >> 1)
#define L3GD20_ADDRESS_SA0_HIGH (0xD6 >> 1)

// Public Methods //////////////////////////////////////////////////////////////

bool L3G::init(uint8_t device, uint8_t sa0) {
    _device = device;
    switch(_device) {
        case L3G4200D_DEVICE:
            if(sa0 == L3G_SA0_LOW) {
                address = L3G4200D_ADDRESS_SA0_LOW;
                return true;
            }
            else if(sa0 == L3G_SA0_HIGH) {
                address = L3G4200D_ADDRESS_SA0_HIGH;
                return true;
            }
            else return autoDetectAddress();
            break;

        case L3GD20_DEVICE:
            if(sa0 == L3G_SA0_LOW) {
                address = L3GD20_ADDRESS_SA0_LOW;
                return true;
            }
            else if (sa0 == L3G_SA0_HIGH) {
                address = L3GD20_ADDRESS_SA0_HIGH;
                return true;
            }
            else return autoDetectAddress();
            break;

        default: return autoDetectAddress();
    }
}

// Turns on the L3G's gyro and places it in normal mode.
void L3G::enableDefault(void) {
    // 0x0F = 0b00001111
    // Normal power mode, all axes enabled
    writeReg(L3G_CTRL_REG1, 0x0F);
}

// Writes a gyro register
void L3G::writeReg(uint8_t reg, uint8_t value) {
    uint8_t Buf[2];
    Buf[0] = reg;
    Buf[1] = value;
//    uint8_t Rslt =
    i2cimu.WriteWrite(address, Buf, 2, nullptr, 0);
//    Uart.Printf("WReg: %X; %X; r=%u\r\n", reg, value, Rslt);
}

// Reads a gyro register
uint8_t L3G::readReg(uint8_t reg) {
//    Uart.Printf("addr=%X\r\n", address);
    uint8_t value = 0;
//    uint8_t Rslt =
    i2cimu.WriteRead(address, &reg, 1, &value, 1);
//    Uart.Printf("RReg=%X; %X; r=%u\r\n", reg, value, Rslt);
    return value;
}

// Reads the 3 gyro channels and stores them in vector g
void L3G::read(int16_t *pX, int16_t *pY, int16_t *pZ) {
    uint16_t Buf[3];
    uint8_t reg = L3G_OUT_X_L | (1 << 7);
//    uint8_t Rslt =
    i2cimu.WriteRead(address, &reg, 1, (uint8_t*) &Buf, 6);

    *pX = Buf[0];
    *pY = Buf[1];
    *pZ = Buf[2];
//    Uart.Printf("Read: %u; %u; %u; r=%u\r\n", Buf[0], Buf[1], Buf[2], Rslt);
}

// Private Methods //////////////////////////////////////////////////////////////

bool L3G::autoDetectAddress(void) {
    // try each possible address and stop if reading WHO_AM_I returns the expected response
    address = L3G4200D_ADDRESS_SA0_LOW;
    if (readReg(L3G_WHO_AM_I) == 0xD3) return true;
    address = L3G4200D_ADDRESS_SA0_HIGH;
    if (readReg(L3G_WHO_AM_I) == 0xD3) return true;
    address = L3GD20_ADDRESS_SA0_LOW;
    if (readReg(L3G_WHO_AM_I) == 0xD4 || readReg(L3G_WHO_AM_I) == 0xD7) return true;
    address = L3GD20_ADDRESS_SA0_HIGH;
    if (readReg(L3G_WHO_AM_I) == 0xD4 || readReg(L3G_WHO_AM_I) == 0xD7) return true;
    return false;
}
