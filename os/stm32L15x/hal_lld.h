/*
    ChibiOS - Copyright (C) 2006..2015 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/**
 * @file    STM32L1xx/hal_lld.h
 * @brief   STM32L1xx HAL subsystem low level driver header.
 * @pre     This module requires the following macros to be defined in the
 *          @p board.h file:
 *          - STM32_LSECLK.
 *          - STM32_HSECLK.
 *          - STM32_HSE_BYPASS (optionally).
 *          .
 *          One of the following macros must also be defined:
 *          - STM32L100xB, STM32L100xBA, STM32L100xC.
 *          - STM32L151xB, STM32L151xBA, STM32L151xC, STM32L151xCA,
 *            STM32L151xD, STM32L151xDX, STM32L151xE.
 *          - STM32L152xB, STM32L152xBA, STM32L152xC, STM32L152xCA,
 *            STM32L152xD, STM32L152xDX, STM32L152xE.
 *          - STM32L162xC, STM32L162xCA, STM32L162xD, STM32L162xDX,
 *            STM32L162xE.
 *          .
 *
 * @addtogroup HAL
 * @{
 */

#ifndef _HAL_LLD_H_
#define _HAL_LLD_H_

#include "stm32_registry.h"

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @name    Platform identification
 * @{
 */
#if defined(STM32L100xB) || defined(STM32L151xB) ||                         \
    defined(STM32L152xB) || defined(__DOXYGEN__)
#define PLATFORM_NAME           "STM32L1xx Ultra Low Power Medium Density"

#elif defined(STM32L100xBA) || defined(STM32L100xC)  ||                     \
      defined(STM32L151xBA) || defined(STM32L151xC)  ||                     \
      defined(STM32L151xCA) || defined(STM32L152xBA) ||                     \
      defined(STM32L152xC)  || defined(STM32L152xCA) ||                     \
      defined(STM32L162xC)  || defined(STM32L162xCA)
#define PLATFORM_NAME           "STM32L1xx Ultra Low Power Medium Density Plus"

#elif defined(STM32L151xD)  || defined(STM32L151xDX) ||                     \
      defined(STM32L151xE)  || defined(STM32L152xD)  ||                     \
      defined(STM32L152xDX) || defined(STM32L152xE)  ||                     \
      defined(STM32L162xD)  || defined(STM32L162xDX) ||                     \
      defined(STM32L162xE)
#define PLATFORM_NAME           "STM32L1xx Ultra Low Power High Density"

#else
#error "STM32L1xx device not specified"
#endif

/**
 * @brief   Sub-family identifier.
 */
#if !defined(STM32L1XX) || defined(__DOXYGEN__)
#define STM32L1XX
#endif
/** @} */

/**
 * @name    Internal clock sources
 * @{
 */
#define STM32_HSICLK            16000000    /**< High speed internal clock. */
#define STM32_LSICLK            38000       /**< Low speed internal clock.  */
/** @} */

/**
 * @name    PWR_CR register bits definitions
 * @{
 */
#define STM32_VOS_MASK          (3 << 11)   /**< Core voltage mask.         */
#define STM32_VOS_1P8           (1 << 11)   /**< Core voltage 1.8 Volts.    */
#define STM32_VOS_1P5           (2 << 11)   /**< Core voltage 1.5 Volts.    */
#define STM32_VOS_1P2           (3 << 11)   /**< Core voltage 1.2 Volts.    */

#define STM32_PLS_MASK          (7 << 5)    /**< PLS bits mask.             */
#define STM32_PLS_LEV0          (0 << 5)    /**< PVD level 0.               */
#define STM32_PLS_LEV1          (1 << 5)    /**< PVD level 1.               */
#define STM32_PLS_LEV2          (2 << 5)    /**< PVD level 2.               */
#define STM32_PLS_LEV3          (3 << 5)    /**< PVD level 3.               */
#define STM32_PLS_LEV4          (4 << 5)    /**< PVD level 4.               */
#define STM32_PLS_LEV5          (5 << 5)    /**< PVD level 5.               */
#define STM32_PLS_LEV6          (6 << 5)    /**< PVD level 6.               */
#define STM32_PLS_LEV7          (7 << 5)    /**< PVD level 7.               */
/** @} */

/**
 * @name    RCC_CR register bits definitions
 * @{
 */
#define STM32_RTCPRE_MASK       (3 << 29)   /**< RTCPRE mask.               */
#define STM32_RTCPRE_DIV2       (0 << 29)   /**< HSE divided by 2.          */
#define STM32_RTCPRE_DIV4       (1 << 29)   /**< HSE divided by 4.          */
#define STM32_RTCPRE_DIV8       (2 << 29)   /**< HSE divided by 2.          */
#define STM32_RTCPRE_DIV16      (3 << 29)   /**< HSE divided by 16.         */
/** @} */

/**
 * @name    RCC_CFGR register bits definitions
 * @{
 */
#define STM32_SW_MSI            (0 << 0)    /**< SYSCLK source is MSI.      */
#define STM32_SW_HSI            (1 << 0)    /**< SYSCLK source is HSI.      */
#define STM32_SW_HSE            (2 << 0)    /**< SYSCLK source is HSE.      */
#define STM32_SW_PLL            (3 << 0)    /**< SYSCLK source is PLL.      */

#define STM32_HPRE_DIV1         (0 << 4)    /**< SYSCLK divided by 1.       */
#define STM32_HPRE_DIV2         (8 << 4)    /**< SYSCLK divided by 2.       */
#define STM32_HPRE_DIV4         (9 << 4)    /**< SYSCLK divided by 4.       */
#define STM32_HPRE_DIV8         (10 << 4)   /**< SYSCLK divided by 8.       */
#define STM32_HPRE_DIV16        (11 << 4)   /**< SYSCLK divided by 16.      */
#define STM32_HPRE_DIV64        (12 << 4)   /**< SYSCLK divided by 64.      */
#define STM32_HPRE_DIV128       (13 << 4)   /**< SYSCLK divided by 128.     */
#define STM32_HPRE_DIV256       (14 << 4)   /**< SYSCLK divided by 256.     */
#define STM32_HPRE_DIV512       (15 << 4)   /**< SYSCLK divided by 512.     */

#define STM32_PPRE1_DIV1        (0 << 8)    /**< HCLK divided by 1.         */
#define STM32_PPRE1_DIV2        (4 << 8)    /**< HCLK divided by 2.         */
#define STM32_PPRE1_DIV4        (5 << 8)    /**< HCLK divided by 4.         */
#define STM32_PPRE1_DIV8        (6 << 8)    /**< HCLK divided by 8.         */
#define STM32_PPRE1_DIV16       (7 << 8)    /**< HCLK divided by 16.        */

#define STM32_PPRE2_DIV1        (0 << 11)   /**< HCLK divided by 1.         */
#define STM32_PPRE2_DIV2        (4 << 11)   /**< HCLK divided by 2.         */
#define STM32_PPRE2_DIV4        (5 << 11)   /**< HCLK divided by 4.         */
#define STM32_PPRE2_DIV8        (6 << 11)   /**< HCLK divided by 8.         */
#define STM32_PPRE2_DIV16       (7 << 11)   /**< HCLK divided by 16.        */

#define STM32_PLLSRC_HSI        (0 << 16)   /**< PLL clock source is HSI.   */
#define STM32_PLLSRC_HSE        (1 << 16)   /**< PLL clock source is HSE.   */

#define STM32_MCOSEL_NOCLOCK    (0 << 24)   /**< No clock on MCO pin.       */
#define STM32_MCOSEL_SYSCLK     (1 << 24)   /**< SYSCLK on MCO pin.         */
#define STM32_MCOSEL_HSI        (2 << 24)   /**< HSI clock on MCO pin.      */
#define STM32_MCOSEL_MSI        (3 << 24)   /**< MSI clock on MCO pin.      */
#define STM32_MCOSEL_HSE        (4 << 24)   /**< HSE clock on MCO pin.      */
#define STM32_MCOSEL_PLL        (5 << 24)   /**< PLL clock on MCO pin.      */
#define STM32_MCOSEL_LSI        (6 << 24)   /**< LSI clock on MCO pin.      */
#define STM32_MCOSEL_LSE        (7 << 24)   /**< LSE clock on MCO pin.      */

#define STM32_MCOPRE_DIV1       (0 << 28)   /**< MCO divided by 1.          */
#define STM32_MCOPRE_DIV2       (1 << 28)   /**< MCO divided by 2.          */
#define STM32_MCOPRE_DIV4       (2 << 28)   /**< MCO divided by 4.          */
#define STM32_MCOPRE_DIV8       (3 << 28)   /**< MCO divided by 8.          */
#define STM32_MCOPRE_DIV16      (4 << 28)   /**< MCO divided by 16.         */
/** @} */

/**
 * @name    RCC_ICSCR register bits definitions
 * @{
 */
#define STM32_MSIRANGE_MASK     (7 << 13)   /**< MSIRANGE field mask.       */
#define STM32_MSIRANGE_64K      (0 << 13)   /**< 64kHz nominal.             */
#define STM32_MSIRANGE_128K     (1 << 13)   /**< 128kHz nominal.            */
#define STM32_MSIRANGE_256K     (2 << 13)   /**< 256kHz nominal.            */
#define STM32_MSIRANGE_512K     (3 << 13)   /**< 512kHz nominal.            */
#define STM32_MSIRANGE_1M       (4 << 13)   /**< 1MHz nominal.              */
#define STM32_MSIRANGE_2M       (5 << 13)   /**< 2MHz nominal.              */
#define STM32_MSIRANGE_4M       (6 << 13)   /**< 4MHz nominal               */
/** @} */

/**
 * @name    RCC_CSR register bits definitions
 * @{
 */
#define STM32_RTCSEL_MASK       (3 << 16)   /**< RTC source mask.           */
#define STM32_RTCSEL_NOCLOCK    (0 << 16)   /**< No RTC source.             */
#define STM32_RTCSEL_LSE        (1 << 16)   /**< RTC source is LSE.         */
#define STM32_RTCSEL_LSI        (2 << 16)   /**< RTC source is LSI.         */
#define STM32_RTCSEL_HSEDIV     (3 << 16)   /**< RTC source is HSE divided. */
/** @} */

/*
 * Configuration-related checks.
 */
#if !defined(STM32L1xx_MCUCONF)
#error "Using a wrong mcuconf.h file, STM32L1xx_MCUCONF not defined"
#endif


/* Various helpers.*/
#include "nvic.h"
#include "stm32_isr.h"
#include "stm32_dma.h"
#include "stm32_rcc.h"

#ifdef __cplusplus
extern "C" {
#endif
  void hal_lld_init(void);
  void stm32_clock_init(void);
#ifdef __cplusplus
}
#endif

#endif /* _HAL_LLD_H_ */

/** @} */
