#include "ui_screens_drink_config.h"
#include "ui.h"
#include "ui_screen.h"
#include "ssd1309_gfx.h"
#include "keypad.h"
#include "drink_db.h"
#include "usb_serial.h"

#include <string.h>
#include <stdio.h>

#define MENU_VISIBLE_ROWS        5u
#define MENU_Y_START             14u
#define MENU_LINE_HEIGHT         10u

#define NAME_ENTRY_MAX_LEN       DRINK_DB_NAME_MAX_LEN

typedef enum
{
    CONFIG_STATE_MAIN_MENU = 0,
    CONFIG_STATE_SELECT_EXISTING,
    CONFIG_STATE_NAME_ENTRY,
    CONFIG_STATE_EDIT_AMOUNTS
} drink_config_state_t;

typedef enum
{
    CHAR_MODE_LOWER = 0,
    CHAR_MODE_UPPER,
    CHAR_MODE_NUMBER
} char_mode_t;

static drink_config_state_t state = CONFIG_STATE_MAIN_MENU;

/*
 * Main Drink Config menu:
 *   0 = Edit Existing
 *   1 = Make New
 */
static uint8_t main_selected = 0u;

/*
 * Existing drink selection.
 */
static uint8_t drink_selected = 0u;
static uint8_t drink_page_start = 0u;

/*
 * Name entry for Make New.
 */
static char new_name[NAME_ENTRY_MAX_LEN + 1u];
static uint8_t name_len = 0u;
static char current_char = 'a';
static char_mode_t char_mode = CHAR_MODE_LOWER;

/*
 * Amount editor.
 */
static drink_t edit_drink;
static uint8_t editing_existing = 0u;
static uint8_t editing_index = 0u;
static uint8_t selected_pump = 0u;

static void reset_to_main_menu(void)
{
    state = CONFIG_STATE_MAIN_MENU;
    main_selected = 0u;
}

static void start_edit_existing_select(void)
{
    state = CONFIG_STATE_SELECT_EXISTING;
    drink_selected = 0u;
    drink_page_start = 0u;
}

static void start_make_new_name_entry(void)
{
    state = CONFIG_STATE_NAME_ENTRY;

    memset(new_name, 0, sizeof(new_name));
    name_len = 0u;

    char_mode = CHAR_MODE_LOWER;
    current_char = 'a';
}

static void start_edit_amounts_for_existing(uint8_t drink_index)
{
    const drink_t *drink = DrinkDB_GetDrink(drink_index);

    if (drink == 0)
    {
        return;
    }

    edit_drink = *drink;
    editing_existing = 1u;
    editing_index = drink_index;
    selected_pump = 0u;

    state = CONFIG_STATE_EDIT_AMOUNTS;
}

static void start_edit_amounts_for_new(void)
{
    memset(&edit_drink, 0, sizeof(edit_drink));

    strncpy(edit_drink.name, new_name, DRINK_DB_NAME_MAX_LEN);
    edit_drink.name[DRINK_DB_NAME_MAX_LEN] = '\0';

    for (uint8_t i = 0u; i < DRINK_DB_PUMP_COUNT; i++)
    {
        edit_drink.amount[i] = 0u;
    }

    edit_drink.valid = 1u;

    editing_existing = 0u;
    editing_index = 0u;
    selected_pump = 0u;

    state = CONFIG_STATE_EDIT_AMOUNTS;
}

static char first_char_for_mode(char_mode_t mode)
{
    switch (mode)
    {
        case CHAR_MODE_LOWER:
            return 'a';

        case CHAR_MODE_UPPER:
            return 'A';

        case CHAR_MODE_NUMBER:
            return '0';

        default:
            return 'a';
    }
}

static const char *mode_name(char_mode_t mode)
{
    switch (mode)
    {
        case CHAR_MODE_LOWER:
            return "lower";

        case CHAR_MODE_UPPER:
            return "UPPER";

        case CHAR_MODE_NUMBER:
            return "123";

        default:
            return "?";
    }
}

