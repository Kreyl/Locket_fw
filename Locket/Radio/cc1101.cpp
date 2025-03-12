/*
 * cc1101.cpp
 *
 *  Created on: Feb 12, 2013
 *      Author: g.kruglov
 */

#include "cc1101.h"
#include "uart.h"

#define CC_MAX_BAUDRATE_HZ  6500000

extern cc1101_t CC;

void CCIrqHandler() { CC.IIrqHandler(); }

retv cc1101_t::Init() {
    // ==== GPIO ====
#if defined STM32L1XX || defined STM32F4XX || defined STM32L4XX || defined STM32F2XX
    AlterFunc_t CC_AF;
    if(ispi.PSpi == SPI1 or ispi.PSpi == SPI2) CC_AF = AF5;
    else CC_AF = AF6;
#elif defined STM32F030 || defined STM32F0 ||defined STM32F1XX
#define CC_AF   AF0
#endif
    PinSetupOut      ((GPIO_TypeDef*)cs_gpio,  cs_pin,   omPushPull);
    PinSetupAlterFunc((GPIO_TypeDef*)spi_gpio, sck_pin,  omPushPull, pudNone, CC_AF);
    PinSetupAlterFunc((GPIO_TypeDef*)spi_gpio, miso_pin, omPushPull, pudNone, CC_AF);
    PinSetupAlterFunc((GPIO_TypeDef*)spi_gpio, mosi_pin, omPushPull, pudNone, CC_AF);
    igdo0.Init(ttFalling);

    CsHi();
    // ==== SPI ====
    // MSB first, master, ClkLowIdle, FirstEdge, Baudrate no more than 6.5MHz
    ispi.Setup(boMSB, cpolIdleLow, cphaFirstEdge, CC_MAX_BAUDRATE_HZ);
    ispi.Enable();
    // ==== Init CC ====
    if(Reset() != retv::Ok) {
        ispi.Disable();
        Printf("CC Rst Fail\r");
        return retv::Fail;
    }
    // Check if Write/Read ok
    if(WriteRegister(CC_PKTLEN, 7) != retv::Ok) {
        Printf("CC W Fail\r");
        return retv::Fail;
    }
    uint8_t b = 0;
    if(ReadRegister(CC_PKTLEN, &b) == retv::Ok) {
        if(b != 7) {
            Printf("CC R/W Fail; rpl=%u\r", b);
            return retv::Fail;
        }
    }
    else {
        Printf("CC R Fail\r");
        return retv::Fail;
    }
    // Proceed with init
    FlushRxFIFO();
    // Common regs
    WriteRegister(CC_FREQ2,    CC_FREQ2_VALUE);      // Frequency control word, high byte.
    WriteRegister(CC_FREQ1,    CC_FREQ1_VALUE);      // Frequency control word, middle byte.
    WriteRegister(CC_FREQ0,    CC_FREQ0_VALUE);      // Frequency control word, low byte.
    WriteRegister(CC_MDMCFG1,  CC_MDMCFG1_VALUE);    // Modem configuration.
    WriteRegister(CC_MDMCFG0,  CC_MDMCFG0_VALUE);    // Modem configuration.
    WriteRegister(CC_MCSM0,    CC_MCSM0_VALUE);      // Main Radio Control State Machine configuration.
    WriteRegister(CC_FIFOTHR,  CC_FIFOTHR_VALUE);    // fifo threshold
    WriteRegister(CC_IOCFG2,   CC_IOCFG2_VALUE);     // GDO2 output pin configuration.
    WriteRegister(CC_IOCFG0,   CC_IOCFG0_VALUE);     // GDO0 output pin configuration.
    WriteRegister(CC_PKTCTRL1, CC_PKTCTRL1_VALUE);   // Packet automation control.
    WriteRegister(CC_PKTCTRL0, CC_PKTCTRL0_VALUE);   // Packet automation control.
    WriteRegister(CC_PATABLE, CC_Pwr0dBm);
    WriteRegister(CC_MCSM2, CC_MCSM2_VALUE);
    WriteRegister(CC_MCSM1, CC_MCSM1_VALUE);

    igdo0.EnableIrq(IRQ_PRIO_HIGH);
    Printf("CC init ok\r");
    return retv::Ok;
}

#if 1 // ======================= TX, RX, freq and power ========================
void cc1101_t::PowerOff() {
    while(istate != CC_STB_IDLE) EnterIdle();
    EnterPwrDown();
}

void cc1101_t::PrintStateI() {
    GetStatus();
    PrintfI("0x%02X\r", istate);
}

void cc1101_t::SetChannel(uint8_t achannel) {
    while(istate != CC_STB_IDLE) EnterIdle();   // CC must be in IDLE mode
    WriteRegister(CC_CHANNR, achannel);         // Now set channel
}

