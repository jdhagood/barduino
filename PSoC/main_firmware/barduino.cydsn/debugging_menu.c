#include "debugging_menu.h"

#include "usb_serial.h"
#include "pumps.h"
#include "ir_sensors.h"
#include "hall_sensor.h"
#include "keypad.h"
#include "neopixel.h"
#include "tmc2209_stepper.h"
#include "ssd1309.h"
#include "ssd1309_gfx.h"
#include "drink_db.h"
#include "neopixel_anim.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define STEPPER_HOME_RPM          50u
#define STEPPER_HOME_MICROSTEPS   16u

static uint8_t stepper_homing_active = 0u;

static void DebuggingMenu_PrintHelp(void)
{
    USBSerial_WriteLine("Debug commands:");
    USBSerial_WriteLine("  help");
    USBSerial_WriteLine("  status");
    USBSerial_WriteLine("");

    USBSerial_WriteLine("Pump commands:");
    USBSerial_WriteLine("  pump <n> forward");
    USBSerial_WriteLine("  pump <n> reverse");
    USBSerial_WriteLine("  pump <n> stop");
    USBSerial_WriteLine("  pump <n> dispense <ms> <reverse_ms>");
    USBSerial_WriteLine("  pumps stop");
    USBSerial_WriteLine("");

    USBSerial_WriteLine("IR commands:");
    USBSerial_WriteLine("  ir readings");
    USBSerial_WriteLine("  ir read sensors");
    USBSerial_WriteLine("");

    USBSerial_WriteLine("Keypad commands:");
    USBSerial_WriteLine("  keypad readings");
    USBSerial_WriteLine("");

    USBSerial_WriteLine("Stepper commands:");
    USBSerial_WriteLine("  stepper enable");
    USBSerial_WriteLine("  stepper disable");
    USBSerial_WriteLine("  stepper stop");
    USBSerial_WriteLine("  stepper home");
    USBSerial_WriteLine("  stepper microsteps <2|4|8|16>");
    USBSerial_WriteLine("  stepper advance <steps> <direction> <max_rpm>");
    USBSerial_WriteLine("  stepper advance <steps> <direction> <max_rpm> <accel_sps2>");
    USBSerial_WriteLine("  stepper motor <steps> <direction> <speed_rpm>");
    USBSerial_WriteLine("  direction = forward, reverse, f, r, cw, ccw");
    USBSerial_WriteLine("");

    USBSerial_WriteLine("NeoPixel commands:");
    USBSerial_WriteLine("  neopixel fill <r> <g> <b>");
    USBSerial_WriteLine("  neopixel clear");
    USBSerial_WriteLine("  neopixel rainbow <interval_ms>");
    USBSerial_WriteLine("  neopixel chase <r> <g> <b> <interval_ms>");
    USBSerial_WriteLine("  neopixel breathe <r> <g> <b> <interval_ms>");
    USBSerial_WriteLine("  neopixel scanner <r> <g> <b> <interval_ms>");
    USBSerial_WriteLine("  neopixel sparkle <r> <g> <b> <interval_ms>");
    USBSerial_WriteLine("  neopixel off");
    USBSerial_WriteLine("");

    USBSerial_WriteLine("OLED commands:");
    USBSerial_WriteLine("  oled print <message>");
    USBSerial_WriteLine("  oled clear");
    USBSerial_WriteLine("");
    
    USBSerial_WriteLine("Hall sensor commands:");
    USBSerial_WriteLine("  hall sensor read");
    USBSerial_WriteLine("");

    USBSerial_WriteLine("Drink DB commands:");
    USBSerial_WriteLine("  drinks list");
    USBSerial_WriteLine("  drinks capacity");
    USBSerial_WriteLine("  drinks format");
}


static uint8_t DebuggingMenu_ParseDirection(const char *direction_string,
                                            tmc2209_dir_t *direction)
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


