#include "hall_sensor.h"

/*
 * Debounce/filter samples.
 *
 * If HallSensor_Update() is called every 5 ms:
 *     2 samples = ~10 ms debounce/filtering
 *
 * Hall sensors are not mechanically bouncy like buttons, but this helps reject
 * short glitches/noise.
 */
#define HALL_SENSOR_DEBOUNCE_SAMPLES  2u

static hall_sensor_polarity_t hall_polarity = HALL_SENSOR_ACTIVE_HIGH;

static uint8_t stable_active = 0u;
static uint8_t previous_stable_active = 0u;

static uint8_t candidate_active = 0u;
static uint8_t debounce_count = 0u;

static uint8_t activated_event = 0u;
static uint8_t deactivated_event = 0u;

static uint8_t HallSensor_ReadHardwareActive(void)
{
    uint8_t raw = HALL_EFFECT_SENSOR_INPUT_Read() ? 1u : 0u;

    if (hall_polarity == HALL_SENSOR_ACTIVE_LOW)
    {
        raw = raw ? 0u : 1u;
    }

    return raw;
}

void HallSensor_Init(hall_sensor_polarity_t polarity)
{
    hall_polarity = polarity;

    stable_active = HallSensor_ReadHardwareActive();
    previous_stable_active = stable_active;

    candidate_active = stable_active;
    debounce_count = 0u;

    activated_event = 0u;
    deactivated_event = 0u;
}

void HallSensor_Update(void)
{
    uint8_t raw_active = HallSensor_ReadHardwareActive();

    if (raw_active == candidate_active)
    {
        if (debounce_count < HALL_SENSOR_DEBOUNCE_SAMPLES)
        {
            debounce_count++;
        }
    }
    else
    {
        candidate_active = raw_active;
        debounce_count = 0u;
    }

    if (debounce_count >= HALL_SENSOR_DEBOUNCE_SAMPLES)
    {
        if (stable_active != candidate_active)
        {
            previous_stable_active = stable_active;
            stable_active = candidate_active;

            if ((previous_stable_active == 0u) && (stable_active == 1u))
            {
                activated_event = 1u;
            }
            else if ((previous_stable_active == 1u) && (stable_active == 0u))
            {
                deactivated_event = 1u;
            }
        }
    }
}

uint8_t HallSensor_ReadRaw(void)
{
    return HallSensor_ReadHardwareActive();
}

uint8_t HallSensor_IsActive(void)
{
    return stable_active;
}

uint8_t HallSensor_WasActivated(void)
{
    if (activated_event)
    {
        activated_event = 0u;
        return 1u;
    }

    return 0u;
}

uint8_t HallSensor_WasDeactivated(void)
{
    if (deactivated_event)
    {
        deactivated_event = 0u;
        return 1u;
    }

    return 0u;
}

void HallSensor_ClearEvents(void)
{
    activated_event = 0u;
    deactivated_event = 0u;
}