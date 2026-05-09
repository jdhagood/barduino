#ifndef UI_H
#define UI_H

#include <stdint.h>
#include "ui_screen.h"

#define UI_STACK_DEPTH 8u

void UI_Init(void);
void UI_Update(void);
void UI_Draw(void);

void UI_PushScreen(const ui_screen_t *screen);
void UI_PopScreen(void);
void UI_SetScreen(const ui_screen_t *screen);

uint8_t UI_CanGoBack(void);

#endif