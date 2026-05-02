#include <project.h>

#include "tmc2209_stepper.h"
#include "ssd1309.h"
#include "ssd1309_gfx.h"
#include "pumps.h"
#include "usb_serial.h"
#include "keypad.h"
#include "ir_sensors.h"
#include "neopixel.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


static void PrintHelp(void)
{
    USBSerial_WriteLine("Commands:");
    USBSerial_WriteLine("  help");
    USBSerial_WriteLine("  status");
    USBSerial_WriteLine("");
    USBSerial_WriteLine("Pump commands:");
    USBSerial_WriteLine("  pump <n> forward");
    USBSerial_WriteLine("  pump <n> reverse");
    USBSerial_WriteLine("  pump <n> stop");
    USBSerial_WriteLine("  pump <n> dispense <ms> <reverse_ms>");
    USBSerial_WriteLine("");
    USBSerial_WriteLine("IR commands:");
    USBSerial_WriteLine("  ir readings");
    USBSerial_WriteLine("");
    USBSerial_WriteLine("Stepper commands:");
    USBSerial_WriteLine("  stepper motor <steps> <direction> <speed_rpm>");
    USBSerial_WriteLine("  direction = forward, reverse, f, r");
    USBSerial_WriteLine("");
    USBSerial_WriteLine("NeoPixel commands:");
    USBSerial_WriteLine("  neopixel fill <r> <g> <b>");
    USBSerial_WriteLine("");
    USBSerial_WriteLine("OLED commands:");
    USBSerial_WriteLine("  oled print <message>");
}


static void PrintIRReadings(void)
{
    uint8_t mask = IRSensors_GetActiveMask();

    USBSerial_Printf("IR active mask: 0x%02X\r\n", mask);

    for (uint8_t i = 0u; i < IR_SENSOR_COUNT; i++)
    {
        USBSerial_Printf(
            "IR %u: %s\r\n",
            i,
            (mask & (1u << i)) ? "ACTIVE" : "inactive"
        );
    }
}


static uint8_t ParseDirection(const char *direction_string, tmc2209_dir_t *direction)
{
    if ((direction_string == NULL) || (direction == NULL))
    {
        return 0u;
    }

    if ((strcmp(direction_string, "forward") == 0) ||
        (strcmp(direction_string, "f") == 0) ||
        (strcmp(direction_string, "cw") == 0))
    {
        *direction = TMC2209_DIR_FORWARD;
        return 1u;
    }

    if ((strcmp(direction_string, "reverse") == 0) ||
        (strcmp(direction_string, "r") == 0) ||
        (strcmp(direction_string, "ccw") == 0))
    {
        *direction = TMC2209_DIR_REVERSE;
        return 1u;
    }

    return 0u;
}


static void HandleStepperCommand(const char *cmd)
{
    unsigned long steps;
    unsigned int speed_rpm;
    char direction_string[16];
    tmc2209_dir_t direction;

    if (sscanf(cmd,
               "stepper motor %lu %15s %u",
               &steps,
               direction_string,
               &speed_rpm) != 3)
    {
        USBSerial_WriteLine("Usage: stepper motor <steps> <direction> <speed_rpm>");
        USBSerial_WriteLine("Example: stepper motor 3200 forward 60");
        return;
    }

    if (steps == 0ul)
    {
        USBSerial_WriteLine("Stepper steps must be greater than 0");
        return;
    }

    if (speed_rpm == 0u)
    {
        USBSerial_WriteLine("Stepper speed must be greater than 0 RPM");
        return;
    }

    if (!ParseDirection(direction_string, &direction))
    {
        USBSerial_WriteLine("Invalid direction. Use forward, reverse, f, r, cw, or ccw.");
        return;
    }

    if (TMC2209_IsBusy())
    {
        USBSerial_WriteLine("Stepper is already moving");
        return;
    }

    TMC2209_MoveStepsNonBlocking(
        direction,
        (uint32_t)steps,
        (uint16_t)speed_rpm
    );

    USBSerial_Printf(
        "Stepper moving %lu steps %s at %u RPM\r\n",
        steps,
        direction_string,
        speed_rpm
    );
}


static void HandleNeoPixelCommand(const char *cmd)
{
    int r;
    int g;
    int b;

    if (sscanf(cmd, "neopixel fill %d %d %d", &r, &g, &b) != 3)
    {
        USBSerial_WriteLine("Usage: neopixel fill <r> <g> <b>");
        USBSerial_WriteLine("Example: neopixel fill 255 0 80");
        return;
    }

    if ((r < 0) || (r > 255) ||
        (g < 0) || (g > 255) ||
        (b < 0) || (b > 255))
    {
        USBSerial_WriteLine("RGB values must be between 0 and 255");
        return;
    }

    NeoPixel_SetAll((uint8_t)r, (uint8_t)g, (uint8_t)b);
    NeoPixel_Show();

    USBSerial_Printf("NeoPixel fill: R=%d G=%d B=%d\r\n", r, g, b);
}


