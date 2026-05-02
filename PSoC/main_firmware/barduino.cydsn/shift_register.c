#include "shift_register.h"

static uint16_t shift_register_state = 0u;

static void ShiftRegister_PulseClock(void)
{
    SR_CLK_Write(1u);
    CyDelayUs(1u);
    SR_CLK_Write(0u);
    CyDelayUs(1u);
}

static void ShiftRegister_PulseLatch(void)
{
    SR_LATCH_Write(1u);
    CyDelayUs(1u);
    SR_LATCH_Write(0u);
    CyDelayUs(1u);
}

static void ShiftRegister_DisableOutputs(void)
{
    /*
     * 74HC595 OE is active-low:
     *     OE = 1 -> outputs high impedance / disabled
     *     OE = 0 -> outputs enabled
     */
    SR_OE_Write(1u);
}

static void ShiftRegister_EnableOutputs(void)
{
    SR_OE_Write(0u);
}

void ShiftRegister_Init(void)
{
    /*
     * Keep outputs disabled while initializing so the pumps cannot glitch on.
     */
    ShiftRegister_DisableOutputs();

    SR_DATA_Write(0u);
    SR_CLK_Write(0u);
    SR_LATCH_Write(0u);

    /*
     * Shift and latch all zeros while outputs are still disabled.
     */
    shift_register_state = 0u;
    ShiftRegister_Update();

    /*
     * Only enable outputs after known-safe zeros have been latched.
     */
    ShiftRegister_EnableOutputs();
}

void ShiftRegister_Write16(uint16_t value)
{
    shift_register_state = value;
    ShiftRegister_Update();
}

void ShiftRegister_WriteBit(uint8_t bit_index, uint8_t value)
{
    ShiftRegister_SetBit(bit_index, value);
    ShiftRegister_Update();
}

void ShiftRegister_SetBit(uint8_t bit_index, uint8_t value)
{
    if (bit_index >= SHIFT_REGISTER_BITS)
    {
        return;
    }

    if (value)
    {
        shift_register_state |= (uint16_t)(1u << bit_index);
    }
    else
    {
        shift_register_state &= (uint16_t)(~(uint16_t)(1u << bit_index));
    }
}

uint8_t ShiftRegister_GetBit(uint8_t bit_index)
{
    if (bit_index >= SHIFT_REGISTER_BITS)
    {
        return 0u;
    }

    return (shift_register_state & (uint16_t)(1u << bit_index)) ? 1u : 0u;
}

void ShiftRegister_Update(void)
{
    /*
     * Sends bit 15 first and bit 0 last.
     *
     * Software bit numbering:
     *   bit 0  = first 74HC595 Q0
     *   bit 7  = first 74HC595 Q7
     *   bit 8  = second 74HC595 Q0
     *   bit 15 = second 74HC595 Q7
     */
    for (int8_t bit = 15; bit >= 0; bit--)
    {
        if (shift_register_state & (uint16_t)(1u << bit))
        {
            SR_DATA_Write(1u);
        }
        else
        {
            SR_DATA_Write(0u);
        }

        ShiftRegister_PulseClock();
    }

    ShiftRegister_PulseLatch();
}

void ShiftRegister_Clear(void)
{
    shift_register_state = 0u;
    ShiftRegister_Update();
}

uint16_t ShiftRegister_GetOutputState(void)
{
    return shift_register_state;
}