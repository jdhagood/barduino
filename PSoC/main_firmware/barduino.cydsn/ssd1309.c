#include "ssd1309.h"
#include <string.h>

uint8_t ssd1309_buffer[SSD1309_BUFSIZE];

static void SSD1309_Select(void)
{
    OLED_CS_Write(0);
}

static void SSD1309_Deselect(void)
{
    OLED_CS_Write(1);
}

static void SSD1309_WriteByte(uint8_t byte)
{
    /*
     * Depending on your PSoC SPI component, status flag names may differ.
     * This version uses a conservative write-then-wait approach.
     */

    SPIM_1_WriteTxData(byte);

    /*
     * If this status macro does not compile, open SPIM_1.h and check the
     * generated status bit names. You can temporarily replace this wait with
     * CyDelayUs(2) for bring-up.
     */
    while (!(SPIM_1_ReadTxStatus() & SPIM_1_STS_SPI_DONE))
    {
        /* wait */
    }
}

void SSD1309_Command(uint8_t command)
{
    SSD1309_Select();
    OLED_DC_Write(0);       // command mode
    SSD1309_WriteByte(command);
    SSD1309_Deselect();
}

void SSD1309_Data(uint8_t data)
{
    SSD1309_Select();
    OLED_DC_Write(1);       // data mode
    SSD1309_WriteByte(data);
    SSD1309_Deselect();
}

void SSD1309_Reset(void)
{
    OLED_RST_Write(1);
    CyDelay(1);

    OLED_RST_Write(0);
    CyDelay(20);

    OLED_RST_Write(1);
    CyDelay(20);
}

static void SSD1309_SetAddressWindow(void)
{
    SSD1309_Command(0x21);  // Set column address
    SSD1309_Command(0x00);  // Start column
    SSD1309_Command(0x7F);  // End column: 127

    SSD1309_Command(0x22);  // Set page address
    SSD1309_Command(0x00);  // Start page
    SSD1309_Command(0x07);  // End page: 7
}

void SSD1309_Update(void)
{
    SSD1309_SetAddressWindow();

    SSD1309_Select();
    OLED_DC_Write(1);       // data mode

    for (uint16_t i = 0; i < SSD1309_BUFSIZE; i++)
    {
        SSD1309_WriteByte(ssd1309_buffer[i]);
    }

    SSD1309_Deselect();
}

void SSD1309_Clear(void)
{
    memset(ssd1309_buffer, 0x00, SSD1309_BUFSIZE);
}

void SSD1309_Fill(uint8_t pattern)
{
    memset(ssd1309_buffer, pattern, SSD1309_BUFSIZE);
}

void SSD1309_DisplayOn(void)
{
    SSD1309_Command(0xAF);
}

void SSD1309_DisplayOff(void)
{
    SSD1309_Command(0xAE);
}

void SSD1309_SetContrast(uint8_t contrast)
{
    SSD1309_Command(0x81);
    SSD1309_Command(contrast);
}

void SSD1309_Invert(uint8_t invert)
{
    if (invert)
    {
        SSD1309_Command(0xA7);  // inverted display
    }
    else
    {
        SSD1309_Command(0xA6);  // normal display
    }
}

void SSD1309_Start(void)
{
    OLED_CS_Write(1);
    OLED_DC_Write(0);
    OLED_RST_Write(1);

    SPIM_1_Start();

    SSD1309_Reset();

    /*
     * SSD1309 128x64 initialization.
     * This sequence works for many SPI SSD1309 OLED modules.
     */
    SSD1309_Command(0xAE);        // Display OFF

    SSD1309_Command(0xD5);        // Set display clock divide ratio / oscillator frequency
    SSD1309_Command(0x80);

    SSD1309_Command(0xA8);        // Set multiplex ratio
    SSD1309_Command(0x3F);        // 1/64 duty

    SSD1309_Command(0xD3);        // Set display offset
    SSD1309_Command(0x00);

    SSD1309_Command(0x40);        // Set display start line to 0

    /*
     * Some SSD1309 modules use an external charge pump / booster circuit.
     * 0x8D/0x14 works on many modules, but if the display stays blank,
     * try commenting these two lines out or using 0x8D, 0x10.
     */
    SSD1309_Command(0x8D);        // Charge pump setting
    SSD1309_Command(0x14);        // Enable charge pump

    SSD1309_Command(0x20);        // Memory addressing mode
    SSD1309_Command(0x00);        // Horizontal addressing mode

    SSD1309_Command(0xA1);        // Segment remap: column address 127 mapped to SEG0
    SSD1309_Command(0xC8);        // COM scan direction remapped

    SSD1309_Command(0xDA);        // COM pins hardware configuration
    SSD1309_Command(0x12);

    SSD1309_Command(0x81);        // Contrast control
    SSD1309_Command(0x7F);

    SSD1309_Command(0xD9);        // Pre-charge period
    SSD1309_Command(0xF1);

    SSD1309_Command(0xDB);        // VCOMH deselect level
    SSD1309_Command(0x40);

    SSD1309_Command(0xA4);        // Display follows RAM content
    SSD1309_Command(0xA6);        // Normal display

    SSD1309_Clear();
    SSD1309_Update();

    SSD1309_Command(0xAF);        // Display ON
}
