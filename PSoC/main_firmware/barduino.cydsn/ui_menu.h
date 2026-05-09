#ifndef UI_MENU_H
#define UI_MENU_H

#include <stdint.h>

typedef void (*ui_menu_action_t)(void);

typedef struct
{
    const char *label;
    ui_menu_action_t action;
} ui_menu_item_t;

typedef struct
{
    const char *title;
    const ui_menu_item_t *items;
    uint8_t item_count;
    uint8_t selected_index;
    uint8_t top_visible_index;
} ui_menu_t;

void UI_MenuInit(ui_menu_t *menu,
                 const char *title,
                 const ui_menu_item_t *items,
                 uint8_t item_count);

void UI_MenuUpdate(ui_menu_t *menu);
void UI_MenuDraw(const ui_menu_t *menu);

#endif