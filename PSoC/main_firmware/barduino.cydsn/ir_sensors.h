#ifndef IR_SENSORS_H
#define IR_SENSORS_H

#include <project.h>
#include <stdint.h>

#define IR_SENSOR_COUNT 6u

typedef enum
{
    IR_SENSOR_0 = 0,
    IR_SENSOR_1,
    IR_SENSOR_2,
    IR_SENSOR_3,
    IR_SENSOR_4,
    IR_SENSOR_5
} ir_sensor_id_t;

/*
 * Polarity options:
 *
 * IR_SENSOR_ACTIVE_HIGH:
 *     raw status bit = 1 means sensor active/detected
 *
 * IR_SENSOR_ACTIVE_LOW:
 *     raw status bit = 0 means sensor active/detected
 */
typedef enum
{
    IR_SENSOR_ACTIVE_HIGH = 0,
    IR_SENSOR_ACTIVE_LOW  = 1
} ir_sensor_polarity_t;

/*
 * Call once during startup.
 *
 * active_polarity:
 *     choose whether the sensors read active-high or active-low.
 */
void IRSensors_Init(ir_sensor_polarity_t active_polarity);

/*
 * Call periodically from the main loop.
 *
 * Recommended call rate:
 *     every 1 ms to 10 ms
 *
 * Used for debouncing/filtering and edge detection.
 */
void IRSensors_Update(void);

/*
 * Raw unfiltered active-state read.
 *
 * Returns:
 *     1 if sensor is currently active
 *     0 if sensor is currently inactive
 */
uint8_t IRSensor_ReadRaw(ir_sensor_id_t sensor);

/*
 * Raw unfiltered active-state bitmask.
 *
 * bit = 1 means active/detected.
 */
uint8_t IRSensors_ReadRawMask(void);

/*
 * Debounced/filtered sensor state.
 *
 * Returns:
 *     1 if sensor is debounced active
 *     0 if sensor is debounced inactive
 */
uint8_t IRSensor_IsActive(ir_sensor_id_t sensor);

/*
 * Debounced/filtered active-state bitmask.
 *
 * bit = 1 means active/detected.
 */
uint8_t IRSensors_GetActiveMask(void);

/*
 * One-shot event:
 * returns 1 once when sensor transitions inactive -> active.
 */
uint8_t IRSensor_WasActivated(ir_sensor_id_t sensor);

/*
 * One-shot event:
 * returns 1 once when sensor transitions active -> inactive.
 */
uint8_t IRSensor_WasDeactivated(ir_sensor_id_t sensor);

/*
 * Clear queued edge events.
 */
void IRSensors_ClearEvents(void);

#endif