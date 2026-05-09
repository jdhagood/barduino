#ifndef NEOPIXEL_H
#define NEOPIXEL_H

#include <project.h>
#include <stdint.h>

/*
 * This should match the StripLights component configuration.
 * For your current hardware, configure StripLights as:
 *     LED Channels: 1
 *     LEDs per String: 30
 */
#define NEOPIXEL_COUNT  StripLights_TOTAL_LEDS

typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} neopixel_color_t;

void NeoPixel_Init(void);
void NeoPixel_Show(void);

void NeoPixel_Clear(void);
void NeoPixel_ClearAndShow(void);

void NeoPixel_SetPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b);
void NeoPixel_SetAll(uint8_t r, uint8_t g, uint8_t b);
void NeoPixel_FillAndShow(uint8_t r, uint8_t g, uint8_t b);

void NeoPixel_SetBrightness(uint8_t brightness);

neopixel_color_t NeoPixel_GetPixel(uint16_t index);
neopixel_color_t NeoPixel_Color(uint8_t r, uint8_t g, uint8_t b);

#endif