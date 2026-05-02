#ifndef KEYPAD_H
#define KEYPAD_H

#include <project.h>
#include <stdint.h>

typedef enum
{
    BUTTON_UP = 0,
    BUTTON_DOWN,
    BUTTON_LEFT,
    BUTTON_RIGHT,
    BUTTON_ENTER,
    BUTTON_BACK,
    BUTTON_COUNT
} button_t;

/*
 * Call once during startup.
 */
void Keypad_Init(void);

/*
 * Call periodically from the main loop.
 *
 * Recommended call rate:
 *     every 1 ms to 10 ms
 *
 * Example:
 *     for (;;) {
 *         Keypad_Update();
 *         ...
 *         CyDelay(5);
 *     }
 */
void Keypad_Update(void);

/*
 * Returns 1 while the button is currently debounced as pressed.
 * Returns 0 otherwise.
 *
 * This is level-based.
 */
uint8_t Keypad_IsPressed(button_t button);

/*
 * Returns 1 once when the button transitions from released to pressed.
 *
 * This is edge-based and is what you usually want for UI menu actions.
 * It will not return 1 again until the button is released and pressed again.
 */
uint8_t Keypad_WasPressed(button_t button);

/*
 * Returns 1 once when the button transitions from pressed to released.
 */
uint8_t Keypad_WasReleased(button_t button);

/*
 * Returns the current debounced button state as a bitmask.
 *
 * bit = 1 means pressed.
 */
uint8_t Keypad_GetPressedMask(void);

/*
 * Clears one-shot press/release events.
 * Usually not needed unless you want to discard queued button events.
 */
void Keypad_ClearEvents(void);

#endif