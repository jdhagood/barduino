#include "ssd1309_gfx.h"
#include "font5x7.h"

static uint8_t cursor_x = 0;
static uint8_t cursor_y = 0;

void SSD1309_DrawPixel(uint8_t x, uint8_t y, ssd1309_color_t color)
{
    if (x >= SSD1309_WIDTH || y >= SSD1309_HEIGHT)
    {
        return;
    }

    uint16_t index = x + (y / 8) * SSD1309_WIDTH;
    uint8_t bit_mask = 1 << (y & 0x07);

    switch (color)
    {
        case SSD1309_COLOR_WHITE:
            ssd1309_buffer[index] |= bit_mask;
            break;

        case SSD1309_COLOR_BLACK:
            ssd1309_buffer[index] &= ~bit_mask;
            break;

        case SSD1309_COLOR_INVERT:
            ssd1309_buffer[index] ^= bit_mask;
            break;

        default:
            break;
    }
}

void SSD1309_DrawFastHLine(uint8_t x, uint8_t y, uint8_t w, ssd1309_color_t color)
{
    if (y >= SSD1309_HEIGHT || x >= SSD1309_WIDTH)
    {
        return;
    }

    if ((x + w) > SSD1309_WIDTH)
    {
        w = SSD1309_WIDTH - x;
    }

    for (uint8_t i = 0; i < w; i++)
    {
        SSD1309_DrawPixel(x + i, y, color);
    }
}

void SSD1309_DrawFastVLine(uint8_t x, uint8_t y, uint8_t h, ssd1309_color_t color)
{
    if (x >= SSD1309_WIDTH || y >= SSD1309_HEIGHT)
    {
        return;
    }

    if ((y + h) > SSD1309_HEIGHT)
    {
        h = SSD1309_HEIGHT - y;
    }

    for (uint8_t i = 0; i < h; i++)
    {
        SSD1309_DrawPixel(x, y + i, color);
    }
}

void SSD1309_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, ssd1309_color_t color)
{
    if (w == 0 || h == 0)
    {
        return;
    }

    SSD1309_DrawFastHLine(x, y, w, color);
    SSD1309_DrawFastHLine(x, y + h - 1, w, color);
    SSD1309_DrawFastVLine(x, y, h, color);
    SSD1309_DrawFastVLine(x + w - 1, y, h, color);
}

void SSD1309_FillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, ssd1309_color_t color)
{
    for (uint8_t yy = y; yy < y + h && yy < SSD1309_HEIGHT; yy++)
    {
        SSD1309_DrawFastHLine(x, yy, w, color);
    }
}

void SSD1309_SetCursor(uint8_t x, uint8_t y)
{
    cursor_x = x;
    cursor_y = y;
}

void SSD1309_WriteChar(char c, ssd1309_color_t color)
{
    if (c == '\n')
    {
        cursor_x = 0;
        cursor_y += 8;
        return;
    }

    if (c == '\r')
    {
        cursor_x = 0;
        return;
    }

    if (c < 32 || c > 126)
    {
        c = '?';
    }

    if (cursor_x + 6 >= SSD1309_WIDTH)
    {
        cursor_x = 0;
        cursor_y += 8;
    }

    if (cursor_y + 8 >= SSD1309_HEIGHT)
    {
        return;
    }

    uint16_t font_index = (c - 32) * 5;

    for (uint8_t col = 0; col < 5; col++)
    {
        uint8_t line = font5x7[font_index + col];

        for (uint8_t row = 0; row < 7; row++)
        {
            if (line & (1 << row))
            {
                SSD1309_DrawPixel(cursor_x + col, cursor_y + row, color);
            }
            else
            {
                SSD1309_DrawPixel(cursor_x + col, cursor_y + row, SSD1309_COLOR_BLACK);
            }
        }
    }

    // One blank column after character
    for (uint8_t row = 0; row < 7; row++)
    {
        SSD1309_DrawPixel(cursor_x + 5, cursor_y + row, SSD1309_COLOR_BLACK);
    }

    cursor_x += 6;
}

void SSD1309_WriteString(const char *str, ssd1309_color_t color)
{
    while (*str)
    {
        SSD1309_WriteChar(*str, color);
        str++;
    }
}
