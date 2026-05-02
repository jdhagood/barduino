#include "pumps.h"
#include "shift_register.h"

/*
 * 74HC595 output bit mapping:
 *
 * bit 0  -> PUMP0_A
 * bit 1  -> PUMP0_B
 * bit 2  -> PUMP1_A
 * bit 3  -> PUMP1_B
 * bit 4  -> PUMP2_A
 * bit 5  -> PUMP2_B
 * bit 6  -> PUMP3_A
 * bit 7  -> PUMP3_B
 * bit 8  -> PUMP4_A
 * bit 9  -> PUMP4_B
 * bit 10 -> PUMP5_A
 * bit 11 -> PUMP5_B
 */
#define PUMP_A_BIT(pump)    ((uint8_t)((uint8_t)(pump) * 2u))
#define PUMP_B_BIT(pump)    ((uint8_t)(((uint8_t)(pump) * 2u) + 1u))

typedef enum
{
    PUMP_TASK_IDLE = 0,
    PUMP_TASK_DISPENSE_FORWARD,
    PUMP_TASK_REVERSE
} pump_task_state_t;

typedef struct
{
    pump_state_t state;
    pump_task_state_t task_state;
    uint32_t state_start_ms;
    uint32_t dispense_time_ms;
    uint32_t reverse_time_ms;
} pump_control_t;

static pump_control_t pumps[PUMP_COUNT];

/*
 * This module owns a simple millisecond timebase using SysTick.
 * If another module already owns SysTick, replace Pumps_Millis()
 * with your shared SystemTime_Millis().
 */
static volatile uint32_t pump_millis = 0u;

static void Pumps_SysTickISR(void)
{
    pump_millis++;
}

static uint32_t Pumps_Millis(void)
{
    uint32_t now;

    CyGlobalIntDisable;
    now = pump_millis;
    CyGlobalIntEnable;

    return now;
}

static uint8_t Pump_IsValid(pump_id_t pump)
{
    return ((uint8_t)pump < PUMP_COUNT);
}

/*
 * Low-level L9110S command.
 *
 * A=0, B=0 -> off/coast
 * A=1, B=0 -> forward
 * A=0, B=1 -> reverse
 * A=1, B=1 -> brake, intentionally unused
 */
static void Pump_WritePins(pump_id_t pump, uint8_t a, uint8_t b)
{
    uint8_t a_bit;
    uint8_t b_bit;

    if (!Pump_IsValid(pump))
    {
        return;
    }

    a_bit = PUMP_A_BIT(pump);
    b_bit = PUMP_B_BIT(pump);

    /*
     * Set both bits in the software image first,
     * then update once so the physical outputs change together.
     */
    ShiftRegister_SetBit(a_bit, a);
    ShiftRegister_SetBit(b_bit, b);
    ShiftRegister_Update();
}

static void Pump_SetState(pump_id_t pump, pump_state_t state)
{
    if (!Pump_IsValid(pump))
    {
        return;
    }

    switch (state)
    {
        case PUMP_STATE_OFF:
            Pump_WritePins(pump, 0u, 0u);
            break;

        case PUMP_STATE_FORWARD:
            Pump_WritePins(pump, 1u, 0u);
            break;

        case PUMP_STATE_REVERSE:
            Pump_WritePins(pump, 0u, 1u);
            break;

        default:
            Pump_WritePins(pump, 0u, 0u);
            state = PUMP_STATE_OFF;
            break;
    }

    pumps[pump].state = state;
}

void Pumps_Init(void)
{
    ShiftRegister_Init();

    /*
     * 1 ms SysTick timebase for nonblocking pump timing.
     *
     * If you already use SysTick elsewhere, remove these two lines and
     * provide a shared millis function instead.
     */
    CySysTickStart();
    CySysTickSetCallback(0u, Pumps_SysTickISR);

    for (uint8_t i = 0; i < PUMP_COUNT; i++)
    {
        pumps[i].state = PUMP_STATE_OFF;
        pumps[i].task_state = PUMP_TASK_IDLE;
        pumps[i].state_start_ms = 0u;
        pumps[i].dispense_time_ms = 0u;
        pumps[i].reverse_time_ms = 0u;
    }

    Pumps_StopAll();
}

void Pumps_StopAll(void)
{
    for (uint8_t i = 0; i < PUMP_COUNT; i++)
    {
        pumps[i].state = PUMP_STATE_OFF;
        pumps[i].task_state = PUMP_TASK_IDLE;
        pumps[i].state_start_ms = 0u;
        pumps[i].dispense_time_ms = 0u;
        pumps[i].reverse_time_ms = 0u;
    }

    /*
     * Clear all 16 shift-register outputs.
     * This turns all pump H-bridge inputs off.
     */
    ShiftRegister_Clear();
}