static void DebuggingMenu_PrintStatus(void)
{
    USBSerial_WriteLine("System status:");

    USBSerial_Printf("  IR mask: 0x%02X\r\n", IRSensors_GetActiveMask());
    USBSerial_Printf("  Keypad mask: 0x%02X\r\n", Keypad_GetPressedMask());

    USBSerial_Printf("  Stepper enabled: %u\r\n", TMC2209_GetEnabled());
    USBSerial_Printf("  Stepper busy: %u\r\n", TMC2209_IsBusy());
    USBSerial_Printf("  Stepper microsteps: %u\r\n", TMC2209_GetMicrosteps());
    USBSerial_Printf("  Stepper steps done: %lu\r\n", TMC2209_GetStepsCompleted());
    USBSerial_Printf("  Stepper steps left: %lu\r\n", TMC2209_GetStepsRemaining());
    USBSerial_Printf("  Stepper speed SPS: %lu\r\n", TMC2209_GetCurrentSpeedSPS());

    for (uint8_t i = 0u; i < PUMP_COUNT; i++)
    {
        USBSerial_Printf("  Pump %u busy: %u state: %u\r\n",
                         i,
                         Pump_IsBusy((pump_id_t)i),
                         Pump_GetState((pump_id_t)i));
    }

    USBSerial_Printf("  Drinks saved: %u\r\n", DrinkDB_GetCount());
    USBSerial_Printf("  Drink capacity: %u\r\n", DrinkDB_GetMaxCapacity());
}


static void DebuggingMenu_PrintIRReadings(void)
{
    uint8_t mask = IRSensors_GetActiveMask();

    USBSerial_Printf("IR active mask: 0x%02X\r\n", mask);

    for (uint8_t i = 0u; i < IR_SENSOR_COUNT; i++)
    {
        USBSerial_Printf("IR %u: %s\r\n",
                         i,
                         (mask & (1u << i)) ? "ACTIVE" : "inactive");
    }
}


static void DebuggingMenu_PrintKeypadReadings(void)
{
    uint8_t mask = Keypad_GetPressedMask();

    USBSerial_Printf("Keypad pressed mask: 0x%02X\r\n", mask);
    USBSerial_Printf("  UP:    %u\r\n", Keypad_IsPressed(BUTTON_UP));
    USBSerial_Printf("  DOWN:  %u\r\n", Keypad_IsPressed(BUTTON_DOWN));
    USBSerial_Printf("  LEFT:  %u\r\n", Keypad_IsPressed(BUTTON_LEFT));
    USBSerial_Printf("  RIGHT: %u\r\n", Keypad_IsPressed(BUTTON_RIGHT));
    USBSerial_Printf("  ENTER: %u\r\n", Keypad_IsPressed(BUTTON_ENTER));
    USBSerial_Printf("  BACK:  %u\r\n", Keypad_IsPressed(BUTTON_BACK));
}


static void DebuggingMenu_HandlePumpCommand(const char *cmd)
{
    int pump_num;
    unsigned long dispense_ms;
    unsigned long reverse_ms;
    char action[16];

    if (strcmp(cmd, "pumps stop") == 0)
    {
        Pumps_StopAll();
        USBSerial_WriteLine("All pumps stopped");
        return;
    }

    if (sscanf(cmd, "pump %d %15s", &pump_num, action) != 2)
    {
        USBSerial_WriteLine("Usage: pump <n> forward|reverse|stop|dispense <ms> <reverse_ms>");
        return;
    }

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
            Pump_DispenseNonBlocking((pump_id_t)pump_num,
                                     (uint32_t)dispense_ms,
                                     (uint32_t)reverse_ms);

            USBSerial_Printf("Pump %d dispense %lu ms, reverse %lu ms\r\n",
                             pump_num,
                             dispense_ms,
                             reverse_ms);
            return;
        }

        USBSerial_WriteLine("Usage: pump <n> dispense <ms> <reverse_ms>");
        return;
    }

    USBSerial_WriteLine("Unknown pump command");
}