void cc1101_t::SetBitrate(const CCRegValue_t* bitrate_setup) {
    while(istate != CC_STB_IDLE) EnterIdle();   // CC must be in IDLE mode
    for(int i=0; i<CC_BRSETUP_CNT; i++) {
        WriteRegister(bitrate_setup[i].Reg, bitrate_setup[i].Value);
    }
}

void cc1101_t::TransmitAsyncX(uint8_t *ptr, uint8_t sz, ftVoidVoid callback) {
    EnterTX();
#if CC_CCA_MODE != 0
    // if prev state == RX, ClearChannelCheck is applied.
    if(istate == CC_STB_RX) {
        DELAY_LOOP_34uS(); // Wait >30 us
        GetStatus();
        if(istate != CC_STB_TX) return; // TX not entered
    }
#endif
    icallback = callback;
    WriteTX(ptr, sz);
}

void cc1101_t::TransmitCcaX(uint8_t *ptr, uint8_t sz, ftVoidVoid callback) {
    GetStatus();
    if(istate != CC_STB_RX) {
        EnterRX(); // if prev state == RX, ClearChannelCheck is applied.
        DELAY_LOOP_144uS(); // 100...150 uS for 250kBaud
    }
    EnterTX();
    DELAY_LOOP_34uS(); // Wait >30 us
    GetStatus();
    if(istate == CC_STB_TX) { // TX entered
        icallback = callback;
        WriteTX(ptr, sz);
    }
    else CC.EnterIdle();
}

void cc1101_t::TransmitAsyncX(uint8_t *ptr, uint8_t sz) {
    EnterTX();  // Start transmission of preamble while writing FIFO
    WriteTX(ptr, sz);
}

void cc1101_t::Transmit(uint8_t *ptr, uint8_t sz) {
    EnterTX();  // Start transmission of preamble while writing FIFO
    chSysLock();
    icallback = nullptr;
    WriteTX(ptr, sz);
    // Enter TX and wait IRQ
    chThdSuspendS(&thd_ref); // Wait IRQ
    chSysUnlock();          // Will be here when IRQ fires
}

// Enter RX mode and wait reception for timeout_ms.
retv cc1101_t::Receive(uint32_t timeout_ms, uint8_t *ptr, uint8_t sz, int8_t *prssi) {
    return Receive_st(TIME_MS2I(timeout_ms), ptr, sz, prssi);
}

retv cc1101_t::Receive_st(sysinterval_t timeout_st, uint8_t *ptr, uint8_t sz, int8_t *prssi) {
    FlushRxFIFO();
    chSysLock();
    EnterRX();
    msg_t Rslt = chThdSuspendTimeoutS(&thd_ref, timeout_st);    // Wait IRQ
    chSysUnlock();  // Will be here when IRQ will fire, or timeout occur - with appropriate message

    if(Rslt == MSG_TIMEOUT) {   // Nothing received, timeout occured
        EnterIdle();            // Get out of RX mode
        return retv::Timeout;
    }
    else return ReadFIFO(ptr, prssi, sz);
}

void cc1101_t::ReceiveAsyncI(ftVoidVoid callback) {
    GetStatus();
    if(istate != CC_STB_RX) { // Not in RX
        EnterIdle();
        FlushRxFIFO();
        EnterRX();
    }
    icallback = callback;
}

/*
void cc1101_t::ReceiveAsync(ftVoidVoid callback) {
    chSysLock();
    GetStatus();
    if(istate != CC_STB_RX) { // Not in RX
        EnterIdle();
        FlushRxFIFO();
        EnterRX();
    }
    icallback = callback;
    chSysUnlock();
}

uint8_t cc1101_t::RxCcaTx_st(uint8_t *PtrTx, uint8_t sz,  int8_t *prssi) {
    chSysLock();
    icallback = nullptr;
    // Enter RX if not yet
    GetStatus();
//    PrintfI("S1: %X\r", istate);
    if(istate != CC_STB_RX) { // Not in RX
        EnterRX();
        chThdSleepS(TIME_US2I(117)); // 100...150 uS for 250kBaud
    }
    EnterTX();
    // Where we are?
    chThdSleepS(TIME_US2I(108));
    GetStatus();
//    PrintfI("S2: %X\r", istate);
    if(istate != CC_STB_TX) { // Tx entered
        WriteTX((uint8_t*)PtrTx, sz);
        chThdSuspendS(&thd_ref); // Wait IRQ
        chSysUnlock();          // Will be here when IRQ fires
        return retv::Ok;
    }
    else {
        chSysUnlock();
        return retv::Fail;
    }
}

uint8_t cc1101_t::RxIfNotYet_st(sysinterval_t RxTimeout_st, uint8_t *PtrRx, uint8_t sz,  int8_t *prssi) {
    // Enter RX if not yet
    chSysLock();
    GetStatus();
//    PrintfI("S3: %X\r", istate);
    if(istate != CC_STB_RX) { // Not in RX
        EnterIdle();
        FlushRxFIFO();
        EnterRX();
    }
    msg_t Rslt = chThdSuspendTimeoutS(&thd_ref, RxTimeout_st); // Wait IRQ
    chSysUnlock();
    if(Rslt == MSG_TIMEOUT) return retv::Fail; // No IRQ occured
    else { // IRQ fired
        return ReadFIFO(PtrRx, prssi, sz);
    }
}
*/
// Return rssi in dBm
int8_t cc1101_t::RSSI_dBm(uint8_t araw_rssi) {
    int16_t rssi = araw_rssi;
    if (rssi >= 128) rssi -= 256;
    rssi = (rssi / 2) - 74;    // now it is in dBm
    return rssi;
}
#endif

