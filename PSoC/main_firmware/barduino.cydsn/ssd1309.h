#ifndef SSD1309_H
#define SSD1309_H

#include <project.h>
#include <stdint.h>

#define SSD1309_WIDTH      128
#define SSD1309_HEIGHT     64
#define SSD1309_PAGES      8
#define SSD1309_BUFSIZE    (SSD1309_WIDTH * SSD1309_PAGES)

typedef enum
{
    SSD1309_COLOR_BLACK = 0,
    SSD1309_COLOR_WHITE = 1,
    SSD1309_COLOR_INVERT = 2
} ssd1309_color_t;

extern uint8_t ssd1309_buffer[SSD1309_BUFSIZE];

void SSD1309_Start(void);
void SSD1309_Reset(void);
void SSD1309_Command(uint8_t command);
void SSD1309_Data(uint8_t data);
void SSD1309_Update(void);
void SSD1309_Clear(void);
void SSD1309_Fill(uint8_t pattern);
void SSD1309_DisplayOn(void);
void SSD1309_DisplayOff(void);
void SSD1309_SetContrast(uint8_t contrast);
void SSD1309_Invert(uint8_t invert);

#endif
