#include "neopixel_anim.h"
#include "neopixel.h"

#define DEFAULT_INTERVAL_MS  50u

typedef struct
{
    neopixel_anim_mode_t mode;

    uint8_t r;
    uint8_t g;
    uint8_t b;

    uint16_t interval_ms;
    uint32_t last_update_ms;

    uint16_t step;
    int8_t direction;
} neopixel_anim_state_t;

static neopixel_anim_state_t anim;

static volatile uint32_t anim_millis = 0u;

/*
 * If another module already owns SysTick, do not use this callback.
 * Replace NeoPixelAnim_Millis() with your shared SystemTime_Millis().
 */
static void NeoPixelAnim_SysTickISR(void)
{
    anim_millis++;
}

static uint32_t NeoPixelAnim_Millis(void)
{
    uint32_t now;

    CyGlobalIntDisable;
    now = anim_millis;
    CyGlobalIntEnable;

    return now;
}

static uint8_t elapsed(uint32_t start_ms, uint16_t interval_ms)
{
    return ((uint32_t)(NeoPixelAnim_Millis() - start_ms) >= interval_ms) ? 1u : 0u;
}

static uint8_t scale8(uint8_t value, uint8_t scale)
{
    return (uint8_t)(((uint16_t)value * (uint16_t)scale) / 255u);
}

/*
 * Standard color wheel.
 * pos: 0-255
 */
static void color_wheel(uint8_t pos, uint8_t *r, uint8_t *g, uint8_t *b)
{
    pos = 255u - pos;

    if (pos < 85u)
    {
        *r = (uint8_t)(255u - pos * 3u);
        *g = 0u;
        *b = (uint8_t)(pos * 3u);
    }
    else if (pos < 170u)
    {
        pos -= 85u;
        *r = 0u;
        *g = (uint8_t)(pos * 3u);
        *b = (uint8_t)(255u - pos * 3u);
    }
    else
    {
        pos -= 170u;
        *r = (uint8_t)(pos * 3u);
        *g = (uint8_t)(255u - pos * 3u);
        *b = 0u;
    }
}

/*
 * Approximate triangle wave brightness: 0 -> 255 -> 0.
 */
static uint8_t triangle_brightness(uint16_t step)
{
    uint8_t phase = (uint8_t)(step & 0xFFu);

    if (phase < 128u)
    {
        return (uint8_t)(phase * 2u);
    }
    else
    {
        return (uint8_t)(255u - ((phase - 128u) * 2u));
    }
}

static void anim_rainbow(void)
{
    uint8_t r;
    uint8_t g;
    uint8_t b;

    for (uint16_t i = 0u; i < NEOPIXEL_COUNT; i++)
    {
        uint8_t pos = (uint8_t)(((uint32_t)i * 256u / NEOPIXEL_COUNT) + anim.step);
        color_wheel(pos, &r, &g, &b);
        NeoPixel_SetPixel(i, r, g, b);
    }

    NeoPixel_Show();
    anim.step++;
}

static void anim_theater_chase(void)
{
    for (uint16_t i = 0u; i < NEOPIXEL_COUNT; i++)
    {
        if (((i + anim.step) % 3u) == 0u)
        {
            NeoPixel_SetPixel(i, anim.r, anim.g, anim.b);
        }
        else
        {
            NeoPixel_SetPixel(i, 0u, 0u, 0u);
        }
    }

    NeoPixel_Show();
    anim.step++;
}

static void anim_breathing(void)
{
    uint8_t brightness = triangle_brightness(anim.step);

    uint8_t r = scale8(anim.r, brightness);
    uint8_t g = scale8(anim.g, brightness);
    uint8_t b = scale8(anim.b, brightness);

    NeoPixel_SetAll(r, g, b);
    NeoPixel_Show();

    anim.step += 4u;
}

static void anim_scanner(void)
{
    uint16_t pos;

    NeoPixel_Clear();

    pos = anim.step;

    if (pos < NEOPIXEL_COUNT)
    {
        NeoPixel_SetPixel(pos, anim.r, anim.g, anim.b);

        if (pos > 0u)
        {
            NeoPixel_SetPixel(pos - 1u,
                              scale8(anim.r, 80u),
                              scale8(anim.g, 80u),
                              scale8(anim.b, 80u));
        }

        if (pos > 1u)
        {
            NeoPixel_SetPixel(pos - 2u,
                              scale8(anim.r, 25u),
                              scale8(anim.g, 25u),
                              scale8(anim.b, 25u));
        }
    }

    NeoPixel_Show();

    if (anim.direction > 0)
    {
        if (anim.step + 1u >= NEOPIXEL_COUNT)
        {
            anim.direction = -1;
        }
        else
        {
            anim.step++;
        }
    }
    else
    {
        if (anim.step == 0u)
        {
            anim.direction = 1;
        }
        else
        {
            anim.step--;
        }
    }
}

static uint16_t lfsr_random(void)
{
    /*
     * Tiny pseudo-random generator.
     */
    static uint16_t lfsr = 0xACE1u;

    uint16_t bit = (uint16_t)(((lfsr >> 0) ^
                               (lfsr >> 2) ^
                               (lfsr >> 3) ^
                               (lfsr >> 5)) & 1u);

    lfsr = (uint16_t)((lfsr >> 1) | (bit << 15));

    return lfsr;
}