static char increment_char(char c, char_mode_t mode)
{
    switch (mode)
    {
        case CHAR_MODE_LOWER:
            if (c < 'a' || c > 'z') return 'a';
            return (c == 'z') ? 'a' : (char)(c + 1);

        case CHAR_MODE_UPPER:
            if (c < 'A' || c > 'Z') return 'A';
            return (c == 'Z') ? 'A' : (char)(c + 1);

        case CHAR_MODE_NUMBER:
            if (c < '0' || c > '9') return '0';
            return (c == '9') ? '0' : (char)(c + 1);

        default:
            return 'a';
    }
}

static char decrement_char(char c, char_mode_t mode)
{
    switch (mode)
    {
        case CHAR_MODE_LOWER:
            if (c < 'a' || c > 'z') return 'a';
            return (c == 'a') ? 'z' : (char)(c - 1);

        case CHAR_MODE_UPPER:
            if (c < 'A' || c > 'Z') return 'A';
            return (c == 'A') ? 'Z' : (char)(c - 1);

        case CHAR_MODE_NUMBER:
            if (c < '0' || c > '9') return '0';
            return (c == '0') ? '9' : (char)(c - 1);

        default:
            return 'a';
    }
}

static void cycle_char_mode_forward(void)
{
    if (char_mode == CHAR_MODE_NUMBER)
    {
        char_mode = CHAR_MODE_LOWER;
    }
    else
    {
        char_mode = (char_mode_t)(char_mode + 1u);
    }

    current_char = first_char_for_mode(char_mode);
}

static void cycle_char_mode_backward(void)
{
    if (char_mode == CHAR_MODE_LOWER)
    {
        char_mode = CHAR_MODE_NUMBER;
    }
    else
    {
        char_mode = (char_mode_t)(char_mode - 1u);
    }

    current_char = first_char_for_mode(char_mode);
}

static void update_main_menu(void)
{
    if (Keypad_WasPressed(BUTTON_UP) || Keypad_WasPressed(BUTTON_DOWN))
    {
        main_selected = (main_selected == 0u) ? 1u : 0u;
    }

    if (Keypad_WasPressed(BUTTON_ENTER))
    {
        if (main_selected == 0u)
        {
            start_edit_existing_select();
        }
        else
        {
            start_make_new_name_entry();
        }
    }
}

static void update_select_existing(void)
{
    uint8_t count = DrinkDB_GetCount();

    if (count == 0u)
    {
        if (Keypad_WasPressed(BUTTON_BACK))
        {
            reset_to_main_menu();
        }
        return;
    }

    if (Keypad_WasPressed(BUTTON_UP))
    {
        if (drink_selected > 0u)
        {
            drink_selected--;
        }

        if (drink_selected < drink_page_start)
        {
            if (drink_page_start >= MENU_VISIBLE_ROWS)
            {
                drink_page_start -= MENU_VISIBLE_ROWS;
                drink_selected = drink_page_start + MENU_VISIBLE_ROWS - 1u;

                if (drink_selected >= count)
                {
                    drink_selected = count - 1u;
                }
            }
            else
            {
                drink_page_start = 0u;
            }
        }
    }

    if (Keypad_WasPressed(BUTTON_DOWN))
    {
        if (drink_selected + 1u < count)
        {
            drink_selected++;
        }

        /*
         * Page behavior requested:
         * when the cursor is at the bottom and DOWN is pressed,
         * show the next set and put the arrow back at the top.
         */
        if (drink_selected >= drink_page_start + MENU_VISIBLE_ROWS)
        {
            drink_page_start += MENU_VISIBLE_ROWS;
            drink_selected = drink_page_start;

            if (drink_selected >= count)
            {
                drink_selected = count - 1u;
            }
        }
    }

    if (Keypad_WasPressed(BUTTON_ENTER))
    {
        start_edit_amounts_for_existing(drink_selected);
    }

    if (Keypad_WasPressed(BUTTON_BACK))
    {
        reset_to_main_menu();
    }
}

