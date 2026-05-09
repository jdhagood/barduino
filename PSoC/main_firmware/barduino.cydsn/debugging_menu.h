#ifndef DEBUGGING_MENU_H
#define DEBUGGING_MENU_H

#include <project.h>
#include <stdint.h>

/*
 * Initializes the USB debug command layer.
 * USBSerial_Init() should already have been called in main.
 */
void DebuggingMenu_Init(void);

/*
 * Call repeatedly in main loop.
 * This checks USB serial for complete commands and handles them.
 */
void DebuggingMenu_Update(void);

/*
 * Directly handle a command string.
 * Useful for testing.
 */
void DebuggingMenu_HandleCommand(const char *cmd);

#endif