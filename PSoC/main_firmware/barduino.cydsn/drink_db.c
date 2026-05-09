#include "drink_db.h"
#include <string.h>

/*
 * EEPROM layout:
 *
 * Row 0:
 *   16-byte database header
 *
 * Rows 1-2:
 *   drink slot 0, 32 bytes
 *
 * Rows 3-4:
 *   drink slot 1, 32 bytes
 *
 * etc.
 *
 * Each drink record is 32 bytes:
 *
 *   byte 0-20   name[21]
 *   byte 21-26  pump amounts[6]
 *   byte 27     valid marker
 *   byte 28     checksum
 *   byte 29-31  reserved
 */

#define DRINK_DB_MAGIC_0              ((uint8_t)'B')
#define DRINK_DB_MAGIC_1              ((uint8_t)'D')
#define DRINK_DB_VERSION              (1u)

#define DRINK_DB_HEADER_ROW           (0u)
#define DRINK_DB_FIRST_SLOT_ROW       (1u)

#define DRINK_RECORD_SIZE             (32u)
#define DRINK_RECORD_ROWS             (2u)

#define DRINK_RECORD_VALID            (0xA5u)
#define DRINK_RECORD_INVALID          (0x00u)

#define DRINK_RECORD_NAME_OFFSET      (0u)
#define DRINK_RECORD_AMOUNT_OFFSET    (21u)
#define DRINK_RECORD_VALID_OFFSET     (27u)
#define DRINK_RECORD_CHECKSUM_OFFSET  (28u)
#define DRINK_RECORD_RESERVED0        (29u)
#define DRINK_RECORD_RESERVED1        (30u)
#define DRINK_RECORD_RESERVED2        (31u)

#define HEADER_MAGIC0_OFFSET          (0u)
#define HEADER_MAGIC1_OFFSET          (1u)
#define HEADER_VERSION_OFFSET         (2u)
#define HEADER_RECORD_SIZE_OFFSET     (3u)
#define HEADER_MAX_SLOTS_OFFSET       (4u)
#define HEADER_CHECKSUM_OFFSET        (15u)

#if CYDEV_EEPROM_ROW_SIZE != 16
#error "drink_db.c assumes 16-byte EEPROM rows."
#endif

static drink_t drinks[DRINK_DB_MAX_DRINKS];
static uint8_t slot_valid[DRINK_DB_MAX_DRINKS];
static uint8_t drink_count = 0u;

static uint8_t db_initialized = 0u;

/* ---------------- Internal helpers ---------------- */

static uint16_t get_physical_capacity_raw(void)
{
    uint16_t available_bytes;
    uint16_t capacity;

    if (CYDEV_EE_SIZE <= CYDEV_EEPROM_ROW_SIZE)
    {
        return 0u;
    }

    available_bytes = (uint16_t)(CYDEV_EE_SIZE - CYDEV_EEPROM_ROW_SIZE);
    capacity = (uint16_t)(available_bytes / DRINK_RECORD_SIZE);

    return capacity;
}

uint16_t DrinkDB_GetMaxCapacity(void)
{
    uint16_t capacity = get_physical_capacity_raw();

    if (capacity > DRINK_DB_MAX_DRINKS)
    {
        capacity = DRINK_DB_MAX_DRINKS;
    }

    return capacity;
}

static uint8_t checksum_bytes_skip_index(const uint8_t *data,
                                         uint8_t length,
                                         uint8_t skip_index)
{
    uint8_t checksum = 0u;

    for (uint8_t i = 0u; i < length; i++)
    {
        if (i != skip_index)
        {
            checksum ^= data[i];
        }
    }

    return checksum;
}

static uint8_t checksum_record(const uint8_t record[DRINK_RECORD_SIZE])
{
    return checksum_bytes_skip_index(record,
                                     DRINK_RECORD_SIZE,
                                     DRINK_RECORD_CHECKSUM_OFFSET);
}

static uint8_t checksum_header(const uint8_t header[CYDEV_EEPROM_ROW_SIZE])
{
    return checksum_bytes_skip_index(header,
                                     CYDEV_EEPROM_ROW_SIZE,
                                     HEADER_CHECKSUM_OFFSET);
}

static uint8_t is_valid_logical_index(uint8_t logical_index)
{
    return (logical_index < drink_count) ? 1u : 0u;
}

static uint8_t logical_to_physical_slot(uint8_t logical_index, uint8_t *physical_slot)
{
    uint8_t logical_seen = 0u;
    uint16_t capacity = DrinkDB_GetMaxCapacity();

    if (physical_slot == 0)
    {
        return 0u;
    }

    for (uint8_t slot = 0u; slot < capacity; slot++)
    {
        if (slot_valid[slot])
        {
            if (logical_seen == logical_index)
            {
                *physical_slot = slot;
                return 1u;
            }

            logical_seen++;
        }
    }

    return 0u;
}

