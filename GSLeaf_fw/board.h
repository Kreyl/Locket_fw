/*
 * board.h
 *
 *  Created on: 2015
 *      Author: Kreyl
 */

#ifndef BOARD_H__
#define BOARD_H__

// ==== General ====
#define BOARD_NAME          "GSLeaf01"
#define APP_NAME            "WarhammerPoint"

// MCU type as defined in the ST header.
#define STM32L476xx

// Freq of external crystal if any. Leave it here even if not used.
#define CRYSTAL_FREQ_HZ         12000000

// OS timer settings
#define STM32_ST_IRQ_PRIORITY   9
#define STM32_ST_USE_TIMER      5
#define STM32_TIMCLK1           (Clk.APB1FreqHz)    // Timer 5 is clocked by APB1

//  Periphery
#define I2C1_ENABLED            TRUE
#define I2C2_ENABLED            FALSE
#define I2C3_ENABLED            FALSE
#define SIMPLESENSORS_ENABLED   FALSE
#define BUTTONS_ENABLED         FALSE

#define ACC_REQUIRED            FALSE

#define ADC_REQUIRED            FALSE
#define STM32_DMA_REQUIRED      TRUE    // Leave this macro name for OS

#if 1 // ========================== GPIO =======================================
// EXTI
#define INDIVIDUAL_EXTI_IRQ_REQUIRED    FALSE

// Buttons
#define BTN1_PIN        GPIOA, 0, pudPullDown
#define BTN2_PIN        GPIOA, 1, pudPullDown

// UART
#define UART_TX_PIN     GPIOA, 9
#define UART_RX_PIN     GPIOA, 10

// RGB LED
#define LED_BLUE_CH     { GPIOB, 4, TIM3, 1, invNotInverted, omPushPull, 255 }
#define LED_GREEN_CH    { GPIOB, 1, TIM3, 4, invNotInverted, omPushPull, 255 }
#define LED_RED_CH      { GPIOB, 0, TIM3, 3, invNotInverted, omPushPull, 255 }

// PWR_EN
#define PWR_EN_PIN      GPIOA, 15, omPushPull
// Charge
#define CHARGE_PIN      GPIOB, 11, pudPullUp

// CS42L52
#define AU_RESET        GPIOB, 3, omPushPull
#define AU_SDIN         GPIOC, 3, omPushPull, pudNone, AF13 // MOSI; SAI1_A
#define AU_SDOUT        GPIOB, 5, omPushPull, pudNone, AF13 // MISO; SAI1_B
#define AU_MCLK         GPIOB, 8, omPushPull, pudNone, AF13
#define AU_MCLK_TIM     GPIOB, 8, TIM4, 3, invNotInverted, omPushPull, 1
#define AU_LRCK         GPIOB, 9, omPushPull, pudNone, AF13
#define AU_SCLK         GPIOB, 10, omPushPull, pudNone, AF13
#define AU_i2c          i2c1
#define AU_SAI          SAI1
#define AU_SAI_A        SAI1_Block_A
#define AU_SAI_B        SAI1_Block_B
#define AU_SAI_RccEn()  RCC->APB2ENR |= RCC_APB2ENR_SAI1EN
#define AU_SAI_RccDis() RCC->APB2ENR &= ~RCC_APB2ENR_SAI1EN
//#define AU_SAI_RccRst() RCC->APB2RSTR = RCC_APB2RSTR_SAI1RST

// Acc
#define Acc_i2c         i2c1
#define ACC_IRQ_GPIO    GPIOB
#define ACC_IRQ_PIN     2

// I2C
#define I2C1_GPIO       GPIOB
#define I2C1_SCL        6
#define I2C1_SDA        7
#define I2C2_GPIO       GPIOB
#define I2C2_SCL        10
#define I2C2_SDA        11
#define I2C3_GPIO       GPIOC
#define I2C3_SCL        0
#define I2C3_SDA        1
// I2C Alternate Function
#define I2C_AF          AF4

// SD
#define SD_PWR_PIN      GPIOC, 7
#define SD_AF           AF12
#define SD_DAT0         GPIOC,  8, omPushPull, pudPullUp, SD_AF
#define SD_DAT1         GPIOC,  9, omPushPull, pudPullUp, SD_AF
#define SD_DAT2         GPIOC, 10, omPushPull, pudPullUp, SD_AF
#define SD_DAT3         GPIOC, 11, omPushPull, pudPullUp, SD_AF
#define SD_CLK          GPIOC, 12, omPushPull, pudNone,   SD_AF
#define SD_CMD          GPIOD,  2, omPushPull, pudPullUp, SD_AF