void Pump_Stop(pump_id_t pump)
{
    if (!Pump_IsValid(pump))
    {
        return;
    }

    pumps[pump].task_state = PUMP_TASK_IDLE;
    pumps[pump].dispense_time_ms = 0u;
    pumps[pump].reverse_time_ms = 0u;

    Pump_SetState(pump, PUMP_STATE_OFF);
}

void Pump_Run(pump_id_t pump, pump_dir_t direction)
{
    if (!Pump_IsValid(pump))
    {
        return;
    }

    /*
     * Manual continuous run cancels any active timed dispense.
     */
    pumps[pump].task_state = PUMP_TASK_IDLE;
    pumps[pump].dispense_time_ms = 0u;
    pumps[pump].reverse_time_ms = 0u;

    if (direction == PUMP_DIR_FORWARD)
    {
        Pump_SetState(pump, PUMP_STATE_FORWARD);
    }
    else
    {
        Pump_SetState(pump, PUMP_STATE_REVERSE);
    }
}

void Pump_DispenseBlocking(pump_id_t pump,
                           uint32_t dispense_time_ms,
                           uint32_t reverse_time_ms)
{
    if (!Pump_IsValid(pump))
    {
        return;
    }

    if (dispense_time_ms > 0u)
    {
        Pump_SetState(pump, PUMP_STATE_FORWARD);
        CyDelay(dispense_time_ms);
    }

    if (reverse_time_ms > 0u)
    {
        Pump_SetState(pump, PUMP_STATE_REVERSE);
        CyDelay(reverse_time_ms);
    }

    Pump_SetState(pump, PUMP_STATE_OFF);
}

void Pump_DispenseNonBlocking(pump_id_t pump,
                              uint32_t dispense_time_ms,
                              uint32_t reverse_time_ms)
{
    uint32_t now;

    if (!Pump_IsValid(pump))
    {
        return;
    }

    if ((dispense_time_ms == 0u) && (reverse_time_ms == 0u))
    {
        Pump_Stop(pump);
        return;
    }

    now = Pumps_Millis();

    pumps[pump].dispense_time_ms = dispense_time_ms;
    pumps[pump].reverse_time_ms = reverse_time_ms;
    pumps[pump].state_start_ms = now;

    if (dispense_time_ms > 0u)
    {
        pumps[pump].task_state = PUMP_TASK_DISPENSE_FORWARD;
        Pump_SetState(pump, PUMP_STATE_FORWARD);
    }
    else
    {
        pumps[pump].task_state = PUMP_TASK_REVERSE;
        Pump_SetState(pump, PUMP_STATE_REVERSE);
    }
}

void Pumps_Update(void)
{
    uint32_t now = Pumps_Millis();

    for (uint8_t i = 0; i < PUMP_COUNT; i++)
    {
        pump_id_t pump = (pump_id_t)i;

        switch (pumps[i].task_state)
        {
            case PUMP_TASK_IDLE:
                break;

            case PUMP_TASK_DISPENSE_FORWARD:
                if ((uint32_t)(now - pumps[i].state_start_ms) >= pumps[i].dispense_time_ms)
                {
                    if (pumps[i].reverse_time_ms > 0u)
                    {
                        pumps[i].task_state = PUMP_TASK_REVERSE;
                        pumps[i].state_start_ms = now;
                        Pump_SetState(pump, PUMP_STATE_REVERSE);
                    }
                    else
                    {
                        Pump_Stop(pump);
                    }
                }
                break;

            case PUMP_TASK_REVERSE:
                if ((uint32_t)(now - pumps[i].state_start_ms) >= pumps[i].reverse_time_ms)
                {
                    Pump_Stop(pump);
                }
                break;

            default:
                Pump_Stop(pump);
                break;
        }
    }
}

uint8_t Pump_IsBusy(pump_id_t pump)
{
    if (!Pump_IsValid(pump))
    {
        return 0u;
    }

    if (pumps[pump].task_state != PUMP_TASK_IDLE)
    {
        return 1u;
    }

    if (pumps[pump].state != PUMP_STATE_OFF)
    {
        return 1u;
    }

    return 0u;
}

pump_state_t Pump_GetState(pump_id_t pump)
{
    if (!Pump_IsValid(pump))
    {
        return PUMP_STATE_OFF;
    }

    return pumps[pump].state;
}