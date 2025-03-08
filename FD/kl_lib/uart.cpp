/*
 * cmd_uart.cpp
 *
 *  Created on: 15.04.2013
 *      Author: kreyl
 */

#include "MsgQ.h"
#include <string.h>
#include "uart.h"
#include "kl_lib.h"

#if 1 // ==================== Common and eternal ===============================
// Pins Alternate function
#if defined STM32L4XX || defined STM32F0XX
#define UART_TX_REG     TDR
#define UART_RX_REG     RDR
#elif defined STM32L1XX || defined STM32F2XX || defined STM32F1XX
#define UART_TX_REG     DR
#define UART_RX_REG     DR
#else
#error "Not defined"
#endif

#endif // Common and eternal

// Wrapper for TX IRQ
extern "C"
void DmaUartTxIrq(void *p, uint32_t flags) { ((BaseUart*)p)->IRQDmaTxHandler(); }

// ==== TX DMA IRQ ====
void BaseUart::IRQDmaTxHandler() {
    dmaStreamDisable(PDmaTx);    // Registers may be changed only when stream is disabled
    IFullSlotsCount -= ITransSize;
    PRead += ITransSize;
    if(PRead >= (TXBuf + UART_TXBUF_SZ)) PRead = TXBuf; // Circulate pointer
    if(IFullSlotsCount == 0) IDmaIsIdle = true; // Nothing left to send
    else ISendViaDMA();
}

void BaseUart::ISendViaDMA() {
    uint32_t PartSz = (TXBuf + UART_TXBUF_SZ) - PRead; // Cnt from PRead to end of buf
    ITransSize = MIN_(IFullSlotsCount, PartSz);
    if(ITransSize != 0) {
        IDmaIsIdle = false;
        dmaStreamSetMemory0(PDmaTx, PRead);
        dmaStreamSetTransactionSize(PDmaTx, ITransSize);
        dmaStreamSetMode(PDmaTx, Params->DmaModeTx);
        dmaStreamEnable(PDmaTx);
    }
}

retv BaseUart::IPutByte(uint8_t b) {
    if(IFullSlotsCount >= UART_TXBUF_SZ) return retv::Overflow;
    *PWrite++ = b;
    if(PWrite >= &TXBuf[UART_TXBUF_SZ]) PWrite = TXBuf;   // Circulate buffer
    IFullSlotsCount++;
    return retv::Ok;
}

void BaseUart::IStartTransmissionIfNotYet() {
    if(IDmaIsIdle) ISendViaDMA();
}

retv BaseUart::IPutByteNow(uint8_t b) {
    while(!(Params->puart->SR & USART_SR_TXE));
    Params->puart->UART_TX_REG = b;
    while(!(Params->puart->SR & USART_SR_TXE));
    return retv::Ok;
}


retv BaseUart::GetByte(uint8_t *b) {
    int32_t WIndx = UART_RXBUF_SZ - PDmaRx->channel->CNDTR;
    int32_t BytesCnt = WIndx - RIndx;
    if(BytesCnt < 0) BytesCnt += UART_RXBUF_SZ;
    if(BytesCnt == 0) return retv::Empty;
    *b = IRxBuf[RIndx++];
    if(RIndx >= UART_RXBUF_SZ) RIndx = 0;
    return retv::Ok;
}


void BaseUart::Init() {
    PinSetupAlterFunc(Params->PGpioTx, Params->PinTx, omPushPull, pudNone, AF7);
    // ==== Clock ====
    if     (Params->puart == USART1) { rccEnableUSART1(FALSE); }
    else if(Params->puart == USART2) { rccEnableUSART2(FALSE); }
    else if(Params->puart == USART3) { rccEnableUSART3(FALSE); }
    OnClkChange();  // Setup baudrate

    Params->puart->CR2 = 0;  // Nothing that interesting there
    PDmaTx = dmaStreamAlloc(Params->DmaTxID, IRQ_PRIO_MEDIUM, DmaUartTxIrq, this);
    dmaStreamSetPeripheral(PDmaTx, &Params->puart->UART_TX_REG);
    dmaStreamSetMode      (PDmaTx, Params->DmaModeTx);
    IDmaIsIdle = true;

    // ==== RX ====
    Params->puart->CR1 = USART_CR1_TE | USART_CR1_RE;        // TX & RX enable
    Params->puart->CR3 = USART_CR3_DMAT | USART_CR3_DMAR;    // Enable DMA at TX & RX
    // ==== Rx pin ====
    PinSetupAlterFunc(Params->PGpioRx, Params->PinRx, omOpenDrain, pudPullUp, AF7);
    // DMA
    PDmaRx = dmaStreamAlloc(Params->DmaRxID, IRQ_PRIO_MEDIUM, nullptr, NULL);
    dmaStreamSetPeripheral(PDmaRx, &Params->puart->UART_RX_REG);
    dmaStreamSetMemory0   (PDmaRx, IRxBuf);
    dmaStreamSetTransactionSize(PDmaRx, UART_RXBUF_SZ);
    dmaStreamSetMode      (PDmaRx, Params->DmaModeRx);
    dmaStreamEnable       (PDmaRx);
    Params->puart->CR1 |= USART_CR1_UE;    // Enable USART
}

void BaseUart::Shutdown() {
    Params->puart->CR1 &= ~USART_CR1_UE; // UART Disable
    if     (Params->puart == USART1) { rccDisableUSART1(); }
    else if(Params->puart == USART2) { rccDisableUSART2(); }
    else if(Params->puart == USART3) { rccDisableUSART3(); }
}

void BaseUart::OnClkChange() {
    if(Params->puart == USART1) Params->puart->BRR = Clk.APB2FreqHz / Params->Baudrate;
    else                       Params->puart->BRR = Clk.APB1FreqHz / Params->Baudrate;
}