static void anim_sparkle(void)
{
    uint16_t index;

    /*
     * Fade all pixels by redrawing them dim/off. This simple version does
     * sparse white/color pops on black.
     */
    NeoPixel_Clear();

    index = lfsr_random() % NEOPIXEL_COUNT;

    NeoPixel_SetPixel(index, anim.r, anim.g, anim.b);

    /*
     * Add a second occasional sparkle.
     */
    if ((lfsr_random() & 0x03u) == 0u)
    {
        index = lfsr_random() % NEOPIXEL_COUNT;
        NeoPixel_SetPixel(index,
                          scale8(anim.r, 80u),
                          scale8(anim.g, 80u),
                          scale8(anim.b, 80u));
    }

    NeoPixel_Show();
}

void NeoPixelAnim_Init(void)
{
    /*
     * If you already have a shared SysTick timebase, remove these two lines
     * and have NeoPixelAnim_Millis() call that shared millis function instead.
     */
    CySysTickStart();
    CySysTickSetCallback(2u, NeoPixelAnim_SysTickISR);

    anim.mode = NEOPIXEL_ANIM_OFF;
    anim.r = 0u;
    anim.g = 0u;
    anim.b = 0u;
    anim.interval_ms = DEFAULT_INTERVAL_MS;
    anim.last_update_ms = 0u;
    anim.step = 0u;
    anim.direction = 1;

    NeoPixel_ClearAndShow();
}

void NeoPixelAnim_Update(void)
{
    if (anim.mode == NEOPIXEL_ANIM_OFF)
    {
        return;
    }

    if (!elapsed(anim.last_update_ms, anim.interval_ms))
    {
        return;
    }

    anim.last_update_ms = NeoPixelAnim_Millis();

    switch (anim.mode)
    {
        case NEOPIXEL_ANIM_SOLID:
            NeoPixel_SetAll(anim.r, anim.g, anim.b);
            NeoPixel_Show();
            break;

        case NEOPIXEL_ANIM_RAINBOW:
            anim_rainbow();
            break;

        case NEOPIXEL_ANIM_THEATER_CHASE:
            anim_theater_chase();
            break;

        case NEOPIXEL_ANIM_BREATHING:
            anim_breathing();
            break;

        case NEOPIXEL_ANIM_SCANNER:
            anim_scanner();
            break;

        case NEOPIXEL_ANIM_SPARKLE:
            anim_sparkle();
            break;

        default:
            break;
    }
}

void NeoPixelAnim_SetOff(void)
{
    anim.mode = NEOPIXEL_ANIM_OFF;
    NeoPixel_ClearAndShow();
}

void NeoPixelAnim_SetSolid(uint8_t r, uint8_t g, uint8_t b)
{
    anim.mode = NEOPIXEL_ANIM_SOLID;
    anim.r = r;
    anim.g = g;
    anim.b = b;
    anim.interval_ms = DEFAULT_INTERVAL_MS;
    anim.last_update_ms = 0u;
    anim.step = 0u;

    NeoPixel_SetAll(r, g, b);
    NeoPixel_Show();
}

void NeoPixelAnim_SetRainbow(uint16_t interval_ms)
{
    anim.mode = NEOPIXEL_ANIM_RAINBOW;
    anim.interval_ms = interval_ms ? interval_ms : DEFAULT_INTERVAL_MS;
    anim.last_update_ms = 0u;
    anim.step = 0u;
}

void NeoPixelAnim_SetTheaterChase(uint8_t r, uint8_t g, uint8_t b, uint16_t interval_ms)
{
    anim.mode = NEOPIXEL_ANIM_THEATER_CHASE;
    anim.r = r;
    anim.g = g;
    anim.b = b;
    anim.interval_ms = interval_ms ? interval_ms : DEFAULT_INTERVAL_MS;
    anim.last_update_ms = 0u;
    anim.step = 0u;
}

void NeoPixelAnim_SetBreathing(uint8_t r, uint8_t g, uint8_t b, uint16_t interval_ms)
{
    anim.mode = NEOPIXEL_ANIM_BREATHING;
    anim.r = r;
    anim.g = g;
    anim.b = b;
    anim.interval_ms = interval_ms ? interval_ms : DEFAULT_INTERVAL_MS;
    anim.last_update_ms = 0u;
    anim.step = 0u;
}

void NeoPixelAnim_SetScanner(uint8_t r, uint8_t g, uint8_t b, uint16_t interval_ms)
{
    anim.mode = NEOPIXEL_ANIM_SCANNER;
    anim.r = r;
    anim.g = g;
    anim.b = b;
    anim.interval_ms = interval_ms ? interval_ms : DEFAULT_INTERVAL_MS;
    anim.last_update_ms = 0u;
    anim.step = 0u;
    anim.direction = 1;
}

void NeoPixelAnim_SetSparkle(uint8_t r, uint8_t g, uint8_t b, uint16_t interval_ms)
{
    anim.mode = NEOPIXEL_ANIM_SPARKLE;
    anim.r = r;
    anim.g = g;
    anim.b = b;
    anim.interval_ms = interval_ms ? interval_ms : DEFAULT_INTERVAL_MS;
    anim.last_update_ms = 0u;
    anim.step = 0u;
}

neopixel_anim_mode_t NeoPixelAnim_GetMode(void)
{
    return anim.mode;
}