#include "ws2812_palette.h"

/* Values are stored as {G, R, B}, matching the WS2812 wire order. */
static const WS2812_Color color_palette[WS2812_COLOR_COUNT] = {
    [WS2812_COLOR_RED]       = {  0U, 255U,   0U },
    [WS2812_COLOR_ORANGE]    = { 64U, 255U,   0U },
    [WS2812_COLOR_YELLOW]    = {255U, 255U,   0U },
    [WS2812_COLOR_GREEN]     = {255U,   0U,   0U },
    [WS2812_COLOR_SPRING]    = {255U,   0U, 128U },
    [WS2812_COLOR_CYAN]      = {255U,   0U, 255U },
    [WS2812_COLOR_BLUE]      = {  0U,   0U, 255U },
    [WS2812_COLOR_PURPLE]    = {  0U, 128U, 255U },
    [WS2812_COLOR_MAGENTA]   = {  0U, 255U, 255U },
    [WS2812_COLOR_PINK]      = { 32U, 128U, 255U },
    [WS2812_COLOR_WHITE]     = {255U, 255U, 255U },
    [WS2812_COLOR_WARM_WHITE]= {180U, 255U, 100U }
};

uint32_t WS2812_PaletteCount(void)
{
    return WS2812_COLOR_COUNT;
}

WS2812_Color WS2812_PaletteGet(WS2812_ColorId color_id)
{
    if ((uint32_t)color_id >= WS2812_COLOR_COUNT) {
        return color_palette[WS2812_COLOR_RED];
    }

    return color_palette[color_id];
}

WS2812_Color WS2812_ColorScale(WS2812_Color color, uint8_t brightness)
{
    color.green = (uint8_t)(((uint16_t)color.green * brightness) / 255U);
    color.red = (uint8_t)(((uint16_t)color.red * brightness) / 255U);
    color.blue = (uint8_t)(((uint16_t)color.blue * brightness) / 255U);
    return color;
}
