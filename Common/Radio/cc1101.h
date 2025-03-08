/*
 * cc1101.h
 *
 *  Created on: Feb 12, 2013
 *      Author: g.kruglov
 */

#ifndef CC1101_H__
#define CC1101_H__

#include <inttypes.h>
#include "kl_lib.h"
#include "cc1101defins.h"
#include "cc1101_rf_settings.h"

#define CC_BUSYWAIT_TIMEOUT     99000   // tics, not ms

void CCIrqHandler();

class cc1101_t : public IrqHandler_t {
private:
    const Spi_t ispi;
    const GPIO_TypeDef *spi_gpio, *cs_gpio;
    const uint16_t sck_pin, miso_pin, mosi_pin, cs_pin;
    const PinIrq_t igdo0;
    uint8_t istate; // Inner CC state, returned as first byte
    thread_reference_t thd_ref;
    volatile ftVoidVoid icallback = nullptr;
    // Pins
    retv BusyWait() {
        for(uint32_t i=0; i<CC_BUSYWAIT_TIMEOUT; i++) {
            if(PinIsLo(spi_gpio, miso_pin)) return retv::Ok;
        }
        return retv::Fail;
    }
    void CsHi() { PinSetHi((GPIO_TypeDef*)cs_gpio, cs_pin); }
    void CsLo() { PinSetLo((GPIO_TypeDef*)cs_gpio, cs_pin); }
    // General
    int8_t RSSI_dBm(uint8_t araw_rssi);
    // Registers and buffers
    retv WriteRegister(const uint8_t addr, const uint8_t adata);
    retv ReadRegister(const uint8_t addr, uint8_t *pdata);
    retv WriteStrobe(uint8_t astrobe);
    retv WriteTX(uint8_t* ptr, uint8_t sz);
    // Strobes
    retv Reset()       { return WriteStrobe(CC_SRES); }
    retv EnterTX()     { return WriteStrobe(CC_STX);  }
    retv EnterRX()     { return WriteStrobe(CC_SRX);  }
    retv FlushRxFIFO() { return WriteStrobe(CC_SFRX); }
    retv FlushTxFIFO() { return WriteStrobe(CC_SFTX); }
    retv GetStatus()   { return WriteStrobe(CC_SNOP); }
public:
    retv Init();
    retv EnterIdle()    { return WriteStrobe(CC_SIDLE); }
    retv EnterPwrDown() { return WriteStrobe(CC_SPWD);  }
    void SetChannel(uint8_t achnl);
    void SetTxPower(uint8_t apwr)  { WriteRegister(CC_PATABLE, apwr); }
    void SetPktSize(uint8_t sz) { WriteRegister(CC_PKTLEN, sz); }
    void SetBitrate(const CCRegValue_t* bitrate_setup);
    // State change
    void TransmitAsyncX(uint8_t *ptr, uint8_t sz, ftVoidVoid acallback);
    void TransmitCcaX(uint8_t *ptr, uint8_t sz, ftVoidVoid acallback);
    void TransmitAsyncX(uint8_t *ptr, uint8_t sz);
    void Transmit(uint8_t *ptr, uint8_t sz);
    retv Receive(uint32_t timeout_ms, uint8_t *ptr, uint8_t sz,  int8_t *prssi=nullptr);
    retv Receive_st(sysinterval_t timeout_st, uint8_t *ptr, uint8_t sz,  int8_t *prssi=nullptr);
    void ReceiveAsync(ftVoidVoid acallback);
    void ReceiveAsyncI(ftVoidVoid acallback);

    uint8_t RxCcaTx_st(uint8_t *ptr, uint8_t sz,  int8_t *prssi=nullptr);
    uint8_t RxIfNotYet_st(sysinterval_t rx_timeout_st, uint8_t *ptr, uint8_t sz,  int8_t *prssi=nullptr);

    void PowerOff();
    retv Recalibrate() {
        do {
            if(EnterIdle() != retv::Ok) return retv::Fail;
        } while(istate != CC_STB_IDLE);
        if(WriteStrobe(CC_SCAL) != retv::Ok) return retv::Fail;
        do {
            GetStatus();
        } while(istate != CC_STB_IDLE);
        return retv::Ok;
    }

    void PrintStateI();

    // Setup
    void DoRxAfterRxAndRxAfterTx()   { WriteRegister(CC_MCSM1, (CC_MCSM1_VALUE | 0x0F)); }
    void DoRxAfterRxAndIdleAfterTx() { WriteRegister(CC_MCSM1, ((CC_MCSM1_VALUE | 0x0C) & 0xFC)); }
    void DoRxAfterTx()   { WriteRegister(CC_MCSM1, (CC_MCSM1_VALUE | 0x03)); }
    void DoIdleAfterTx() { WriteRegister(CC_MCSM1, CC_MCSM1_VALUE); }

    retv ReadFIFO(uint8_t *p, int8_t *prssi, uint8_t sz);

    void IIrqHandler();

    cc1101_t(
            SPI_TypeDef *aspi, GPIO_TypeDef *aspi_gpio,
            uint16_t asck, uint16_t amiso, uint16_t amosi,
            GPIO_TypeDef *acs_gpio, uint16_t acs,
            GPIO_TypeDef *agd0_gpio, uint16_t agdo0_pin):
        ispi(aspi), spi_gpio(aspi_gpio), cs_gpio(acs_gpio),
        sck_pin(asck), miso_pin(amiso), mosi_pin(amosi), cs_pin(acs),
        igdo0(agd0_gpio, agdo0_pin, pudNone, CCIrqHandler),
        istate(0), thd_ref(nullptr) {}
};

#define DELAY_LOOP_34uS()  { for(volatile uint32_t i=0; i<12; i++); } // 12 leads to 34us @ 4MHz sys clk
#define DELAY_LOOP_144uS() { for(volatile uint32_t i=0; i<54; i++); } // 54 leads to 144us @ 4MHz sys clk

#endif //CC1101_H__