static void DebuggingMenu_HandleStepperCommand(const char *cmd)
{
    unsigned long steps;
    unsigned int max_rpm;
    unsigned long accel_sps2;
    unsigned int microsteps;
    char direction_string[16];
    tmc2209_dir_t direction;

    if (strcmp(cmd, "stepper enable") == 0)
    {
        TMC2209_Enable();
        USBSerial_WriteLine("Stepper enabled");
        return;
    }

    if (strcmp(cmd, "stepper disable") == 0)
    {
        TMC2209_StopMotion();
        TMC2209_Disable();
        USBSerial_WriteLine("Stepper disabled");
        return;
    }

    if (strcmp(cmd, "stepper stop") == 0)
    {
        TMC2209_StopMotion();
        USBSerial_WriteLine("Stepper stopped");
        return;
    }

    if (sscanf(cmd, "stepper microsteps %u", &microsteps) == 1)
    {
        if (TMC2209_IsBusy())
        {
            USBSerial_WriteLine("Cannot change microsteps while stepper is moving");
            return;
        }

        if (TMC2209_SetMicrosteps((uint16_t)microsteps))
        {
            USBSerial_Printf("Stepper microsteps set to %u\r\n", microsteps);
        }
        else
        {
            USBSerial_WriteLine("Invalid microsteps. Use 2, 4, 8, or 16.");
        }

        return;
    }

    /*
     * Preferred command:
     *   stepper advance <steps> <direction> <max_rpm> <accel_sps2>
     *
     * Example:
     *   stepper advance 3200 forward 60 3000
     */
    if (sscanf(cmd,
               "stepper advance %lu %15s %u %lu",
               &steps,
               direction_string,
               &max_rpm,
               &accel_sps2) == 4)
    {
        if (steps == 0ul)
        {
            USBSerial_WriteLine("Steps must be greater than 0");
            return;
        }

        if (max_rpm == 0u)
        {
            USBSerial_WriteLine("Max speed must be greater than 0 RPM");
            return;
        }

        if (!DebuggingMenu_ParseDirection(direction_string, &direction))
        {
            USBSerial_WriteLine("Invalid direction. Use forward, reverse, f, r, cw, or ccw.");
            return;
        }

        if (TMC2209_IsBusy())
        {
            USBSerial_WriteLine("Stepper is already moving");
            return;
        }

        TMC2209_Enable();

        if (accel_sps2 == 0ul)
        {
            TMC2209_MoveStepsNonBlocking(direction,
                                         (uint32_t)steps,
                                         (uint16_t)max_rpm);
        }
        else
        {
            TMC2209_MoveStepsAccelNonBlocking(direction,
                                              (uint32_t)steps,
                                              (uint16_t)max_rpm,
                                              (uint32_t)accel_sps2);
        }

        USBSerial_Printf("Stepper advance: %lu steps %s, max %u RPM, accel %lu sps^2\r\n",
                         steps,
                         direction_string,
                         max_rpm,
                         accel_sps2);
        return;
    }

    /*
     * Version without acceleration:
     *   stepper advance <steps> <direction> <max_rpm>
     */
    if (sscanf(cmd,
               "stepper advance %lu %15s %u",
               &steps,
               direction_string,
               &max_rpm) == 3)
    {
        if (steps == 0ul)
        {
            USBSerial_WriteLine("Steps must be greater than 0");
            return;
        }

        if (max_rpm == 0u)
        {
            USBSerial_WriteLine("Max speed must be greater than 0 RPM");
            return;
        }

        if (!DebuggingMenu_ParseDirection(direction_string, &direction))
        {
            USBSerial_WriteLine("Invalid direction. Use forward, reverse, f, r, cw, or ccw.");
            return;
        }

        if (TMC2209_IsBusy())
        {
            USBSerial_WriteLine("Stepper is already moving");
            return;
        }

        TMC2209_Enable();

        TMC2209_MoveStepsNonBlocking(direction,
                                     (uint32_t)steps,
                                     (uint16_t)max_rpm);

        USBSerial_Printf("Stepper advance: %lu steps %s at %u RPM, no accel\r\n",
                         steps,
                         direction_string,
                         max_rpm);
        return;
    }

    /*
     * Backward-compatible alias:
     *   stepper motor <steps> <direction> <speed_rpm>
     */
    if (sscanf(cmd,
               "stepper motor %lu %15s %u",
               &steps,
               direction_string,
               &max_rpm) == 3)
    {
        if (steps == 0ul)
        {
            USBSerial_WriteLine("Steps must be greater than 0");
            return;
        }

        if (max_rpm == 0u)
        {
            USBSerial_WriteLine("Speed must be greater than 0 RPM");
            return;
        }

        if (!DebuggingMenu_ParseDirection(direction_string, &direction))
        {
            USBSerial_WriteLine("Invalid direction. Use forward, reverse, f, r, cw, or ccw.");
            return;
        }

        if (TMC2209_IsBusy())
        {
            USBSerial_WriteLine("Stepper is already moving");
            return;
        }

        TMC2209_Enable();

        TMC2209_MoveStepsNonBlocking(direction,
                                     (uint32_t)steps,
                                     (uint16_t)max_rpm);

        USBSerial_Printf("Stepper moving %lu steps %s at %u RPM\r\n",
                         steps,
                         direction_string,
                         max_rpm);
        return;
    }

    USBSerial_WriteLine("Unknown stepper command");
    USBSerial_WriteLine("Usage:");
    USBSerial_WriteLine("  stepper enable");
    USBSerial_WriteLine("  stepper disable");
    USBSerial_WriteLine("  stepper stop");
    USBSerial_WriteLine("  stepper microsteps <2|4|8|16>");
    USBSerial_WriteLine("  stepper advance <steps> <direction> <max_rpm>");
    USBSerial_WriteLine("  stepper advance <steps> <direction> <max_rpm> <accel_sps2>");
}


