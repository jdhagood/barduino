#include "tmc2209_stepper.h"

#define TMC2209_EN_ACTIVE_LEVEL      (0u)
#define TMC2209_EN_INACTIVE_LEVEL    (1u)

#define STEP_PULSE_TICKS             (1u)

static tmc2209_config_t motor_cfg;

static volatile tmc2209_motion_state_t motion_state = TMC2209_MOTION_IDLE;

static volatile uint32_t total_steps = 0;
static volatile uint32_t steps_completed = 0;

static volatile uint32_t target_sps = 0;
static volatile uint32_t current_sps = 0;
static volatile uint32_t accel_sps2 = 0;
static volatile uint8_t accel_enabled = 0;

static volatile uint32_t phase_accum = 0;
static volatile uint32_t phase_increment = 0;

static volatile uint8_t step_pin_high = 0;
static volatile uint8_t step_high_ticks_remaining = 0;

static uint32_t rpm_to_steps_per_second(uint16_t rpm)
{
    uint32_t steps_per_rev;

    steps_per_rev = (uint32_t)motor_cfg.full_steps_per_rev *
                    (uint32_t)motor_cfg.microsteps;

    return ((uint32_t)rpm * steps_per_rev) / 60u;
}

static void update_phase_increment_from_speed(void)
{
    /*
     * DDS-style accumulator:
     *
     * A step is emitted whenever phase_accum overflows 32 bits.
     *
     * phase_increment = desired_step_frequency / timer_tick_frequency * 2^32
     */
    uint64_t increment;

    if (current_sps == 0)
    {
        phase_increment = 0;
        return;
    }

    increment = ((uint64_t)current_sps << 32) / motor_cfg.timer_tick_hz;
    phase_increment = (uint32_t)increment;
}

static void set_current_speed_sps(uint32_t sps)
{
    current_sps = sps;
    update_phase_increment_from_speed();
}

void TMC2209_Init(const tmc2209_config_t *config)
{
    motor_cfg = *config;

    if (motor_cfg.full_steps_per_rev == 0)
        motor_cfg.full_steps_per_rev = 200;

    if (motor_cfg.microsteps == 0)
        motor_cfg.microsteps = 16;

    if (motor_cfg.timer_tick_hz == 0)
        motor_cfg.timer_tick_hz = 20000u;

    TMC_STEP_Write(0);
    TMC_DIR_Write(0);
    TMC2209_Disable();

    motion_state = TMC2209_MOTION_IDLE;

    total_steps = 0;
    steps_completed = 0;
    target_sps = 0;
    current_sps = 0;
    accel_sps2 = 0;
    accel_enabled = 0;
    phase_accum = 0;
    phase_increment = 0;
    step_pin_high = 0;
    step_high_ticks_remaining = 0;

    /*
     * Do not start Stepper_Timer here.
     * Start it in main.c after configuring interrupt mode and ISR vector.
     */
}

void TMC2209_Enable(void)
{
    TMC_EN_Write(TMC2209_EN_ACTIVE_LEVEL);
}

void TMC2209_Disable(void)
{
    TMC_EN_Write(TMC2209_EN_INACTIVE_LEVEL);
}

void TMC2209_SetDirection(tmc2209_dir_t dir)
{
    uint8_t level = (dir == TMC2209_DIR_REVERSE) ? 1u : 0u;

    if (motor_cfg.invert_dir)
    {
        level = !level;
    }

    TMC_DIR_Write(level);
}

void TMC2209_MoveStepsNonBlocking(tmc2209_dir_t direction,
                                  uint32_t steps,
                                  uint16_t rpm)
{
    uint32_t sps;

    if (steps == 0 || rpm == 0)
    {
        return;
    }

    sps = rpm_to_steps_per_second(rpm);

    TMC2209_SetDirection(direction);
    TMC2209_Enable();

    total_steps = steps;
    steps_completed = 0;

    target_sps = sps;
    accel_sps2 = 0;
    accel_enabled = 0;

    phase_accum = 0;
    set_current_speed_sps(sps);

    step_pin_high = 0;
    step_high_ticks_remaining = 0;
    TMC_STEP_Write(0);

    motion_state = TMC2209_MOTION_RUNNING;
}

