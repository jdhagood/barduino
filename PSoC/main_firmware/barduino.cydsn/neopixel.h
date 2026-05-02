#ifndef NEOPIXEL_H
#define NEOPIXEL_H

#include <project.h>
#include <stdint.h>

/*
 * You configured StripLights as:
 *     1 channel
 *     5 LEDs per string
 *
 * StripLights_TOTAL_LEDS should therefore be 5.
 */
#define NEOPIXEL_COUNT  5

typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} neopixel_color_t;

/*
 * Initialize StripLights-based NeoPixel control.
 */
void NeoPixel_Init(void);

/*
 * Push current StripLights display memory to the physical LEDs.
 * This waits for the previous update to finish, then triggers a blocking update.
 */
void NeoPixel_Show(void);

/*
 * Clear display memory only. Call NeoPixel_Show() to update physical LEDs.
 */
void NeoPixel_Clear(void);

/*
 * Clear display memory and immediately update LEDs.
 */
void NeoPixel_ClearAndShow(void);

/*
 * Set one LED in display memory. Call NeoPixel_Show() to update physical LEDs.
 */
void NeoPixel_SetPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b);

/*
 * Fill all LEDs in display memory. Call NeoPixel_Show() to update physical LEDs.
 */
void NeoPixel_SetAll(uint8_t r, uint8_t g, uint8_t b);

/*
 * Fill all LEDs and immediately update physical LEDs.
 */
void NeoPixel_FillAndShow(uint8_t r, uint8_t g, uint8_t b);

/*
 * Set brightness using 0-255 style input.
 *
 * Internally maps to StripLights_Dim():
 *     255 -> no dimming
 *     128 -> 50%
 *     64  -> 25%
 *     32  -> 12.5%
 *     low -> 6.3%
 */
void NeoPixel_SetBrightness(uint8_t brightness);

/*
 * Read back software-side cached color.
 */
neopixel_color_t NeoPixel_GetPixel(uint16_t index);

/*
 * Convenience constructor.
 */
neopixel_color_t NeoPixel_Color(uint8_t r, uint8_t g, uint8_t b);

#endif