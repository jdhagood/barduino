#ifndef DRINK_DB_H
#define DRINK_DB_H

#include <project.h>
#include <stdint.h>

#define DRINK_DB_NAME_MAX_LEN    20u
#define DRINK_DB_PUMP_COUNT      6u

/*
 * Software maximum. Actual capacity is also limited by CYDEV_EE_SIZE.
 *
 * Capacity with 32-byte records:
 *   512 B EEPROM -> 15 drinks
 *   1 KB EEPROM  -> 31 drinks
 *   2 KB EEPROM  -> 63 drinks
 *
 * 32 is a conservative default. Increase if you confirm your device has enough EEPROM.
 */
#define DRINK_DB_MAX_DRINKS      32u

typedef struct
{
    char name[DRINK_DB_NAME_MAX_LEN + 1u];      // 20 chars + null terminator
    uint8_t amount[DRINK_DB_PUMP_COUNT];        // pump amounts 0–9
    uint8_t valid;
} drink_t;

void DrinkDB_Init(void);

/*
 * Clears EEPROM drink database and RAM database.
 * This deletes all saved drinks.
 */
uint8_t DrinkDB_FormatEEPROM(void);

/*
 * Reloads drink database from EEPROM into RAM.
 * Usually only called inside DrinkDB_Init().
 */
uint8_t DrinkDB_LoadFromEEPROM(void);

/*
 * Returns the actual max number of drinks supported by the current EEPROM size
 * and DRINK_DB_MAX_DRINKS.
 */
uint16_t DrinkDB_GetMaxCapacity(void);

/*
 * Number of currently valid drinks.
 */
uint8_t DrinkDB_GetCount(void);

/*
 * Gets drink by logical index: 0..DrinkDB_GetCount()-1.
 * Logical indices skip deleted/invalid slots.
 */
const drink_t *DrinkDB_GetDrink(uint8_t index);

/*
 * Adds drink to first free/deleted slot.
 * Returns 1 on success, 0 on failure.
 */
uint8_t DrinkDB_AddDrink(const drink_t *drink);

/*
 * Updates existing drink by logical index.
 * Returns 1 on success, 0 on failure.
 */
uint8_t DrinkDB_UpdateDrink(uint8_t index, const drink_t *drink);

/*
 * Deletes drink by logical index.
 * Delete only invalidates the slot; it does not compact EEPROM.
 * Returns 1 on success, 0 on failure.
 */
uint8_t DrinkDB_DeleteDrink(uint8_t index);

/*
 * Optional helper for debug.
 * Saves a physical EEPROM slot from RAM.
 */
uint8_t DrinkDB_SaveSlot(uint8_t physical_slot);

#endif