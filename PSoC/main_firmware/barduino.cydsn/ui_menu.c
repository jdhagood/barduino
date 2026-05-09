#include "ui_menu.h"
#include "keypad.h"
#include "ssd1309_gfx.h"

#define UI_MENU_VISIBLE_ITEMS 5u
#define UI_MENU_LINE_HEIGHT   10u

void UI_MenuInit(ui_menu_t *menu,
                 const char *title,
                 const ui_menu_item_t *items,
                 uint8_t item_count)
{
    menu->title = title;
    menu->items = items;
    menu->item_count = item_count;
    menu->selected_index = 0u;
    menu->top_visible_index = 0u;
}

void UI_MenuUpdate(ui_menu_t *menu)
{
    if (menu == 0 || menu->item_count == 0u)
    {
        return;
    }

    if (Keypad_WasPressed(BUTTON_UP))
    {
        if (menu->selected_index > 0u)
        {
            menu->selected_index--;
        }

        if (menu->selected_index < menu->top_visible_index)
        {
            menu->top_visible_index = menu->selected_index;
        }
    }

    if (Keypad_WasPressed(BUTTON_DOWN))
    {
        if (menu->selected_index + 1u < menu->item_count)
        {
            menu->selected_index++;
        }

        if (menu->selected_index >= menu->top_visible_index + UI_MENU_VISIBLE_ITEMS)
        {
            menu->top_visible_index++;
        }
    }

    if (Keypad_WasPressed(BUTTON_ENTER))
    {
        ui_menu_action_t action = menu->items[menu->selected_index].action;

        if (action != 0)
        {
            action();
        }
    }
}

void UI_MenuDraw(const ui_menu_t *menu)
{
    uint8_t y = 0u;

    if (menu == 0)
    {
        return;
    }

    SSD1309_SetCursor(0, 0);
    SSD1309_WriteString(menu->title, SSD1309_COLOR_WHITE);

    SSD1309_DrawFastHLine(0, 9, 128, SSD1309_COLOR_WHITE);

    for (uint8_t row = 0u; row < UI_MENU_VISIBLE_ITEMS; row++)
    {
        uint8_t item_index = menu->top_visible_index + row;

        if (item_index >= menu->item_count)
        {
            break;
        }

        y = 12u + row * UI_MENU_LINE_HEIGHT;

        if (item_index == menu->selected_index)
        {
            SSD1309_SetCursor(0, y);
            SSD1309_WriteString(">", SSD1309_COLOR_WHITE);
        }

        SSD1309_SetCursor(10, y);
        SSD1309_WriteString(menu->items[item_index].label, SSD1309_COLOR_WHITE);
    }
}