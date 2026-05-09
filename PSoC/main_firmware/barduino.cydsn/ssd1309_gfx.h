#ifndef SSD1309_GFX_H
#define SSD1309_GFX_H

#include <stdint.h>
#include "ssd1309.h"

void SSD1309_DrawPixel(uint8_t x, uint8_t y, ssd1309_color_t color);
void SSD1309_DrawFastHLine(uint8_t x, uint8_t y, uint8_t w, ssd1309_color_t color);
void SSD1309_DrawFastVLine(uint8_t x, uint8_t y, uint8_t h, ssd1309_color_t color);
void SSD1309_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, ssd1309_color_t color);
void SSD1309_FillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, ssd1309_color_t color);

void SSD1309_SetCursor(uint8_t x, uint8_t y);
void SSD1309_WriteChar(char c, ssd1309_color_t color);
void SSD1309_WriteString(const char *str, ssd1309_color_t color);

void SSD1309_DrawBitmap(uint8_t x,
                        uint8_t y,
                        const unsigned char *bitmap,
                        uint16_t width,
                        uint16_t height,
                        ssd1309_color_t color);

void SSD1309_DrawBitmapWithBackground(uint8_t x,
                                      uint8_t y,
                                      const unsigned char *bitmap,
                                      uint16_t width,
                                      uint16_t height,
                                      ssd1309_color_t foreground,
                                      ssd1309_color_t background);

#endif
