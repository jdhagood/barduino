#include "ui_screens_make_drinks.h"
#include "ui_menu.h"
#include "pumps.h"
#include "usb_serial.h"

static ui_menu_t drinks_menu;

static void make_drink_1(void)
{
    USBSerial_WriteLine("Making drink 1");

    Pump_DispenseNonBlocking(PUMP_0, 3000u, 250u);
    Pump_DispenseNonBlocking(PUMP_1, 1500u, 250u);
}

static void make_drink_2(void)
{
    USBSerial_WriteLine("Making drink 2");

    Pump_DispenseNonBlocking(PUMP_2, 2500u, 250u);
    Pump_DispenseNonBlocking(PUMP_3, 2000u, 250u);
}

static const ui_menu_item_t drink_items[] =
{
    {"Drink 1", make_drink_1},
    {"Drink 2", make_drink_2}
};

static void on_enter(void)
{
    UI_MenuInit(&drinks_menu, "Make Drinks", drink_items, 2u);
}

static void update(void)
{
    UI_MenuUpdate(&drinks_menu);
}

static void draw(void)
{
    UI_MenuDraw(&drinks_menu);
}

static const ui_screen_t screen =
{
    on_enter,
    0,
    update,
    draw
};

const ui_screen_t *MakeDrinks_GetScreen(void)
{
    return &screen;
}