#if 1 // ======================== Registers & Strobes ==========================
retv cc1101_t::ReadRegister (uint8_t areg_addr, uint8_t *pdata) {
    CsLo();                       // Start transmission
    if(BusyWait() != retv::Ok) {  // Wait for chip to become ready
        CsHi();
        return retv::Fail;
    }
    istate = ispi.ReadWriteByte(areg_addr | CC_READ_FLAG) & 0b01110000; // Transmit header byte
    *pdata = ispi.ReadWriteByte(0); // Read reply
    CsHi();                         // End transmission
    return retv::Ok;
}
retv cc1101_t::WriteRegister (uint8_t areg_addr, uint8_t AData) {
    CsLo();                      // Start transmission
    if(BusyWait() != retv::Ok) { // Wait for chip to become ready
        CsHi();
        return retv::Fail;
    }
    ispi.ReadWriteByte(areg_addr);  // Transmit header byte
    ispi.ReadWriteByte(AData);      // Write data
    CsHi();                         // End transmission
    return retv::Ok;
}
retv cc1101_t::WriteStrobe (uint8_t AStrobe) {
    CsLo();                      // Start transmission
    if(BusyWait() != retv::Ok) { // Wait for chip to become ready
        CsHi();
        return retv::Fail;
    }
    istate = ispi.ReadWriteByte(AStrobe); // Write strobe
    CsHi();                               // End transmission
    istate &= 0b01110000;                 // Mask needed bits
    return retv::Ok;
}

retv cc1101_t::WriteTX(uint8_t* ptr, uint8_t Length) {
    CsLo();                      // Start transmission
    if(BusyWait() != retv::Ok) { // Wait for chip to become ready
        CsHi();
        return retv::Fail;
    }
    ispi.ReadWriteByte(CC_FIFO|CC_WRITE_FLAG|CC_BURST_FLAG); // Address with write & burst flags
//    Printf("TX: ");
    for(uint8_t i=0; i<Length; i++) {
        uint8_t b = *ptr++;
        ispi.ReadWriteByte(b);  // Write bytes
//        Printf("%X ", b);
    }
    CsHi();    // End transmission
//    Printf("\r");
    return retv::Ok;
}

retv cc1101_t::ReadFIFO(uint8_t *p, int8_t *prssi, uint8_t sz) {
    uint8_t b;
     // Check if received successfully
     if(ReadRegister(CC_PKTSTATUS, &b) != retv::Ok) return retv::Fail;
//     PrintfI("PktSt: %02X\r", b);
     if(b & 0x80) {  // CRC OK
         // Read FIFO
         CsLo();
         if(BusyWait() != retv::Ok) { // Wait for chip to become ready
             CsHi();
             return retv::Fail;
         }
         ispi.ReadWriteByte(CC_FIFO|CC_READ_FLAG|CC_BURST_FLAG); // Address with read & burst flags
         for(uint8_t i=0; i<sz; i++) { // Read bytes
             b = ispi.ReadWriteByte(0);
             *p++ = b;
             // Uart.Printf(" %X", b);
         }
         // Receive two additional info bytes
         b = ispi.ReadWriteByte(0); // rssi
         ispi.ReadWriteByte(0);     // LQI
         CsHi();                    // End transmission
         if(prssi != nullptr) *prssi = RSSI_dBm(b);
         return retv::Ok;
     }
     else return retv::Fail;
}
#endif

void cc1101_t::IIrqHandler() {
//    PrintfI("i %X\r", icallback);
    if(icallback != nullptr) {
        icallback();
//        icallback = nullptr;
    }
    else chThdResumeI(&thd_ref, MSG_OK);  // NotNull check performed inside chThdResumeI
}