// Radio: SPI, PGpio, Sck, Miso, Mosi, Cs, Gdo0
#define CC_Setup0       SPI1, GPIOA, 5,6,7, GPIOA,4, GPIOA,3

#endif // GPIO

#if 1 // =========================== I2C =======================================
// i2cclkPCLK1, i2cclkSYSCLK, i2cclkHSI
#define I2C_CLK_SRC     i2cclkHSI
#define I2C_BAUDRATE_HZ 400000
#endif

#if ADC_REQUIRED // ======================= Inner ADC ==========================
// Clock divider: clock is generated from the APB2
#define ADC_CLK_DIVIDER		adcDiv2

// ADC channels
#define ADC_BATTERY_CHNL 	14
// ADC_VREFINT_CHNL
#define ADC_CHANNELS        { ADC_BATTERY_CHNL, ADC_VREFINT_CHNL }
#define ADC_CHANNEL_CNT     2   // Do not use countof(AdcChannels) as preprocessor does not know what is countof => cannot check
#define ADC_SAMPLE_TIME     ast24d5Cycles
#define ADC_OVERSAMPLING_RATIO  64   // 1 (no oversampling), 2, 4, 8, 16, 32, 64, 128, 256
#endif

#if 1 // =========================== DMA =======================================
#define STM32_DMA_REQUIRED  TRUE
// ==== Uart ====
#define UART_DMA_TX_MODE(Chnl) (STM32_DMA_CR_CHSEL(Chnl) | DMA_PRIORITY_LOW | STM32_DMA_CR_MSIZE_BYTE | STM32_DMA_CR_PSIZE_BYTE | STM32_DMA_CR_MINC | STM32_DMA_CR_DIR_M2P | STM32_DMA_CR_TCIE)
#define UART_DMA_RX_MODE(Chnl) (STM32_DMA_CR_CHSEL(Chnl) | DMA_PRIORITY_MEDIUM | STM32_DMA_CR_MSIZE_BYTE | STM32_DMA_CR_PSIZE_BYTE | STM32_DMA_CR_MINC | STM32_DMA_CR_DIR_P2M | STM32_DMA_CR_CIRC)
#define UART_DMA_TX     STM32_DMA_STREAM_ID(1, 4)
#define UART_DMA_RX     STM32_DMA_STREAM_ID(1, 5)
#define UART_DMA_CHNL   2

// ==== I2C ====
#define I2C1_DMA_TX     STM32_DMA_STREAM_ID(2, 7)
#define I2C1_DMA_RX     STM32_DMA_STREAM_ID(2, 6)
#define I2C1_DMA_CHNL   5
#define I2C2_DMA_TX     STM32_DMA_STREAM_ID(1, 4)
#define I2C2_DMA_RX     STM32_DMA_STREAM_ID(1, 5)
#define I2C2_DMA_CHNL   3
#define I2C3_DMA_TX     STM32_DMA_STREAM_ID(1, 2)
#define I2C3_DMA_RX     STM32_DMA_STREAM_ID(1, 3)
#define I2C3_DMA_CHNL   3

// ==== SAI ====
#define SAI_DMA_A       STM32_DMA_STREAM_ID(2, 1)
#define SAI_DMA_B       STM32_DMA_STREAM_ID(2, 2)
#define SAI_DMA_CHNL    1

// ==== SDMMC ====
#define STM32_SDC_SDMMC1_DMA_STREAM   STM32_DMA_STREAM_ID(2, 5)

#if ADC_REQUIRED
#define ADC_DMA         STM32_DMA1_STREAM1
#define ADC_DMA_MODE    STM32_DMA_CR_CHSEL(0) |   /* DMA1 Stream1 Channel0 */ \
                        DMA_PRIORITY_LOW | \
                        STM32_DMA_CR_MSIZE_HWORD | \
                        STM32_DMA_CR_PSIZE_HWORD | \
                        STM32_DMA_CR_MINC |       /* Memory pointer increase */ \
                        STM32_DMA_CR_DIR_P2M |    /* Direction is peripheral to memory */ \
                        STM32_DMA_CR_TCIE         /* Enable Transmission Complete IRQ */
#endif // ADC

#endif // DMA

#if 1 // ========================== USART ======================================
#define PRINTF_FLOAT_EN FALSE
#define UART_TXBUF_SZ   512
#define UART_RXBUF_SZ   256
#define CMD_BUF_SZ      256

#define CMD_UART        USART1

#define CMD_UART_PARAMS \
    CMD_UART, UART_TX_PIN, UART_RX_PIN, \
    UART_DMA_TX, UART_DMA_RX, UART_DMA_TX_MODE(UART_DMA_CHNL), UART_DMA_RX_MODE(UART_DMA_CHNL), \
    uartclkHSI // Use independent clock

#endif

#endif //BOARD_H__
