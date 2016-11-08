/*
 * board.h
 *
 *  Created on: 12 сент. 2015 г.
 *      Author: Kreyl
 */

#pragma once

#include <inttypes.h>

// ==== General ====
#define BOARD_NAME          "Locket4_1"
#define APP_NAME            "FirstDaysOfEmpire"

// ==== High-level peripery control ====
#define PILL_ENABLED        TRUE
#define BEEPER_ENABLED      FALSE
#define BTN_ENABLED         TRUE

#define SIMPLESENSORS_ENABLED   BTN_ENABLED

// MCU type as defined in the ST header.
#define STM32L151xB

// Freq of external crystal if any. Leave it here even if not used.
#define CRYSTAL_FREQ_HZ     12000000

#define SYS_TIM_CLK         (Clk.APB1FreqHz)
#define I2C1_ENABLED        PILL_ENABLED
#define I2C_USE_SEMAPHORE   FALSE
#define ADC_REQUIRED        FALSE

#if 1 // ========================== GPIO =======================================
// PortMinTim_t: GPIO, Pin, Tim, TimChnl, invInverted, omPushPull, TopValue
// UART
#define UART_GPIO       GPIOA
#define UART_TX_PIN     9
#define UART_RX_PIN     10
#define UART_AF         AF7 // for USART1 @ GPIOA

// LED
#define LED_EN_PIN      { GPIOB, 2, omPushPull }
#define LED_R_PIN       { GPIOB, 1, TIM3, 4, invInverted, omOpenDrain, 255 }
#define LED_G_PIN       { GPIOB, 0, TIM3, 3, invInverted, omOpenDrain, 255 }
#define LED_B_PIN       { GPIOB, 5, TIM3, 2, invInverted, omOpenDrain, 255 }

// Button
#define BTN_PIN         { GPIOA, 0, pudPullDown }

// Vibro
#define VIBRO_TOP       99
#define VIBRO_PIN       { GPIOB, 12, TIM10, 1, invNotInverted, omPushPull, VIBRO_TOP }

// Beeper
#define BEEPER_TOP      22
#define BEEPER_PIN      { GPIOB, 15, TIM11, 1, invNotInverted, omPushPull, BEEPER_TOP }

// DIP switch
#define DIP_SW_CNT      6
#define DIP_SW1         { GPIOA, 15, pudPullUp }
#define DIP_SW2         { GPIOC, 13, pudPullUp }
#define DIP_SW3         { GPIOC, 14, pudPullUp }
#define DIP_SW4         { GPIOA, 12, pudPullUp }
#define DIP_SW5         { GPIOA, 11, pudPullUp }
#define DIP_SW6         { GPIOA, 8,  pudPullUp }

// I2C
#if I2C1_ENABLED
#define I2C1_GPIO       GPIOB
#define I2C1_SCL        6
#define I2C1_SDA        7
#define I2C1_AF         AF4
#endif

// Pill power
#define PILL_PWR_PIN    { GPIOB, 3, omPushPull }

// Radio
#define CC_GPIO         GPIOA
#define CC_GDO2         2
#define CC_GDO0         3
#define CC_SCK          5
#define CC_MISO         6
#define CC_MOSI         7
#define CC_CS           4
// Input pin (do not touch)
#define CC_GDO0_IRQ     { CC_GPIO, CC_GDO0, pudNone }

#endif // GPIO

#if 1 // ========================= Timer =======================================
#endif // Timer

#if I2C1_ENABLED // ====================== I2C ================================
#define I2C1_BAUDRATE   400000
#define I2C_PILL        i2c1
#endif

#if 1 // =========================== SPI =======================================
#define CC_SPI          SPI1
#define CC_SPI_AF       AF5
#endif

#if 1 // ========================== USART ======================================
#define UART            USART1
#define UART_TX_REG     UART->DR
#define UART_RX_REG     UART->DR
#endif

#if ADC_REQUIRED // ======================= Inner ADC ==========================
// Clock divider: clock is generated from the APB2
#define ADC_CLK_DIVIDER     adcDiv2

// ADC channels
//#define BAT_CHNL          1

#define ADC_VREFINT_CHNL    17  // All 4xx, F072 and L151 devices. Do not change.
#define ADC_CHANNELS        { ADC_VREFINT_CHNL }
#define ADC_CHANNEL_CNT     1   // Do not use countof(AdcChannels) as preprocessor does not know what is countof => cannot check
#define ADC_SAMPLE_TIME     ast96Cycles
#define ADC_SAMPLE_CNT      8   // How many times to measure every channel

#define ADC_SEQ_LEN         (ADC_SAMPLE_CNT * ADC_CHANNEL_CNT)

#endif

#if 1 // =========================== DMA =======================================
#define STM32_DMA_REQUIRED  TRUE
// ==== Uart ====
#define UART_DMA_TX     STM32_DMA1_STREAM4
#define UART_DMA_RX     STM32_DMA1_STREAM5
#define UART_DMA_CHNL   0   // Dummy

#if I2C1_ENABLED // ==== I2C ====
#define I2C1_DMA_TX     STM32_DMA1_STREAM6
#define I2C1_DMA_RX     STM32_DMA1_STREAM7
#endif

#if ADC_REQUIRED
#define ADC_DMA         STM32_DMA1_STREAM1
#define ADC_DMA_MODE    STM32_DMA_CR_CHSEL(0) |   /* dummy */ \
                        DMA_PRIORITY_LOW | \
                        STM32_DMA_CR_MSIZE_HWORD | \
                        STM32_DMA_CR_PSIZE_HWORD | \
                        STM32_DMA_CR_MINC |       /* Memory pointer increase */ \
                        STM32_DMA_CR_DIR_P2M |    /* Direction is peripheral to memory */ \
                        STM32_DMA_CR_TCIE         /* Enable Transmission Complete IRQ */
#endif // ADC

#endif // DMA