static void DebuggingMenu_HandleNeoPixelCommand(const char *cmd)
{
    int r;
    int g;
    int b;
    unsigned int interval_ms;

    if (strcmp(cmd, "neopixel clear") == 0 || strcmp(cmd, "neopixel off") == 0)
    {
        NeoPixelAnim_SetOff();
        USBSerial_WriteLine("NeoPixels off");
        return;
    }

    if (sscanf(cmd, "neopixel rainbow %u", &interval_ms) == 1)
    {
        NeoPixelAnim_SetRainbow((uint16_t)interval_ms);
        USBSerial_Printf("NeoPixel rainbow interval %u ms\r\n", interval_ms);
        return;
    }

    if (sscanf(cmd, "neopixel fill %d %d %d", &r, &g, &b) == 3)
    {
        if ((r < 0) || (r > 255) ||
            (g < 0) || (g > 255) ||
            (b < 0) || (b > 255))
        {
            USBSerial_WriteLine("RGB values must be between 0 and 255");
            return;
        }

        NeoPixelAnim_SetSolid((uint8_t)r, (uint8_t)g, (uint8_t)b);
        USBSerial_Printf("NeoPixel fill: R=%d G=%d B=%d\r\n", r, g, b);
        return;
    }

    if (sscanf(cmd, "neopixel chase %d %d %d %u", &r, &g, &b, &interval_ms) == 4)
    {
        if ((r < 0) || (r > 255) ||
            (g < 0) || (g > 255) ||
            (b < 0) || (b > 255))
        {
            USBSerial_WriteLine("RGB values must be between 0 and 255");
            return;
        }

        NeoPixelAnim_SetTheaterChase((uint8_t)r, (uint8_t)g, (uint8_t)b, (uint16_t)interval_ms);
        USBSerial_WriteLine("NeoPixel theater chase started");
        return;
    }

    if (sscanf(cmd, "neopixel breathe %d %d %d %u", &r, &g, &b, &interval_ms) == 4)
    {
        if ((r < 0) || (r > 255) ||
            (g < 0) || (g > 255) ||
            (b < 0) || (b > 255))
        {
            USBSerial_WriteLine("RGB values must be between 0 and 255");
            return;
        }

        NeoPixelAnim_SetBreathing((uint8_t)r, (uint8_t)g, (uint8_t)b, (uint16_t)interval_ms);
        USBSerial_WriteLine("NeoPixel breathing started");
        return;
    }

    if (sscanf(cmd, "neopixel scanner %d %d %d %u", &r, &g, &b, &interval_ms) == 4)
    {
        if ((r < 0) || (r > 255) ||
            (g < 0) || (g > 255) ||
            (b < 0) || (b > 255))
        {
            USBSerial_WriteLine("RGB values must be between 0 and 255");
            return;
        }

        NeoPixelAnim_SetScanner((uint8_t)r, (uint8_t)g, (uint8_t)b, (uint16_t)interval_ms);
        USBSerial_WriteLine("NeoPixel scanner started");
        return;
    }

    if (sscanf(cmd, "neopixel sparkle %d %d %d %u", &r, &g, &b, &interval_ms) == 4)
    {
        if ((r < 0) || (r > 255) ||
            (g < 0) || (g > 255) ||
            (b < 0) || (b > 255))
        {
            USBSerial_WriteLine("RGB values must be between 0 and 255");
            return;
        }

        NeoPixelAnim_SetSparkle((uint8_t)r, (uint8_t)g, (uint8_t)b, (uint16_t)interval_ms);
        USBSerial_WriteLine("NeoPixel sparkle started");
        return;
    }

    USBSerial_WriteLine("Unknown NeoPixel command");
    USBSerial_WriteLine("Examples:");
    USBSerial_WriteLine("  neopixel rainbow 30");
    USBSerial_WriteLine("  neopixel chase 255 0 0 100");
    USBSerial_WriteLine("  neopixel breathe 0 0 255 20");
    USBSerial_WriteLine("  neopixel scanner 255 80 0 30");
    USBSerial_WriteLine("  neopixel sparkle 255 255 255 80");
}


