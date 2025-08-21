/*
 * cmd_uart.h
 *
 *  Created on: 15.04.2013
 *      Author: kreyl
 */

#ifndef UART_H__
#define UART_H__

#include "kl_lib.h"
#include <cstring>
#include "shell.h"
#include "board.h"

extern "C"
void DmaUartTxIrq(void *p, uint32_t flags);

struct UartParams_t {
    uint32_t Baudrate;
    USART_TypeDef* puart;
    GPIO_TypeDef *PGpioTx;
    uint16_t PinTx;
    GPIO_TypeDef *PGpioRx;
    uint16_t PinRx;
    // DMA
    uint32_t DmaTxID, DmaRxID;
    uint32_t DmaModeTx, DmaModeRx;
    // MCU-specific
    UartParams_t(uint32_t ABaudrate, USART_TypeDef* AUart,
            GPIO_TypeDef *APGpioTx, uint16_t APinTx,
            GPIO_TypeDef *APGpioRx, uint16_t APinRx,
            uint32_t ADmaTxID, uint32_t ADmaRxID,
            uint32_t ADmaModeTx, uint32_t ADmaModeRx
    ) : Baudrate(ABaudrate), puart(AUart),
            PGpioTx(APGpioTx), PinTx(APinTx), PGpioRx(APGpioRx), PinRx(APinRx),
            DmaTxID(ADmaTxID), DmaRxID(ADmaRxID),
            DmaModeTx(ADmaModeTx), DmaModeRx(ADmaModeRx)
    {}
};

// ==== Base class ====
class BaseUart {
protected:
    const stm32_dma_stream_t *PDmaTx = nullptr;
    const stm32_dma_stream_t *PDmaRx = nullptr;
    const UartParams_t *Params;
    char TXBuf[UART_TXBUF_SZ];
    char *PRead, *PWrite;
    bool IDmaIsIdle;
    uint32_t IFullSlotsCount, ITransSize;
    void ISendViaDMA();
    int32_t OldWIndx, RIndx;
    uint8_t IRxBuf[UART_RXBUF_SZ];
protected:
    retv IPutByte(uint8_t b);
    retv IPutByteNow(uint8_t b);
    void IStartTransmissionIfNotYet();
    // ==== Constructor ====
    BaseUart(const UartParams_t *aparams) : Params(aparams)
    , PRead(TXBuf), PWrite(TXBuf), IDmaIsIdle(true), IFullSlotsCount(0), ITransSize(0)
    , OldWIndx(0), RIndx(0)
    {}
public:
    void Init();
    void Shutdown();
    void OnClkChange();
    // Enable/Disable
    void EnableTx()  { Params->puart->CR1 |= USART_CR1_TE; }
    void DisableTx() { Params->puart->CR1 &= ~USART_CR1_TE; }
    void EnableRx()  { Params->puart->CR1 |= USART_CR1_RE; }
    void DisableRx() { Params->puart->CR1 &= ~USART_CR1_RE; }
    void FlushTx() { while(!IDmaIsIdle) chThdSleepMilliseconds(1); }  // wait DMA
    void EnableTCIrq(const uint32_t Priority, ftVoidVoid ACallback);
    // Inner use
    void IRQDmaTxHandler();
    retv GetByte(uint8_t *b);
};

class CmdUart : public BaseUart, public Shell {
private:
    retv IPutChar(char c) { return IPutByte(c);  }
    void IStartTransmissionIfNotYet() { BaseUart::IStartTransmissionIfNotYet(); }
public:
    CmdUart(const UartParams_t *aparams) : BaseUart(aparams) {}
    retv TryParseRxBuff() {
        uint8_t b;
        while(GetByte(&b) == retv::Ok) {
            if(cmd.PutChar(b) == pdrNewCmd) return retv::Ok;
        } // while get byte
        return retv::Fail;
    }
};

#endif //UART_H__
