#ifndef USB_SERIAL_H
#define USB_SERIAL_H

#include <project.h>
#include <stdint.h>
#include <stdarg.h>

#ifndef USB_SERIAL_TX_BUFFER_SIZE
#define USB_SERIAL_TX_BUFFER_SIZE 128u
#endif

#ifndef USB_SERIAL_RX_LINE_BUFFER_SIZE
#define USB_SERIAL_RX_LINE_BUFFER_SIZE 64u
#endif

/*
 * Start USBFS CDC serial.
 *
 * timeout_ms:
 *     0      -> wait forever for USB enumeration
 *     nonzero -> give up after timeout_ms
 *
 * Returns:
 *     1 if connected/configured
 *     0 if timeout occurred
 */
uint8_t USBSerial_Init(uint32_t timeout_ms);

/*
 * Call periodically from main loop.
 * This handles reconnect and collects incoming serial bytes into line strings.
 */
void USBSerial_Update(void);

/*
 * Returns 1 if USB is configured.
 */
uint8_t USBSerial_IsConnected(void);

/*
 * Arduino-style boolean serial available.
 *
 * Returns:
 *     1 if a complete command/string line is ready
 *     0 otherwise
 */
uint8_t USBSerial_SerialAvailable(void);

/*
 * Existing lower-level byte-count available.
 *
 * Returns number of raw bytes waiting in the USB CDC RX buffer.
 */
uint16_t USBSerial_Available(void);

/*
 * Read one complete received line into user buffer.
 *
 * The line terminator '\r' or '\n' is removed.
 *
 * Returns:
 *     1 if a line was copied
 *     0 if no complete line was available
 */
uint8_t USBSerial_ReadLine(char *buffer, uint16_t max_length);

/*
 * Arduino-String-like helper.
 *
 * Returns:
 *     pointer to internal null-terminated command string if available
 *     NULL if no complete command is available
 *
 * WARNING:
 *     The returned pointer points to an internal static buffer.
 *     Copy it if you need to keep it.
 */
const char *USBSerial_ReadString(void);

/*
 * Raw byte read.
 *
 * Returns:
 *     1 if a byte was read
 *     0 otherwise
 */
uint8_t USBSerial_ReadByte(uint8_t *byte);

/*
 * Raw data read.
 */
uint16_t USBSerial_ReadData(uint8_t *buffer, uint16_t max_length);

/*
 * Transmit helpers.
 */
void USBSerial_WriteString(const char *str);
void USBSerial_WriteLine(const char *str);
void USBSerial_WriteData(const uint8_t *data, uint16_t length);
void USBSerial_WriteChar(char c);
void USBSerial_Printf(const char *fmt, ...);

/*
 * Clear raw RX and pending parsed command line.
 */
void USBSerial_ClearRxBuffer(void);

#endif