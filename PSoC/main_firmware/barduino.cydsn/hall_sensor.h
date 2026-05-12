#ifndef HALL_SENSOR_H
#define HALL_SENSOR_H

#include <project.h>
#include <stdint.h>

typedef enum
{
    HALL_SENSOR_ACTIVE_HIGH = 0,
    HALL_SENSOR_ACTIVE_LOW  = 1
} hall_sensor_polarity_t;

/*
 * Call once during startup.
 */
void HallSensor_Init(hall_sensor_polarity_t polarity);

/*
 * Call periodically from main loop.
 *
 * Recommended:
 *     every 1 ms to 10 ms
 */
void HallSensor_Update(void);

/*
 * Raw unfiltered read.
 *
 * Returns:
 *     1 if sensor is currently active
 *     0 if inactive
 */
uint8_t HallSensor_ReadRaw(void);

/*
 * Debounced/filtered state.
 *
 * Returns:
 *     1 if sensor is active
 *     0 if inactive
 */
uint8_t HallSensor_IsActive(void);

/*
 * One-shot event.
 *
 * Returns 1 once when the sensor transitions inactive -> active.
 */
uint8_t HallSensor_WasActivated(void);

/*
 * One-shot event.
 *
 * Returns 1 once when the sensor transitions active -> inactive.
 */
uint8_t HallSensor_WasDeactivated(void);

/*
 * Clears queued activation/deactivation events.
 */
void HallSensor_ClearEvents(void);

#endif