static void update_name_entry(void)
{
    if (Keypad_WasPressed(BUTTON_UP))
    {
        current_char = increment_char(current_char, char_mode);
    }

    if (Keypad_WasPressed(BUTTON_DOWN))
    {
        current_char = decrement_char(current_char, char_mode);
    }

    if (Keypad_WasPressed(BUTTON_RIGHT))
    {
        cycle_char_mode_forward();
    }

    if (Keypad_WasPressed(BUTTON_LEFT))
    {
        cycle_char_mode_backward();
    }

    if (Keypad_WasPressed(BUTTON_ENTER))
    {
        if (name_len < NAME_ENTRY_MAX_LEN)
        {
            new_name[name_len] = current_char;
            name_len++;
            new_name[name_len] = '\0';
        }
        else
        {
            /*
             * If full and ENTER is pressed again, accept the name.
             */
            start_edit_amounts_for_new();
        }
    }

    /*
     * BACK deletes. If empty, return to Drink Config menu.
     */
    if (Keypad_WasPressed(BUTTON_BACK))
    {
        if (name_len > 0u)
        {
            name_len--;
            new_name[name_len] = '\0';
        }
        else
        {
            reset_to_main_menu();
        }
    }

    /*
     * Name is entered when user presses ENTER after at least one char?
     *
     * Because ENTER is also used to add characters, use this convention:
     * hold character limit to 20, then ENTER at full name accepts.
     *
     * Better practical option:
     * use RIGHT to switch modes, UP/DOWN chars, ENTER append,
     * and add a selectable DONE character. But with only the requested controls,
     * we need a way to finish.
     *
     * The simplest usable finish rule:
     * if user enters at least one char and presses ENTER while current_char is '_',
     * save. But '_' is not currently in the character set.
     *
     * Instead, use LEFT+RIGHT not available simultaneously.
     *
     * Recommended change:
     * ENTER appends a char, BACK deletes/goes back, and when char limit reached,
     * proceeds automatically. If you want ENTER to finish name immediately,
     * we need a separate "confirm" gesture or a DONE symbol.
     */
}

/*
 * Improved name entry using a DONE sentinel.
 *
 * To make name entry usable, we treat current_char = '#' in NUMBER mode as DONE.
 * It appears after 9. Press ENTER on DONE to accept the name.
 */
static char increment_char_with_done(char c, char_mode_t mode)
{
    if (mode == CHAR_MODE_NUMBER)
    {
        if (c < '0' || c > '9')
        {
            if (c == '#') return '0';
            return '0';
        }

        if (c == '9') return '#';
        return (char)(c + 1);
    }

    return increment_char(c, mode);
}

static char decrement_char_with_done(char c, char_mode_t mode)
{
    if (mode == CHAR_MODE_NUMBER)
    {
        if (c == '#') return '9';
        if (c < '0' || c > '9') return '#';
        if (c == '0') return '#';
        return (char)(c - 1);
    }

    return decrement_char(c, mode);
}

static void update_name_entry_better(void)
{
    if (Keypad_WasPressed(BUTTON_UP))
    {
        current_char = increment_char_with_done(current_char, char_mode);
    }

    if (Keypad_WasPressed(BUTTON_DOWN))
    {
        current_char = decrement_char_with_done(current_char, char_mode);
    }

    if (Keypad_WasPressed(BUTTON_RIGHT))
    {
        cycle_char_mode_forward();
    }

    if (Keypad_WasPressed(BUTTON_LEFT))
    {
        cycle_char_mode_backward();
    }

    if (Keypad_WasPressed(BUTTON_ENTER))
    {
        /*
         * In number mode, '#' means DONE.
         */
        if ((char_mode == CHAR_MODE_NUMBER) && (current_char == '#'))
        {
            if (name_len == 0u)
            {
                return;
            }

            start_edit_amounts_for_new();
            return;
        }

        if (name_len < NAME_ENTRY_MAX_LEN)
        {
            new_name[name_len] = current_char;
            name_len++;
            new_name[name_len] = '\0';
        }
    }

    if (Keypad_WasPressed(BUTTON_BACK))
    {
        if (name_len > 0u)
        {
            name_len--;
            new_name[name_len] = '\0';
        }
        else
        {
            reset_to_main_menu();
        }
    }
}

