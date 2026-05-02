#include "neopixel.h"

/*
 * StripLights color format:
 *
 * The example project uses:
 *     StripLights_Pixel(ledPosition, 0, 0x00550000);
 *
 * That indicates a 24-bit packed color layout of:
 *     0x00RRGGBB
 *
 * The StripLights customizer handles WS2811 vs WS2812 byte ordering internally
 * based on the Part Type setting. If red/green are swapped, change the Part Type
 * in the StripLights customizer between WS2811 and WS2812.
 */
#define NEOPIXEL_BLACK 0x00000000u

static neopixel_color_t neopixel_cache[NEOPIXEL_COUNT];

static uint32_t NeoPixel_PackColor(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)r << 16) |
           ((uint32_t)g << 8)  |
           ((uint32_t)b);
}

static void NeoPixel_WaitReady(void)
{
    while (StripLights_Ready() == 0u)
    {
        /* wait for previous LED transfer to finish */
    }
}

void NeoPixel_Init(void)
{
    /*
     * Datasheet:
     * StripLights_Start() initializes hardware, clears display memory,
     * and enables interrupts.
     */
    StripLights_Start();

    /*
     * No dimming by default.
     * Datasheet dim levels:
     *     0 = no dimming
     *     1 = 50%
     *     2 = 25%
     *     3 = 12.5%
     *     4 = 6.3%
     */
    StripLights_Dim(0u);

    NeoPixel_ClearAndShow();
}

void NeoPixel_Show(void)
{
    /*
     * TriggerWait() starts transfer and does not return until complete.
     * For only 5 LEDs this is short and avoids queueing issues.
     */
    NeoPixel_WaitReady();
    StripLights_TriggerWait(1u);
}

void NeoPixel_Clear(void)
{
    for (uint16_t i = 0u; i < NEOPIXEL_COUNT; i++)
    {
        neopixel_cache[i].r = 0u;
        neopixel_cache[i].g = 0u;
        neopixel_cache[i].b = 0u;
    }

    /*
     * MemClear clears display memory but does not update the LEDs.
     */
    StripLights_MemClear(NEOPIXEL_BLACK);
}

void NeoPixel_ClearAndShow(void)
{
    NeoPixel_Clear();
    NeoPixel_Show();
}

void NeoPixel_SetPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t color;

    if (index >= NEOPIXEL_COUNT)
    {
        return;
    }

    neopixel_cache[index].r = r;
    neopixel_cache[index].g = g;
    neopixel_cache[index].b = b;

    color = NeoPixel_PackColor(r, g, b);

    /*
     * Your configuration is one channel, five LEDs.
     * Use x = LED index, y = 0.
     */
    StripLights_Pixel((uint32_t)index, 0u, color);
}

void NeoPixel_SetAll(uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t color = NeoPixel_PackColor(r, g, b);

    for (uint16_t i = 0u; i < NEOPIXEL_COUNT; i++)
    {
        neopixel_cache[i].r = r;
        neopixel_cache[i].g = g;
        neopixel_cache[i].b = b;

        StripLights_Pixel((uint32_t)i, 0u, color);
    }
}

void NeoPixel_FillAndShow(uint8_t r, uint8_t g, uint8_t b)
{
    NeoPixel_SetAll(r, g, b);
    NeoPixel_Show();
}

void NeoPixel_SetBrightness(uint8_t brightness)
{
    uint32_t dim_level;

    /*
     * StripLights_Dim() levels from datasheet:
     *     0: no dimming
     *     1: 50%
     *     2: 25%
     *     3: 12.5%
     *     4: 6.3%
     *
     * Map Arduino-like brightness 0-255 to those coarse levels.
     */
    if (brightness >= 224u)
    {
        dim_level = 0u;   // full brightness
    }
    else if (brightness >= 112u)
    {
        dim_level = 1u;   // 50%
    }
    else if (brightness >= 56u)
    {
        dim_level = 2u;   // 25%
    }
    else if (brightness >= 28u)
    {
        dim_level = 3u;   // 12.5%
    }
    else
    {
        dim_level = 4u;   // 6.3%
    }

    StripLights_Dim(dim_level);
}

neopixel_color_t NeoPixel_GetPixel(uint16_t index)
{
    neopixel_color_t color = {0u, 0u, 0u};

    if (index >= NEOPIXEL_COUNT)
    {
        return color;
    }

    return neopixel_cache[index];
}

neopixel_color_t NeoPixel_Color(uint8_t r, uint8_t g, uint8_t b)
{
    neopixel_color_t color;

    color.r = r;
    color.g = g;
    color.b = b;

    return color;
}