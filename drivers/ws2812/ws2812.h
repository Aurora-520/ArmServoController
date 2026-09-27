#ifndef WS2812_H
#define WS2812_H

#include <stdint.h>

/* Change this value when a strip with more LEDs is connected. */
#define WS2812_LED_COUNT 1U

typedef struct
{
    uint8_t green;
    uint8_t red;
    uint8_t blue;
} WS2812_Color;

void WS2812_Init(void);
void WS2812_SetPixel(uint32_t index, WS2812_Color color);
void WS2812_SetAll(WS2812_Color color);
void WS2812_Show(void);

#endif
