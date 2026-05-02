#ifndef PUMPS_H
#define PUMPS_H

#include <project.h>
#include <stdint.h>

#define PUMP_COUNT 6u

typedef enum
{
    PUMP_0 = 0,
    PUMP_1,
    PUMP_2,
    PUMP_3,
    PUMP_4,
    PUMP_5
} pump_id_t;

typedef enum
{
    PUMP_DIR_FORWARD = 0,
    PUMP_DIR_REVERSE = 1
} pump_dir_t;

typedef enum
{
    PUMP_STATE_OFF = 0,
    PUMP_STATE_FORWARD,
    PUMP_STATE_REVERSE
} pump_state_t;

void Pumps_Init(void);
void Pumps_Update(void);

void Pumps_StopAll(void);
void Pump_Stop(pump_id_t pump);
void Pump_Run(pump_id_t pump, pump_dir_t direction);

void Pump_DispenseBlocking(pump_id_t pump,
                           uint32_t dispense_time_ms,
                           uint32_t reverse_time_ms);

void Pump_DispenseNonBlocking(pump_id_t pump,
                              uint32_t dispense_time_ms,
                              uint32_t reverse_time_ms);

uint8_t Pump_IsBusy(pump_id_t pump);
pump_state_t Pump_GetState(pump_id_t pump);

#endif