static void DebuggingMenu_HandleOLEDCommand(const char *cmd)
{
    const char *prefix = "oled print ";
    const char *message;

    if (strcmp(cmd, "oled clear") == 0)
    {
        SSD1309_Clear();
        SSD1309_Update();
        USBSerial_WriteLine("OLED cleared");
        return;
    }

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

static void DebuggingMenu_PrintHallSensorReading(void)
{
    USBSerial_Printf("Hall sensor raw: %u\r\n", HallSensor_ReadRaw());
    USBSerial_Printf("Hall sensor active: %u\r\n", HallSensor_IsActive());
}


static void DebuggingMenu_HandleDrinksCommand(const char *cmd)
{
    if (strcmp(cmd, "drinks capacity") == 0)
    {
        USBSerial_Printf("Drink count: %u\r\n", DrinkDB_GetCount());
        USBSerial_Printf("Drink capacity: %u\r\n", DrinkDB_GetMaxCapacity());
        return;
    }

    if (strcmp(cmd, "drinks list") == 0)
    {
        uint8_t count = DrinkDB_GetCount();

        USBSerial_Printf("Drinks: %u/%u\r\n", count, DrinkDB_GetMaxCapacity());

        for (uint8_t i = 0u; i < count; i++)
        {
            const drink_t *drink = DrinkDB_GetDrink(i);

            if (drink != 0)
            {
                USBSerial_Printf("%u: %s  [%u %u %u %u %u %u]\r\n",
                                 i,
                                 drink->name,
                                 drink->amount[0],
                                 drink->amount[1],
                                 drink->amount[2],
                                 drink->amount[3],
                                 drink->amount[4],
                                 drink->amount[5]);
            }
        }

        return;
    }

    if (strcmp(cmd, "drinks format") == 0)
    {
        if (DrinkDB_FormatEEPROM())
        {
            USBSerial_WriteLine("Drink EEPROM formatted");
        }
        else
        {
            USBSerial_WriteLine("Drink EEPROM format failed");
        }

        return;
    }

    USBSerial_WriteLine("Unknown drinks command");
}


void DebuggingMenu_Init(void)
{
    USBSerial_WriteLine("Debug menu ready. Type help.");
}


void DebuggingMenu_Update(void)
{
    if (USBSerial_SerialAvailable())
    {
        const char *cmd = USBSerial_ReadString();

        if (cmd != NULL)
        {
            USBSerial_Printf("RX command: [%s]\r\n", cmd);
            DebuggingMenu_HandleCommand(cmd);
        }
    }
}


static void DebuggingMenu_UpdateStepperHoming(void)
{
    if (!stepper_homing_active)
    {
        return;
    }

    if (HallSensor_IsActive())
    {
        TMC2209_StopMotion();
        stepper_homing_active = 0u;

        USBSerial_WriteLine("Stepper homing complete: hall sensor detected");
    }
}

void DebuggingMenu_HandleCommand(const char *cmd)
{
    if (cmd == NULL)
    {
        return;
    }

    if (strcmp(cmd, "help") == 0)
    {
        DebuggingMenu_PrintHelp();
        return;
    }

    if (strcmp(cmd, "status") == 0)
    {
        DebuggingMenu_PrintStatus();
        return;
    }

    if ((strcmp(cmd, "ir readings") == 0) ||
        (strcmp(cmd, "ir read sensors") == 0))
    {
        DebuggingMenu_PrintIRReadings();
        return;
    }

    if (strcmp(cmd, "keypad readings") == 0)
    {
        DebuggingMenu_PrintKeypadReadings();
        return;
    }

    if ((strncmp(cmd, "pump ", 5u) == 0) ||
        (strcmp(cmd, "pumps stop") == 0))
    {
        DebuggingMenu_HandlePumpCommand(cmd);
        return;
    }

    if (strncmp(cmd, "stepper ", 8u) == 0)
    {
        DebuggingMenu_HandleStepperCommand(cmd);
        return;
    }

    if (strncmp(cmd, "neopixel ", 9u) == 0)
    {
        DebuggingMenu_HandleNeoPixelCommand(cmd);
        return;
    }

    if (strncmp(cmd, "oled ", 5u) == 0)
    {
        DebuggingMenu_HandleOLEDCommand(cmd);
        return;
    }
    
    if (strcmp(cmd, "hall sensor read") == 0)
    {
        DebuggingMenu_PrintHallSensorReading();
        return;
    }
    if (strncmp(cmd, "drinks ", 7u) == 0)
    {
        DebuggingMenu_HandleDrinksCommand(cmd);
        return;
    }

    USBSerial_Printf("Unknown command: %s\r\n", cmd);
}