#ifndef STM32H7xx_HAL_CONF_H
#define STM32H7xx_HAL_CONF_H

#define HAL_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED

#define HSE_VALUE              24000000U
#define HSE_STARTUP_TIMEOUT    100U
#define CSI_VALUE              4000000U
#define HSI_VALUE              64000000U
#define LSE_VALUE              32768U
#define LSE_STARTUP_TIMEOUT    5000U
#define LSI_VALUE              32000U
#define EXTERNAL_CLOCK_VALUE   12288000U
#define VDD_VALUE              3300U
#define TICK_INT_PRIORITY      15U
#define USE_RTOS               0U
#define PREFETCH_ENABLE        0U
#define ART_ACCLERATOR_ENABLE  1U

#ifdef HAL_RCC_MODULE_ENABLED
#include "stm32h7xx_hal_rcc.h"
#endif
#ifdef HAL_GPIO_MODULE_ENABLED
#include "stm32h7xx_hal_gpio.h"
#endif
#ifdef HAL_DMA_MODULE_ENABLED
#include "stm32h7xx_hal_dma.h"
#include "stm32h7xx_hal_dma_ex.h"
#endif
#ifdef HAL_CORTEX_MODULE_ENABLED
#include "stm32h7xx_hal_cortex.h"
#endif
#ifdef HAL_FLASH_MODULE_ENABLED
#include "stm32h7xx_hal_flash.h"
#endif
#ifdef HAL_PWR_MODULE_ENABLED
#include "stm32h7xx_hal_pwr.h"
#include "stm32h7xx_hal_pwr_ex.h"
#endif
#ifdef HAL_UART_MODULE_ENABLED
#include "stm32h7xx_hal_uart.h"
#endif

#ifdef __GNUC__
#define assert_param(expr) ((void)0U)
#else
#define assert_param(expr) ((void)0U)
#endif

#endif
