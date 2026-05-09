#include "ui.h"
#include "ssd1309.h"
#include "ssd1309_gfx.h"
#include "keypad.h"
#include "ui_screens_main.h"

static const ui_screen_t *screen_stack[UI_STACK_DEPTH];
static uint8_t stack_top = 0u;

static uint8_t needs_redraw = 1u;

static const ui_screen_t *UI_CurrentScreen(void)
{
    if (stack_top == 0u)
    {
        return 0;
    }

    return screen_stack[stack_top - 1u];
}

void UI_Init(void)
{
    stack_top = 0u;
    needs_redraw = 1u;

    SSD1309_Start();
    SSD1309_Clear();
    SSD1309_Update();

    UI_PushScreen(MainMenu_GetScreen());
}

void UI_Update(void)
{
    const ui_screen_t *screen;

    Keypad_Update();

    if (Keypad_WasPressed(BUTTON_BACK))
    {
        if (UI_CanGoBack())
        {
            UI_PopScreen();
            needs_redraw = 1u;
            return;
        }
    }

    screen = UI_CurrentScreen();

    if ((screen != 0) && (screen->update != 0))
    {
        screen->update();
    }

    needs_redraw = 1u;
}

void UI_Draw(void)
{
    const ui_screen_t *screen;

    if (!needs_redraw)
    {
        return;
    }

    screen = UI_CurrentScreen();

    SSD1309_Clear();

    if ((screen != 0) && (screen->draw != 0))
    {
        screen->draw();
    }

    SSD1309_Update();

    needs_redraw = 0u;
}

void UI_PushScreen(const ui_screen_t *screen)
{
    if ((screen == 0) || (stack_top >= UI_STACK_DEPTH))
    {
        return;
    }

    screen_stack[stack_top] = screen;
    stack_top++;

    if (screen->on_enter != 0)
    {
        screen->on_enter();
    }

    needs_redraw = 1u;
}

void UI_PopScreen(void)
{
    const ui_screen_t *old_screen;

    if (stack_top <= 1u)
    {
        return;
    }

    old_screen = screen_stack[stack_top - 1u];

    if ((old_screen != 0) && (old_screen->on_exit != 0))
    {
        old_screen->on_exit();
    }

    stack_top--;
    needs_redraw = 1u;
}

void UI_SetScreen(const ui_screen_t *screen)
{
    const ui_screen_t *old_screen;

    if (screen == 0)
    {
        return;
    }

    old_screen = UI_CurrentScreen();

    if ((old_screen != 0) && (old_screen->on_exit != 0))
    {
        old_screen->on_exit();
    }

    stack_top = 0u;
    UI_PushScreen(screen);
}

uint8_t UI_CanGoBack(void)
{
    return (stack_top > 1u) ? 1u : 0u;
}