#ifndef NEOPIXEL_ANIM_H
#define NEOPIXEL_ANIM_H

#include <project.h>
#include <stdint.h>

typedef enum
{
    NEOPIXEL_ANIM_OFF = 0,
    NEOPIXEL_ANIM_SOLID,
    NEOPIXEL_ANIM_RAINBOW,
    NEOPIXEL_ANIM_THEATER_CHASE,
    NEOPIXEL_ANIM_BREATHING,
    NEOPIXEL_ANIM_SCANNER,
    NEOPIXEL_ANIM_SPARKLE
} neopixel_anim_mode_t;

void NeoPixelAnim_Init(void);
void NeoPixelAnim_Update(void);

void NeoPixelAnim_SetOff(void);
void NeoPixelAnim_SetSolid(uint8_t r, uint8_t g, uint8_t b);

void NeoPixelAnim_SetRainbow(uint16_t interval_ms);
void NeoPixelAnim_SetTheaterChase(uint8_t r, uint8_t g, uint8_t b, uint16_t interval_ms);
void NeoPixelAnim_SetBreathing(uint8_t r, uint8_t g, uint8_t b, uint16_t interval_ms);
void NeoPixelAnim_SetScanner(uint8_t r, uint8_t g, uint8_t b, uint16_t interval_ms);
void NeoPixelAnim_SetSparkle(uint8_t r, uint8_t g, uint8_t b, uint16_t interval_ms);

neopixel_anim_mode_t NeoPixelAnim_GetMode(void);

#endif