static uint8_t find_first_free_slot(uint8_t *physical_slot)
{
    uint16_t capacity = DrinkDB_GetMaxCapacity();

    if (physical_slot == 0)
    {
        return 0u;
    }

    for (uint8_t slot = 0u; slot < capacity; slot++)
    {
        if (!slot_valid[slot])
        {
            *physical_slot = slot;
            return 1u;
        }
    }

    return 0u;
}

static uint8_t is_name_nonempty(const char *name)
{
    return ((name != 0) && (name[0] != '\0')) ? 1u : 0u;
}

static void sanitize_drink(drink_t *drink)
{
    if (drink == 0)
    {
        return;
    }

    drink->name[DRINK_DB_NAME_MAX_LEN] = '\0';

    for (uint8_t i = 0u; i < DRINK_DB_PUMP_COUNT; i++)
    {
        if (drink->amount[i] > 9u)
        {
            drink->amount[i] = 9u;
        }
    }

    drink->valid = 1u;
}

static uint8_t physical_slot_to_first_row(uint8_t physical_slot)
{
    return (uint8_t)(DRINK_DB_FIRST_SLOT_ROW +
                    ((uint8_t)physical_slot * DRINK_RECORD_ROWS));
}

static void clear_ram_database(void)
{
    memset(drinks, 0, sizeof(drinks));
    memset(slot_valid, 0, sizeof(slot_valid));
    drink_count = 0u;
}

static void drink_to_record(const drink_t *drink,
                            uint8_t valid_marker,
                            uint8_t record[DRINK_RECORD_SIZE])
{
    memset(record, 0, DRINK_RECORD_SIZE);

    if (drink == 0)
    {
        return;
    }

    for (uint8_t i = 0u; i < (DRINK_DB_NAME_MAX_LEN + 1u); i++)
    {
        record[DRINK_RECORD_NAME_OFFSET + i] = (uint8_t)drink->name[i];
    }

    record[DRINK_RECORD_NAME_OFFSET + DRINK_DB_NAME_MAX_LEN] = '\0';

    for (uint8_t i = 0u; i < DRINK_DB_PUMP_COUNT; i++)
    {
        uint8_t amount = drink->amount[i];

        if (amount > 9u)
        {
            amount = 9u;
        }

        record[DRINK_RECORD_AMOUNT_OFFSET + i] = amount;
    }

    record[DRINK_RECORD_VALID_OFFSET] = valid_marker;
    record[DRINK_RECORD_RESERVED0] = 0u;
    record[DRINK_RECORD_RESERVED1] = 0u;
    record[DRINK_RECORD_RESERVED2] = 0u;

    record[DRINK_RECORD_CHECKSUM_OFFSET] = checksum_record(record);
}

static uint8_t record_to_drink(const uint8_t record[DRINK_RECORD_SIZE],
                               drink_t *drink)
{
    uint8_t expected_checksum;

    if ((record == 0) || (drink == 0))
    {
        return 0u;
    }

    if (record[DRINK_RECORD_VALID_OFFSET] != DRINK_RECORD_VALID)
    {
        return 0u;
    }

    expected_checksum = checksum_record(record);

    if (record[DRINK_RECORD_CHECKSUM_OFFSET] != expected_checksum)
    {
        return 0u;
    }

    memset(drink, 0, sizeof(drink_t));

    for (uint8_t i = 0u; i < (DRINK_DB_NAME_MAX_LEN + 1u); i++)
    {
        drink->name[i] = (char)record[DRINK_RECORD_NAME_OFFSET + i];
    }

    drink->name[DRINK_DB_NAME_MAX_LEN] = '\0';

    if (!is_name_nonempty(drink->name))
    {
        return 0u;
    }

    for (uint8_t i = 0u; i < DRINK_DB_PUMP_COUNT; i++)
    {
        uint8_t amount = record[DRINK_RECORD_AMOUNT_OFFSET + i];

        if (amount > 9u)
        {
            return 0u;
        }

        drink->amount[i] = amount;
    }

    drink->valid = 1u;

    return 1u;
}

static cystatus write_eeprom_row(uint8_t row_number, const uint8_t row_data[CYDEV_EEPROM_ROW_SIZE])
{
    return DrinkEEPROM_Write(row_data, row_number);
}

static void read_eeprom_row(uint8_t row_number, uint8_t row_data[CYDEV_EEPROM_ROW_SIZE])
{
    uint16_t base_addr = (uint16_t)row_number * CYDEV_EEPROM_ROW_SIZE;

    for (uint8_t i = 0u; i < CYDEV_EEPROM_ROW_SIZE; i++)
    {
        row_data[i] = DrinkEEPROM_ReadByte((uint16_t)(base_addr + i));
    }
}

