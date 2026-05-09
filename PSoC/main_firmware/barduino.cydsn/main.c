#include <project.h>

#include "ui.h"
#include "pumps.h"
#include "keypad.h"
#include "ir_sensors.h"
#include "usb_serial.h"
#include "neopixel.h"
#include "neopixel_anim.h"
#include "tmc2209_stepper.h"
#include "drink_db.h"
#include "debugging_menu.h"

#define STEPPER_FULL_STEPS_PER_REV  200u
#define STEPPER_MICROSTEPS          16u
#define STEPPER_TIMER_TICK_HZ       20000u

static void Stepper_Init(void)
{
    tmc2209_config_t stepper_config = {
        .full_steps_per_rev = STEPPER_FULL_STEPS_PER_REV,
        .microsteps = STEPPER_MICROSTEPS,
        .timer_tick_hz = STEPPER_TIMER_TICK_HZ,
        .invert_dir = 0u
    };

    TMC2209_Init(&stepper_config);

    /*
     * Stepper_Timer should be configured as:
     *   Clock = 1 MHz
     *   Period = 49
     *   Terminal count interrupt enabled
     *   ISR rate = 20 kHz
     */
    Stepper_Timer_SetInterruptMode(Stepper_Timer_STATUS_TC_INT_EN_MASK);
    isr_stepper_StartEx(TMC2209_TimerISR);
    Stepper_Timer_Start();

    /*
     * Do not enable the stepper driver at startup.
     */
    TMC2209_Disable();
}

int main(void)
{
    CyGlobalIntEnable;

    USBSerial_Init(1000u);

    Pumps_Init();
    Keypad_Init();
    IRSensors_Init(IR_SENSOR_ACTIVE_HIGH);

    NeoPixel_Init();
    NeoPixelAnim_Init();   /* REQUIRED for animations */

    DrinkDB_Init();

    Stepper_Init();

    UI_Init();
    DebuggingMenu_Init();

    USBSerial_WriteLine("Ready.");

    for (;;)
    {
        USBSerial_Update();

        Pumps_Update();
        IRSensors_Update();

        UI_Update();
        UI_Draw();

        DebuggingMenu_Update();

        /*
         * Nonblocking NeoPixel animation engine.
         */
        NeoPixelAnim_Update();

        CyDelay(5u);
    }
}