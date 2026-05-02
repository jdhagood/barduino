#include "ir_sensors.h"

#define IR_SENSOR_ALL_MASK ((1u << IR_SENSOR_COUNT) - 1u)

/*
 * Debounce/filter time in update samples.
 *
 * If IRSensors_Update() is called every 5 ms:
 *     2 samples = ~10 ms filtering
 *
 * IR sensors are usually cleaner than mechanical buttons,
 * so this can be shorter than keypad debounce.
 */
#define IR_SENSOR_DEBOUNCE_SAMPLES 2u

static ir_sensor_polarity_t sensor_polarity = IR_SENSOR_ACTIVE_HIGH;

static uint8_t stable_active_mask = 0u;
static uint8_t previous_stable_active_mask = 0u;

static uint8_t candidate_active_mask = 0u;
static uint8_t debounce_count = 0u;

static uint8_t activated_event_mask = 0u;
static uint8_t deactivated_event_mask = 0u;

static uint8_t IRSensor_IsValid(ir_sensor_id_t sensor)
{
    return ((uint8_t)sensor < IR_SENSOR_COUNT);
}

static uint8_t IRSensors_ReadHardwareActiveMask(void)
{
    uint8_t raw;

    raw = ir_sensor_status_reg_Read() & IR_SENSOR_ALL_MASK;

    if (sensor_polarity == IR_SENSOR_ACTIVE_LOW)
    {
        raw = ((uint8_t)(~raw)) & IR_SENSOR_ALL_MASK;
    }

    return raw;
}

void IRSensors_Init(ir_sensor_polarity_t active_polarity)
{
    sensor_polarity = active_polarity;

    stable_active_mask = IRSensors_ReadHardwareActiveMask();
    previous_stable_active_mask = stable_active_mask;
    candidate_active_mask = stable_active_mask;

    debounce_count = 0u;

    activated_event_mask = 0u;
    deactivated_event_mask = 0u;
}

void IRSensors_Update(void)
{
    uint8_t raw_active_mask;
    uint8_t changed_mask;

    raw_active_mask = IRSensors_ReadHardwareActiveMask();

    if (raw_active_mask == candidate_active_mask)
    {
        if (debounce_count < IR_SENSOR_DEBOUNCE_SAMPLES)
        {
            debounce_count++;
        }
    }
    else
    {
        candidate_active_mask = raw_active_mask;
        debounce_count = 0u;
    }

    if (debounce_count >= IR_SENSOR_DEBOUNCE_SAMPLES)
    {
        if (stable_active_mask != candidate_active_mask)
        {
            previous_stable_active_mask = stable_active_mask;
            stable_active_mask = candidate_active_mask;

            changed_mask = stable_active_mask ^ previous_stable_active_mask;

            /*
             * inactive -> active
             */
            activated_event_mask |= changed_mask & stable_active_mask;

            /*
             * active -> inactive
             */
            deactivated_event_mask |= changed_mask & previous_stable_active_mask;
        }
    }
}

uint8_t IRSensor_ReadRaw(ir_sensor_id_t sensor)
{
    if (!IRSensor_IsValid(sensor))
    {
        return 0u;
    }

    return (IRSensors_ReadHardwareActiveMask() & (uint8_t)(1u << sensor)) ? 1u : 0u;
}

uint8_t IRSensors_ReadRawMask(void)
{
    return IRSensors_ReadHardwareActiveMask();
}

uint8_t IRSensor_IsActive(ir_sensor_id_t sensor)
{
    if (!IRSensor_IsValid(sensor))
    {
        return 0u;
    }

    return (stable_active_mask & (uint8_t)(1u << sensor)) ? 1u : 0u;
}

uint8_t IRSensors_GetActiveMask(void)
{
    return stable_active_mask;
}

uint8_t IRSensor_WasActivated(ir_sensor_id_t sensor)
{
    uint8_t mask;

    if (!IRSensor_IsValid(sensor))
    {
        return 0u;
    }

    mask = (uint8_t)(1u << sensor);

    if (activated_event_mask & mask)
    {
        activated_event_mask &= (uint8_t)(~mask);
        return 1u;
    }

    return 0u;
}

uint8_t IRSensor_WasDeactivated(ir_sensor_id_t sensor)
{
    uint8_t mask;

    if (!IRSensor_IsValid(sensor))
    {
        return 0u;
    }

    mask = (uint8_t)(1u << sensor);

    if (deactivated_event_mask & mask)
    {
        deactivated_event_mask &= (uint8_t)(~mask);
        return 1u;
    }

    return 0u;
}

void IRSensors_ClearEvents(void)
{
    activated_event_mask = 0u;
    deactivated_event_mask = 0u;
}