static void update_edit_amounts(void)
{
    if (Keypad_WasPressed(BUTTON_UP))
    {
        if (edit_drink.amount[selected_pump] < 9u)
        {
            edit_drink.amount[selected_pump]++;
        }
    }

    if (Keypad_WasPressed(BUTTON_DOWN))
    {
        if (edit_drink.amount[selected_pump] > 0u)
        {
            edit_drink.amount[selected_pump]--;
        }
    }

    if (Keypad_WasPressed(BUTTON_LEFT))
    {
        if (selected_pump > 0u)
        {
            selected_pump--;
        }
    }

    if (Keypad_WasPressed(BUTTON_RIGHT))
    {
        if (selected_pump + 1u < DRINK_DB_PUMP_COUNT)
        {
            selected_pump++;
        }
    }

    /*
     * ENTER saves.
     */
    if (Keypad_WasPressed(BUTTON_ENTER))
    {
        if (editing_existing)
        {
            DrinkDB_UpdateDrink(editing_index, &edit_drink);
            USBSerial_Printf("Saved drink: %s\r\n", edit_drink.name);
        }
        else
        {
            if (DrinkDB_AddDrink(&edit_drink))
            {
                USBSerial_Printf("Added drink: %s\r\n", edit_drink.name);
            }
            else
            {
                USBSerial_WriteLine("Drink DB full");
            }
        }

        reset_to_main_menu();
    }

    /*
     * BACK quits without saving.
     */
    if (Keypad_WasPressed(BUTTON_BACK))
    {
        reset_to_main_menu();
    }
}

static void draw_header(const char *title)
{
    SSD1309_SetCursor(0, 0);
    SSD1309_WriteString(title, SSD1309_COLOR_WHITE);
    SSD1309_DrawFastHLine(0, 9, 128, SSD1309_COLOR_WHITE);
}

static void draw_main_menu(void)
{
    draw_header("Drink Config");

    SSD1309_SetCursor(0, 16);
    SSD1309_WriteString(main_selected == 0u ? ">" : " ", SSD1309_COLOR_WHITE);
    SSD1309_SetCursor(10, 16);
    SSD1309_WriteString("Edit Existing", SSD1309_COLOR_WHITE);

    SSD1309_SetCursor(0, 28);
    SSD1309_WriteString(main_selected == 1u ? ">" : " ", SSD1309_COLOR_WHITE);
    SSD1309_SetCursor(10, 28);
    SSD1309_WriteString("Make New", SSD1309_COLOR_WHITE);
}

static void draw_select_existing(void)
{
    uint8_t count = DrinkDB_GetCount();
    char line[24];

    draw_header("Select Drink");

    if (count == 0u)
    {
        SSD1309_SetCursor(0, 18);
        SSD1309_WriteString("No drinks saved", SSD1309_COLOR_WHITE);

        SSD1309_SetCursor(0, 52);
        SSD1309_WriteString("BACK", SSD1309_COLOR_WHITE);
        return;
    }

    for (uint8_t row = 0u; row < MENU_VISIBLE_ROWS; row++)
    {
        uint8_t index = drink_page_start + row;
        const drink_t *drink;

        if (index >= count)
        {
            break;
        }

        drink = DrinkDB_GetDrink(index);

        SSD1309_SetCursor(0, MENU_Y_START + row * MENU_LINE_HEIGHT);
        SSD1309_WriteString(index == drink_selected ? ">" : " ", SSD1309_COLOR_WHITE);

        SSD1309_SetCursor(10, MENU_Y_START + row * MENU_LINE_HEIGHT);
        SSD1309_WriteString(drink->name, SSD1309_COLOR_WHITE);
    }

    sprintf(line, "%u/%u", (uint8_t)(drink_selected + 1u), count);
    SSD1309_SetCursor(98, 56);
    SSD1309_WriteString(line, SSD1309_COLOR_WHITE);
}

