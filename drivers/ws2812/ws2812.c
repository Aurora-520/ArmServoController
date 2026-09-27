#include "ws2812.h"
#include "stm32h7xx_hal.h"

#define WS2812_DATA_PORT       GPIOA
#define WS2812_DATA_PIN        GPIO_PIN_7

/* STM32H723 runs at 240 MHz in this project. WS2812 uses 800 kHz data. */
#define WS2812_T0H_CYCLES      84U
#define WS2812_T0L_CYCLES      216U
#define WS2812_T1H_CYCLES      168U
#define WS2812_T1L_CYCLES      132U

static WS2812_Color pixels[WS2812_LED_COUNT];

static void DelayCycles(uint32_t cycles)
{
    uint32_t start = DWT->CYCCNT;

    while ((uint32_t)(DWT->CYCCNT - start) < cycles) {
        /* The DWT counter provides a stable sub-microsecond delay. */
    }
}

static void SendByte(uint8_t value)
{
    for (uint32_t bit = 0U; bit < 8U; ++bit) {
        uint32_t high_cycles;
        uint32_t low_cycles;

        if ((value & 0x80U) != 0U) {
            high_cycles = WS2812_T1H_CYCLES;
            low_cycles = WS2812_T1L_CYCLES;
        } else {
            high_cycles = WS2812_T0H_CYCLES;
            low_cycles = WS2812_T0L_CYCLES;
        }

        WS2812_DATA_PORT->BSRR = WS2812_DATA_PIN;
        DelayCycles(high_cycles);
        WS2812_DATA_PORT->BSRR = ((uint32_t)WS2812_DATA_PIN << 16U);
        DelayCycles(low_cycles);
        value <<= 1U;
    }
}

void WS2812_Init(void)
{
    /* Enable the Cortex-M7 cycle counter used by the bit-bang timing. */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    for (uint32_t i = 0U; i < WS2812_LED_COUNT; ++i) {
        pixels[i].green = 0U;
        pixels[i].red = 0U;
        pixels[i].blue = 0U;
    }

    WS2812_DATA_PORT->BSRR = ((uint32_t)WS2812_DATA_PIN << 16U);
}

void WS2812_SetPixel(uint32_t index, WS2812_Color color)
{
    if (index < WS2812_LED_COUNT) {
        pixels[index] = color;
    }
}

void WS2812_SetAll(WS2812_Color color)
{
    for (uint32_t i = 0U; i < WS2812_LED_COUNT; ++i) {
        pixels[i] = color;
    }
}

void WS2812_Show(void)
{
    uint32_t primask = __get_PRIMASK();

    __disable_irq();
    for (uint32_t i = 0U; i < WS2812_LED_COUNT; ++i) {
        /* WS2812 wire order is G-R-B. */
        SendByte(pixels[i].green);
        SendByte(pixels[i].red);
        SendByte(pixels[i].blue);
    }
    WS2812_DATA_PORT->BSRR = ((uint32_t)WS2812_DATA_PIN << 16U);
    __set_PRIMASK(primask);

    /* The device latches the frame after a low level of at least 50 us. */
    HAL_Delay(1U);
}
