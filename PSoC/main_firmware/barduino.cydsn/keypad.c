#include "keypad.h"

/*
 * Button bit assignments in btn_status_reg.
 *
 * Raw hardware:
 *     0 = pressed
 *     1 = released
 *
 * Internal debounced representation:
 *     1 = pressed
 *     0 = released
 */
#define BUTTON_UP_MASK       (1u << BUTTON_UP)
#define BUTTON_DOWN_MASK     (1u << BUTTON_DOWN)
#define BUTTON_LEFT_MASK     (1u << BUTTON_LEFT)
#define BUTTON_RIGHT_MASK    (1u << BUTTON_RIGHT)
#define BUTTON_ENTER_MASK    (1u << BUTTON_ENTER)
#define BUTTON_BACK_MASK     (1u << BUTTON_BACK)

#define BUTTON_ALL_MASK      ((1u << BUTTON_COUNT) - 1u)

/*
 * Debounce time in update samples.
 *
 * If Keypad_Update() is called every 5 ms:
 *     4 samples = 20 ms debounce
 *
 * If Keypad_Update() is called every 1 ms:
 *     20 samples = 20 ms debounce
 *
 * Choose this based on your main-loop timing.
 */
#define KEYPAD_DEBOUNCE_SAMPLES  4u

static uint8_t stable_pressed_mask = 0u;
static uint8_t previous_stable_pressed_mask = 0u;

static uint8_t press_event_mask = 0u;
static uint8_t release_event_mask = 0u;

static uint8_t candidate_pressed_mask = 0u;
static uint8_t debounce_count = 0u;

static uint8_t read_raw_pressed_mask(void)
{
    uint8_t raw;

    /*
     * btn_status_reg_Read() returns the raw hardware level.
     *
     * Because the buttons use pullups:
     *     raw bit = 1 means released
     *     raw bit = 0 means pressed
     *
     * Invert it so internally:
     *     bit = 1 means pressed
     */
    raw = btn_status_reg_Read();

    return ((uint8_t)(~raw)) & BUTTON_ALL_MASK;
}

static uint8_t is_valid_button(button_t button)
{
    return ((uint8_t)button < BUTTON_COUNT);
}

void Keypad_Init(void)
{
    stable_pressed_mask = read_raw_pressed_mask();
    previous_stable_pressed_mask = stable_pressed_mask;

    candidate_pressed_mask = stable_pressed_mask;
    debounce_count = 0u;

    press_event_mask = 0u;
    release_event_mask = 0u;
}

void Keypad_Update(void)
{
    uint8_t raw_pressed_mask;
    uint8_t changed_mask;

    raw_pressed_mask = read_raw_pressed_mask();

    /*
     * If the raw reading matches the current candidate, count how long
     * it has stayed there. Once it remains stable for enough samples,
     * accept it as the new debounced state.
     */
    if (raw_pressed_mask == candidate_pressed_mask)
    {
        if (debounce_count < KEYPAD_DEBOUNCE_SAMPLES)
        {
            debounce_count++;
        }
    }
    else
    {
        candidate_pressed_mask = raw_pressed_mask;
        debounce_count = 0u;
    }

    if (debounce_count >= KEYPAD_DEBOUNCE_SAMPLES)
    {
        if (stable_pressed_mask != candidate_pressed_mask)
        {
            previous_stable_pressed_mask = stable_pressed_mask;
            stable_pressed_mask = candidate_pressed_mask;

            changed_mask = stable_pressed_mask ^ previous_stable_pressed_mask;

            /*
             * Rising edge in internal pressed mask:
             *     released -> pressed
             */
            press_event_mask |= changed_mask & stable_pressed_mask;

            /*
             * Falling edge in internal pressed mask:
             *     pressed -> released
             */
            release_event_mask |= changed_mask & previous_stable_pressed_mask;
        }
    }
}

uint8_t Keypad_IsPressed(button_t button)
{
    if (!is_valid_button(button))
    {
        return 0u;
    }

    return (stable_pressed_mask & (1u << button)) ? 1u : 0u;
}

uint8_t Keypad_WasPressed(button_t button)
{
    uint8_t mask;

    if (!is_valid_button(button))
    {
        return 0u;
    }

    mask = (uint8_t)(1u << button);

    if (press_event_mask & mask)
    {
        /*
         * Consume the event so this press only reports once.
         */
        press_event_mask &= (uint8_t)(~mask);
        return 1u;
    }

    return 0u;
}

uint8_t Keypad_WasReleased(button_t button)
{
    uint8_t mask;

    if (!is_valid_button(button))
    {
        return 0u;
    }

    mask = (uint8_t)(1u << button);

    if (release_event_mask & mask)
    {
        /*
         * Consume the event so this release only reports once.
         */
        release_event_mask &= (uint8_t)(~mask);
        return 1u;
    }

    return 0u;
}

uint8_t Keypad_GetPressedMask(void)
{
    return stable_pressed_mask;
}

void Keypad_ClearEvents(void)
{
    press_event_mask = 0u;
    release_event_mask = 0u;
}