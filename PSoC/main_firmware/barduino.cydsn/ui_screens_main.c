#include "ui_screens_main.h"
#include "ui.h"
#include "ui_menu.h"
#include "ui_screens_make_drinks.h"
#include "ui_screens_drink_config.h"
#include "ui_screens_info.h"

static ui_menu_t main_menu;

static void open_make_drinks(void)
{
    UI_PushScreen(MakeDrinks_GetScreen());
}

static void open_drink_config(void)
{
    UI_PushScreen(DrinkConfig_GetScreen());
}

static void open_info(void)
{
    UI_PushScreen(Info_GetScreen());
}

static const ui_menu_item_t main_items[] =
{
    {"Make Drinks",  open_make_drinks},
    {"Drink Config", open_drink_config},
    {"Info",         open_info}
};

static void main_on_enter(void)
{
    UI_MenuInit(&main_menu, "Main Menu", main_items, 3u);
}

static void main_update(void)
{
    UI_MenuUpdate(&main_menu);
}

static void main_draw(void)
{
    UI_MenuDraw(&main_menu);
}

static const ui_screen_t main_screen =
{
    main_on_enter,
    0,
    main_update,
    main_draw
};

const ui_screen_t *MainMenu_GetScreen(void)
{
    return &main_screen;
}