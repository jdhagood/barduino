#ifndef TMC2209_STEPPER_H
#define TMC2209_STEPPER_H

#include <project.h>
#include <stdint.h>

typedef enum
{
    TMC2209_DIR_FORWARD = 0,
    TMC2209_DIR_REVERSE = 1
} tmc2209_dir_t;

typedef enum
{
    TMC2209_DISABLED = 0,
    TMC2209_ENABLED  = 1
} tmc2209_enable_t;

typedef enum
{
    TMC2209_MOTION_IDLE = 0,
    TMC2209_MOTION_RUNNING = 1
} tmc2209_motion_state_t;

typedef struct
{
    uint16_t full_steps_per_rev;
    uint16_t microsteps;
    uint32_t timer_tick_hz;
    uint8_t invert_dir;
} tmc2209_config_t;

void TMC2209_Init(const tmc2209_config_t *config);

void TMC2209_Enable(void);
void TMC2209_Disable(void);
uint8_t TMC2209_GetEnabled(void);

void TMC2209_SetDirection(tmc2209_dir_t dir);

uint8_t TMC2209_SetMicrosteps(uint16_t microsteps);
uint16_t TMC2209_GetMicrosteps(void);

void TMC2209_MoveStepsNonBlocking(tmc2209_dir_t direction,
                                  uint32_t steps,
                                  uint16_t rpm);

void TMC2209_MoveStepsAccelNonBlocking(tmc2209_dir_t direction,
                                       uint32_t steps,
                                       uint16_t rpm,
                                       uint32_t accel_sps2);

void TMC2209_StopMotion(void);

uint8_t TMC2209_IsBusy(void);
uint32_t TMC2209_GetStepsRemaining(void);
uint32_t TMC2209_GetStepsCompleted(void);
uint32_t TMC2209_GetCurrentSpeedSPS(void);
uint16_t TMC2209_GetFullStepsPerRev(void);

void TMC2209_TimerISR(void);

#endif