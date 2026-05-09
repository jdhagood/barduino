#include "ui_screens_info.h"
#include "ssd1309_gfx.h"
#include "ir_sensors.h"
#include "pumps.h"
#include "tmc2209_stepper.h"
#include <stdio.h>

static void update(void)
{
    /*
     * Nothing needed for now.
     * Dynamic values are drawn every UI refresh.
     */
}

static void draw(void)
{
    char line[24];

    SSD1309_SetCursor(0, 0);
    SSD1309_WriteString("Info", SSD1309_COLOR_WHITE);
    SSD1309_DrawFastHLine(0, 9, 128, SSD1309_COLOR_WHITE);

    SSD1309_SetCursor(0, 14);
    sprintf(line, "IR: 0x%02X", IRSensors_GetActiveMask());
    SSD1309_WriteString(line, SSD1309_COLOR_WHITE);

    SSD1309_SetCursor(0, 26);
    sprintf(line, "Stepper: %s", TMC2209_IsBusy() ? "busy" : "idle");
    SSD1309_WriteString(line, SSD1309_COLOR_WHITE);

    SSD1309_SetCursor(0, 38);
    sprintf(line, "P0: %s", Pump_IsBusy(PUMP_0) ? "busy" : "idle");
    SSD1309_WriteString(line, SSD1309_COLOR_WHITE);

    SSD1309_SetCursor(0, 56);
    SSD1309_WriteString("BACK to return", SSD1309_COLOR_WHITE);
}

static const ui_screen_t screen =
{
    0,
    0,
    update,
    draw
};

const ui_screen_t *Info_GetScreen(void)
{
    return &screen;
}