void TMC2209_MoveStepsAccelNonBlocking(tmc2209_dir_t direction,
                                       uint32_t steps,
                                       uint16_t rpm,
                                       uint32_t accel)
{
    uint32_t sps;

    if (steps == 0 || rpm == 0)
    {
        return;
    }

    if (accel == 0)
    {
        TMC2209_MoveStepsNonBlocking(direction, steps, rpm);
        return;
    }

    sps = rpm_to_steps_per_second(rpm);

    TMC2209_SetDirection(direction);
    TMC2209_Enable();

    total_steps = steps;
    steps_completed = 0;

    target_sps = sps;
    accel_sps2 = accel;
    accel_enabled = 1;

    /*
     * Start from a low nonzero speed.
     * You can tune this.
     */
    if (sps < 50u)
    {
        set_current_speed_sps(sps);
    }
    else
    {
        set_current_speed_sps(50u);
    }

    phase_accum = 0;

    step_pin_high = 0;
    step_high_ticks_remaining = 0;
    TMC_STEP_Write(0);

    motion_state = TMC2209_MOTION_RUNNING;
}

void TMC2209_StopMotion(void)
{
    motion_state = TMC2209_MOTION_IDLE;

    total_steps = 0;
    steps_completed = 0;

    target_sps = 0;
    current_sps = 0;
    phase_increment = 0;
    phase_accum = 0;

    TMC_STEP_Write(0);
    step_pin_high = 0;
    step_high_ticks_remaining = 0;
}

uint8_t TMC2209_IsBusy(void)
{
    return motion_state == TMC2209_MOTION_RUNNING;
}

uint32_t TMC2209_GetStepsRemaining(void)
{
    if (steps_completed >= total_steps)
    {
        return 0;
    }

    return total_steps - steps_completed;
}

uint32_t TMC2209_GetStepsCompleted(void)
{
    return steps_completed;
}

uint32_t TMC2209_GetCurrentSpeedSPS(void)
{
    return current_sps;
}

static void update_acceleration_on_step(void)
{
    uint32_t steps_remaining;
    uint32_t stopping_distance;

    if (!accel_enabled)
    {
        return;
    }

    if (current_sps == 0)
    {
        set_current_speed_sps(1);
        return;
    }

    steps_remaining = total_steps - steps_completed;

    /*
     * stopping distance:
     *
     *     d = v^2 / (2a)
     *
     * v = steps/sec
     * a = steps/sec^2
     * d = steps
     */
    stopping_distance = (current_sps * current_sps) / (2u * accel_sps2);

    if (steps_remaining <= stopping_distance)
    {
        uint32_t dv = accel_sps2 / current_sps;

        if (dv == 0)
        {
            dv = 1;
        }

        if (current_sps > dv)
        {
            set_current_speed_sps(current_sps - dv);
        }
        else
        {
            set_current_speed_sps(1);
        }
    }
    else
    {
        if (current_sps < target_sps)
        {
            uint32_t dv = accel_sps2 / current_sps;

            if (dv == 0)
            {
                dv = 1;
            }

            if ((current_sps + dv) > target_sps)
            {
                set_current_speed_sps(target_sps);
            }
            else
            {
                set_current_speed_sps(current_sps + dv);
            }
        }
    }
}

void TMC2209_TimerISR(void)
{
    uint32_t old_phase;

    /*
     * Clear your timer interrupt here if your Timer component requires it.
     * Depending on PSoC component:
     *
     *     Stepper_Timer_ReadStatusRegister();
     *
     * is commonly enough to clear the interrupt.
     */
    Stepper_Timer_ReadStatusRegister();

    if (step_pin_high)
    {
        if (step_high_ticks_remaining > 0)
        {
            step_high_ticks_remaining--;
        }

        if (step_high_ticks_remaining == 0)
        {
            TMC_STEP_Write(0);
            step_pin_high = 0;
        }
    }

    if (motion_state != TMC2209_MOTION_RUNNING)
    {
        return;
    }

    if (steps_completed >= total_steps)
    {
        TMC2209_StopMotion();
        return;
    }

    if (phase_increment == 0)
    {
        return;
    }

    old_phase = phase_accum;
    phase_accum += phase_increment;

    /*
     * Overflow means it is time to emit one step.
     */
    if (phase_accum < old_phase)
    {
        TMC_STEP_Write(1);
        step_pin_high = 1;
        step_high_ticks_remaining = STEP_PULSE_TICKS;

        steps_completed++;

        update_acceleration_on_step();

        if (steps_completed >= total_steps)
        {
            /*
             * Do not immediately pull STEP low here.
             * Let the pulse high-time complete on the next ISR tick.
             */
            motion_state = TMC2209_MOTION_IDLE;
        }
    }
}