static void read_record_from_eeprom(uint8_t physical_slot,
                                    uint8_t record[DRINK_RECORD_SIZE])
{
    uint8_t first_row = physical_slot_to_first_row(physical_slot);

    read_eeprom_row(first_row, &record[0u]);
    read_eeprom_row((uint8_t)(first_row + 1u), &record[16u]);
}

static uint8_t write_header(void)
{
    uint8_t header[CYDEV_EEPROM_ROW_SIZE];
    cystatus status;

    memset(header, 0, sizeof(header));

    header[HEADER_MAGIC0_OFFSET] = DRINK_DB_MAGIC_0;
    header[HEADER_MAGIC1_OFFSET] = DRINK_DB_MAGIC_1;
    header[HEADER_VERSION_OFFSET] = DRINK_DB_VERSION;
    header[HEADER_RECORD_SIZE_OFFSET] = DRINK_RECORD_SIZE;
    header[HEADER_MAX_SLOTS_OFFSET] = (uint8_t)DrinkDB_GetMaxCapacity();

    header[HEADER_CHECKSUM_OFFSET] = checksum_header(header);

    DrinkEEPROM_UpdateTemperature();

    status = write_eeprom_row(DRINK_DB_HEADER_ROW, header);

    return (status == CYRET_SUCCESS) ? 1u : 0u;
}

static uint8_t read_and_validate_header(void)
{
    uint8_t header[CYDEV_EEPROM_ROW_SIZE];

    read_eeprom_row(DRINK_DB_HEADER_ROW, header);

    if (header[HEADER_MAGIC0_OFFSET] != DRINK_DB_MAGIC_0)
    {
        return 0u;
    }

    if (header[HEADER_MAGIC1_OFFSET] != DRINK_DB_MAGIC_1)
    {
        return 0u;
    }

    if (header[HEADER_VERSION_OFFSET] != DRINK_DB_VERSION)
    {
        return 0u;
    }

    if (header[HEADER_RECORD_SIZE_OFFSET] != DRINK_RECORD_SIZE)
    {
        return 0u;
    }

    if (header[HEADER_CHECKSUM_OFFSET] != checksum_header(header))
    {
        return 0u;
    }

    return 1u;
}

/* ---------------- Public API ---------------- */

void DrinkDB_Init(void)
{
    DrinkEEPROM_Start();

    /*
     * Required/recommended before EEPROM write operations if temperature may
     * have changed significantly. Safe to call during init.
     */
    DrinkEEPROM_UpdateTemperature();

    clear_ram_database();

    if (!DrinkDB_LoadFromEEPROM())
    {
        DrinkDB_FormatEEPROM();
    }

    db_initialized = 1u;
}

uint8_t DrinkDB_FormatEEPROM(void)
{
    uint16_t capacity = DrinkDB_GetMaxCapacity();
    uint8_t empty_record[DRINK_RECORD_SIZE];
    cystatus status;

    clear_ram_database();

    if (capacity == 0u)
    {
        return 0u;
    }

    DrinkEEPROM_UpdateTemperature();

    if (!write_header())
    {
        return 0u;
    }

    /*
     * Clear all configured slots to invalid records.
     */
    memset(empty_record, 0, sizeof(empty_record));
    empty_record[DRINK_RECORD_VALID_OFFSET] = DRINK_RECORD_INVALID;
    empty_record[DRINK_RECORD_CHECKSUM_OFFSET] = checksum_record(empty_record);

    for (uint8_t slot = 0u; slot < capacity; slot++)
    {
        uint8_t first_row = physical_slot_to_first_row(slot);

        status = write_eeprom_row(first_row, &empty_record[0u]);
        if (status != CYRET_SUCCESS)
        {
            return 0u;
        }

        status = write_eeprom_row((uint8_t)(first_row + 1u), &empty_record[16u]);
        if (status != CYRET_SUCCESS)
        {
            return 0u;
        }
    }

    return 1u;
}

uint8_t DrinkDB_LoadFromEEPROM(void)
{
    uint16_t capacity = DrinkDB_GetMaxCapacity();
    uint8_t record[DRINK_RECORD_SIZE];

    clear_ram_database();

    if (capacity == 0u)
    {
        return 0u;
    }

    if (!read_and_validate_header())
    {
        return 0u;
    }

    for (uint8_t slot = 0u; slot < capacity; slot++)
    {
        drink_t temp_drink;

        read_record_from_eeprom(slot, record);

        if (record_to_drink(record, &temp_drink))
        {
            drinks[slot] = temp_drink;
            slot_valid[slot] = 1u;
            drink_count++;
        }
        else
        {
            memset(&drinks[slot], 0, sizeof(drink_t));
            slot_valid[slot] = 0u;
        }
    }

    return 1u;
}

