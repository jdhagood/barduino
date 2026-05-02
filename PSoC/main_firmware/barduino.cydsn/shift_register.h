#ifndef SHIFT_REGISTER_H
#define SHIFT_REGISTER_H

#include <project.h>
#include <stdint.h>

#define SHIFT_REGISTER_BITS   16u

/*
 * Initialize the software output image and clear physical outputs.
 */
void ShiftRegister_Init(void);

/*
 * Write the full 16-bit output image and immediately latch it.
 *
 * bit 0  -> first shifted-register output Q0
 * bit 15 -> last shifted-register output
 */
void ShiftRegister_Write16(uint16_t value);

/*
 * Set or clear one output bit, then immediately update physical outputs.
 */
void ShiftRegister_WriteBit(uint8_t bit_index, uint8_t value);

/*
 * Set/clear one output bit in the software image only.
 * Call ShiftRegister_Update() afterward to latch all changes at once.
 */
void ShiftRegister_SetBit(uint8_t bit_index, uint8_t value);

/*
 * Read one bit from the software output image.
 */
uint8_t ShiftRegister_GetBit(uint8_t bit_index);

/*
 * Latch the current software output image to the physical 74HC595 outputs.
 */
void ShiftRegister_Update(void);

/*
 * Clear all outputs and latch.
 */
void ShiftRegister_Clear(void);

/*
 * Return the current software output image.
 */
uint16_t ShiftRegister_GetOutputState(void);

#endif