static void HandleOLEDCommand(const char *cmd)
{
    const char *prefix = "oled print ";
    const char *message;

    if (strncmp(cmd, prefix, strlen(prefix)) != 0)
    {
        USBSerial_WriteLine("Usage: oled print <message>");
        return;
    }

    message = cmd + strlen(prefix);

    if (message[0] == '\0')
    {
        USBSerial_WriteLine("OLED message cannot be empty");
        return;
    }

    SSD1309_Clear();
    SSD1309_SetCursor(0, 0);
    SSD1309_WriteString(message, SSD1309_COLOR_WHITE);
    SSD1309_Update();

    USBSerial_Printf("OLED printed: %s\r\n", message);
}


static void HandleCommand(const char *cmd)
{
    int pump_num;
    unsigned long dispense_ms;
    unsigned long reverse_ms;
    char action[16];

    if (cmd == NULL)
    {
        return;
    }

    if (strcmp(cmd, "help") == 0)
    {
        PrintHelp();
        return;
    }

    if (strcmp(cmd, "status") == 0)
    {
        USBSerial_WriteLine("System running");

        USBSerial_Printf("Stepper busy: %u\r\n", TMC2209_IsBusy());
        USBSerial_Printf("Stepper steps done: %lu\r\n", TMC2209_GetStepsCompleted());
        USBSerial_Printf("Stepper steps left: %lu\r\n", TMC2209_GetStepsRemaining());

        USBSerial_Printf("IR mask: 0x%02X\r\n", IRSensors_GetActiveMask());

        return;
    }

    if (strcmp(cmd, "ir readings") == 0)
    {
        PrintIRReadings();
        return;
    }

    if (strncmp(cmd, "stepper motor ", 14u) == 0)
    {
        HandleStepperCommand(cmd);
        return;
    }

    if (strncmp(cmd, "neopixel fill ", 14u) == 0)
    {
        HandleNeoPixelCommand(cmd);
        return;
    }

    if (strncmp(cmd, "oled print ", 11u) == 0)
    {
        HandleOLEDCommand(cmd);
        return;
    }

    if (sscanf(cmd, "pump %d %15s", &pump_num, action) == 2)
    {
        if ((pump_num < 0) || (pump_num >= PUMP_COUNT))
        {
            USBSerial_WriteLine("Invalid pump number");
            return;
        }

        if (strcmp(action, "forward") == 0)
        {
            Pump_Run((pump_id_t)pump_num, PUMP_DIR_FORWARD);
            USBSerial_Printf("Pump %d forward\r\n", pump_num);
            return;
        }

        if (strcmp(action, "reverse") == 0)
        {
            Pump_Run((pump_id_t)pump_num, PUMP_DIR_REVERSE);
            USBSerial_Printf("Pump %d reverse\r\n", pump_num);
            return;
        }

        if (strcmp(action, "stop") == 0)
        {
            Pump_Stop((pump_id_t)pump_num);
            USBSerial_Printf("Pump %d stopped\r\n", pump_num);
            return;
        }

        if (strcmp(action, "dispense") == 0)
        {
            if (sscanf(cmd,
                       "pump %d dispense %lu %lu",
                       &pump_num,
                       &dispense_ms,
                       &reverse_ms) == 3)
            {
                Pump_DispenseNonBlocking(
                    (pump_id_t)pump_num,
                    (uint32_t)dispense_ms,
                    (uint32_t)reverse_ms
                );

                USBSerial_Printf(
                    "Pump %d dispense %lu ms, reverse %lu ms\r\n",
                    pump_num,
                    dispense_ms,
                    reverse_ms
                );
                return;
            }
            else
            {
                USBSerial_WriteLine("Usage: pump <n> dispense <ms> <reverse_ms>");
                return;
            }
        }
    }

    USBSerial_Printf("Unknown command: %s\r\n", cmd);
}


int main(void)
{
    CyGlobalIntEnable;

    USBSerial_Init(1000u);

    Pumps_Init();
    Keypad_Init();
    IRSensors_Init(IR_SENSOR_ACTIVE_HIGH); // Change to IR_SENSOR_ACTIVE_LOW if needed.

    SSD1309_Start();
    SSD1309_Clear();
    SSD1309_SetCursor(0, 0);
    SSD1309_WriteString("Barduino Ready", SSD1309_COLOR_WHITE);
    SSD1309_Update();

    NeoPixel_Init();
    NeoPixel_SetBrightness(64u);
    NeoPixel_ClearAndShow();

    /*
     * Match timer_tick_hz to your Stepper_Timer interrupt rate.
     * Example:
     *     1 MHz PWM clock
     *     Period = 49
     *     interrupt rate = 20 kHz
     */
    tmc2209_config_t stepper_config = {
        .full_steps_per_rev = 200,
        .microsteps = 16,
        .timer_tick_hz = 20000u,
        .invert_dir = 0
    };

    TMC2209_Init(&stepper_config);

    Stepper_Timer_SetInterruptMode(Stepper_Timer_STATUS_TC_INT_EN_MASK);
    isr_stepper_StartEx(TMC2209_TimerISR);
    Stepper_Timer_Start();

    TMC2209_Enable();

    USBSerial_WriteLine("Ready. Type help.");

    for (;;)
    {
        USBSerial_Update();
        Keypad_Update();
        Pumps_Update();
        IRSensors_Update();

        if (USBSerial_SerialAvailable())
        {
            const char *cmd = USBSerial_ReadString();

            if (cmd != NULL)
            {
                USBSerial_Printf("RX command: [%s]\r\n", cmd);
                HandleCommand(cmd);
            }
        }

        CyDelay(5u);
    }
}