uint8_t DrinkDB_GetCount(void)
{
    return drink_count;
}

const drink_t *DrinkDB_GetDrink(uint8_t index)
{
    uint8_t physical_slot;

    if (!is_valid_logical_index(index))
    {
        return 0;
    }

    if (!logical_to_physical_slot(index, &physical_slot))
    {
        return 0;
    }

    return &drinks[physical_slot];
}

uint8_t DrinkDB_SaveSlot(uint8_t physical_slot)
{
    uint16_t capacity = DrinkDB_GetMaxCapacity();
    uint8_t first_row;
    uint8_t record[DRINK_RECORD_SIZE];
    cystatus status;

    if (physical_slot >= capacity)
    {
        return 0u;
    }

    first_row = physical_slot_to_first_row(physical_slot);

    DrinkEEPROM_UpdateTemperature();

    /*
     * Power-failure safer write:
     *
     * 1. Write record as invalid.
     * 2. Then write second row again with valid marker + checksum.
     *
     * If power fails before the final row write, the record will not be treated
     * as valid on next boot.
     */
    drink_to_record(&drinks[physical_slot], DRINK_RECORD_INVALID, record);

    status = write_eeprom_row(first_row, &record[0u]);
    if (status != CYRET_SUCCESS)
    {
        return 0u;
    }

    status = write_eeprom_row((uint8_t)(first_row + 1u), &record[16u]);
    if (status != CYRET_SUCCESS)
    {
        return 0u;
    }

    if (slot_valid[physical_slot])
    {
        drink_to_record(&drinks[physical_slot], DRINK_RECORD_VALID, record);

        /*
         * Row 0 is unchanged by switching valid/checksum because those fields
         * live in row 1, so only rewrite row 1.
         */
        status = write_eeprom_row((uint8_t)(first_row + 1u), &record[16u]);
        if (status != CYRET_SUCCESS)
        {
            return 0u;
        }
    }

    return 1u;
}

uint8_t DrinkDB_AddDrink(const drink_t *drink)
{
    uint8_t physical_slot;
    drink_t temp;

    if ((drink == 0) || !is_name_nonempty(drink->name))
    {
        return 0u;
    }

    if (!find_first_free_slot(&physical_slot))
    {
        return 0u;
    }

    temp = *drink;
    sanitize_drink(&temp);

    drinks[physical_slot] = temp;
    slot_valid[physical_slot] = 1u;

    if (!DrinkDB_SaveSlot(physical_slot))
    {
        /*
         * Roll back RAM state if EEPROM write failed.
         */
        memset(&drinks[physical_slot], 0, sizeof(drink_t));
        slot_valid[physical_slot] = 0u;
        return 0u;
    }

    drink_count++;
    return 1u;
}

uint8_t DrinkDB_UpdateDrink(uint8_t index, const drink_t *drink)
{
    uint8_t physical_slot;
    drink_t old_drink;
    drink_t temp;

    if ((drink == 0) || !is_valid_logical_index(index) || !is_name_nonempty(drink->name))
    {
        return 0u;
    }

    if (!logical_to_physical_slot(index, &physical_slot))
    {
        return 0u;
    }

    old_drink = drinks[physical_slot];

    temp = *drink;
    sanitize_drink(&temp);

    drinks[physical_slot] = temp;
    slot_valid[physical_slot] = 1u;

    if (!DrinkDB_SaveSlot(physical_slot))
    {
        drinks[physical_slot] = old_drink;
        slot_valid[physical_slot] = 1u;
        return 0u;
    }

    return 1u;
}

uint8_t DrinkDB_DeleteDrink(uint8_t index)
{
    uint8_t physical_slot;
    drink_t old_drink;

    if (!is_valid_logical_index(index))
    {
        return 0u;
    }

    if (!logical_to_physical_slot(index, &physical_slot))
    {
        return 0u;
    }

    old_drink = drinks[physical_slot];

    /*
     * Mark invalid in RAM and save invalid marker to EEPROM.
     * We do not compact other slots.
     */
    slot_valid[physical_slot] = 0u;
    drinks[physical_slot].valid = 0u;

    if (!DrinkDB_SaveSlot(physical_slot))
    {
        drinks[physical_slot] = old_drink;
        slot_valid[physical_slot] = 1u;
        return 0u;
    }

    memset(&drinks[physical_slot], 0, sizeof(drink_t));

    if (drink_count > 0u)
    {
        drink_count--;
    }

    return 1u;
}