static void draw_name_entry(void)
{
    char line[24];

    draw_header("New Drink Name");

    SSD1309_SetCursor(0, 14);
    SSD1309_WriteString(new_name, SSD1309_COLOR_WHITE);

    SSD1309_DrawRect(0, 26, 128, 16, SSD1309_COLOR_WHITE);

    SSD1309_SetCursor(6, 30);
    if ((char_mode == CHAR_MODE_NUMBER) && (current_char == '#'))
    {
        SSD1309_WriteString("DONE", SSD1309_COLOR_WHITE);
    }
    else
    {
        char cstr[2];
        cstr[0] = current_char;
        cstr[1] = '\0';
        SSD1309_WriteString(cstr, SSD1309_COLOR_WHITE);
    }

    SSD1309_SetCursor(50, 30);
    SSD1309_WriteString(mode_name(char_mode), SSD1309_COLOR_WHITE);

    sprintf(line, "%u/%u", name_len, NAME_ENTRY_MAX_LEN);
    SSD1309_SetCursor(96, 30);
    SSD1309_WriteString(line, SSD1309_COLOR_WHITE);

    SSD1309_SetCursor(0, 48);
    SSD1309_WriteString("UP/DN char", SSD1309_COLOR_WHITE);

    SSD1309_SetCursor(0, 56);
    SSD1309_WriteString("L/R mode #=done", SSD1309_COLOR_WHITE);
}

static void draw_edit_amounts(void)
{
    char line[24];

    draw_header(editing_existing ? "Edit Drink" : "New Amounts");

    SSD1309_SetCursor(0, 12);
    SSD1309_WriteString(edit_drink.name, SSD1309_COLOR_WHITE);

    for (uint8_t i = 0u; i < DRINK_DB_PUMP_COUNT; i++)
    {
        uint8_t y = 24u + i * 7u;

        if (i == selected_pump)
        {
            SSD1309_SetCursor(0, y);
            SSD1309_WriteString(">", SSD1309_COLOR_WHITE);
        }

        sprintf(line, "Pump %u: %u", i, edit_drink.amount[i]);
        SSD1309_SetCursor(10, y);
        SSD1309_WriteString(line, SSD1309_COLOR_WHITE);
    }

    SSD1309_SetCursor(80, 56);
    SSD1309_WriteString("ENT save", SSD1309_COLOR_WHITE);
}

static void on_enter(void)
{
    reset_to_main_menu();
}

static void update(void)
{
    switch (state)
    {
        case CONFIG_STATE_MAIN_MENU:
            update_main_menu();
            break;

        case CONFIG_STATE_SELECT_EXISTING:
            update_select_existing();
            break;

        case CONFIG_STATE_NAME_ENTRY:
            update_name_entry_better();
            break;

        case CONFIG_STATE_EDIT_AMOUNTS:
            update_edit_amounts();
            break;

        default:
            reset_to_main_menu();
            break;
    }
}

static void draw(void)
{
    switch (state)
    {
        case CONFIG_STATE_MAIN_MENU:
            draw_main_menu();
            break;

        case CONFIG_STATE_SELECT_EXISTING:
            draw_select_existing();
            break;

        case CONFIG_STATE_NAME_ENTRY:
            draw_name_entry();
            break;

        case CONFIG_STATE_EDIT_AMOUNTS:
            draw_edit_amounts();
            break;

        default:
            draw_main_menu();
            break;
    }
}

static const ui_screen_t screen =
{
    on_enter,
    0,
    update,
    draw
};

const ui_screen_t *DrinkConfig_GetScreen(void)
{
    return &screen;
}