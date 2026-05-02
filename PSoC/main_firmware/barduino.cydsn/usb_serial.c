#include "usb_serial.h"
#include <stdio.h>
#include <string.h>

static uint8_t usb_serial_configured = 0u;

/*
 * Line-building buffer.
 */
static char rx_build_buffer[USB_SERIAL_RX_LINE_BUFFER_SIZE];
static uint16_t rx_build_index = 0u;

/*
 * Completed command buffer.
 *
 * This simple implementation stores one complete command at a time.
 * If another command arrives before you read the first one, the new one
 * is ignored until the first is consumed.
 */
static char rx_line_buffer[USB_SERIAL_RX_LINE_BUFFER_SIZE];
static volatile uint8_t rx_line_ready = 0u;

static void USBSerial_WaitForTxReady(void)
{
    while (USBSerial_IsConnected() && !USBUART_CDCIsReady())
    {
        /* wait */
    }
}

static void USBSerial_ProcessReceivedByte(uint8_t byte)
{
    /*
     * Treat either '\r' or '\n' as end-of-command.
     * This works with Arduino Serial Monitor:
     *     No line ending       -> not useful for commands
     *     Newline              -> OK
     *     Carriage return      -> OK
     *     Both NL & CR         -> OK
     */
    if ((byte == '\r') || (byte == '\n'))
    {
        /*
         * Ignore empty line endings.
         * This also handles the second character in CRLF or LFCR.
         */
        if (rx_build_index == 0u)
        {
            return;
        }

        /*
         * If previous complete line has not been consumed, do not overwrite it.
         */
        if (rx_line_ready)
        {
            rx_build_index = 0u;
            rx_build_buffer[0] = '\0';
            return;
        }

        rx_build_buffer[rx_build_index] = '\0';

        strncpy(rx_line_buffer, rx_build_buffer, USB_SERIAL_RX_LINE_BUFFER_SIZE);
        rx_line_buffer[USB_SERIAL_RX_LINE_BUFFER_SIZE - 1u] = '\0';

        rx_line_ready = 1u;

        rx_build_index = 0u;
        rx_build_buffer[0] = '\0';

        return;
    }

    /*
     * Optional: handle backspace/delete from terminal.
     */
    if ((byte == 0x08u) || (byte == 0x7Fu))
    {
        if (rx_build_index > 0u)
        {
            rx_build_index--;
            rx_build_buffer[rx_build_index] = '\0';
        }
        return;
    }

    /*
     * Store normal printable bytes.
     * Keep one byte free for null terminator.
     */
    if (rx_build_index < (USB_SERIAL_RX_LINE_BUFFER_SIZE - 1u))
    {
        rx_build_buffer[rx_build_index] = (char)byte;
        rx_build_index++;
        rx_build_buffer[rx_build_index] = '\0';
    }
    else
    {
        /*
         * Overflow policy:
         * discard current command to avoid executing a truncated command.
         */
        rx_build_index = 0u;
        rx_build_buffer[0] = '\0';
    }
}

uint8_t USBSerial_Init(uint32_t timeout_ms)
{
    uint32_t elapsed_ms = 0u;

    rx_build_index = 0u;
    rx_build_buffer[0] = '\0';
    rx_line_buffer[0] = '\0';
    rx_line_ready = 0u;

    USBUART_Start(0u, USBUART_5V_OPERATION);

    while (USBUART_GetConfiguration() == 0u)
    {
        if ((timeout_ms != 0u) && (elapsed_ms >= timeout_ms))
        {
            usb_serial_configured = 0u;
            return 0u;
        }

        CyDelay(1u);
        elapsed_ms++;
    }

    USBUART_CDC_Init();

    usb_serial_configured = 1u;
    return 1u;
}

void USBSerial_Update(void)
{
    uint8_t rx_buffer[64];
    uint16_t count;

    if (USBUART_GetConfiguration() != 0u)
    {
        if (!usb_serial_configured)
        {
            USBUART_CDC_Init();
            usb_serial_configured = 1u;
        }
    }
    else
    {
        usb_serial_configured = 0u;
        return;
    }

    /*
     * PSoC USBFS CDC receive handling.
     * DataIsReady() tells us the host sent data to the OUT endpoint.
     */
    if (USBUART_DataIsReady() != 0u)
    {
        count = USBUART_GetAll(rx_buffer);

        for (uint16_t i = 0u; i < count; i++)
        {
            USBSerial_ProcessReceivedByte(rx_buffer[i]);
        }
    }
}

uint8_t USBSerial_IsConnected(void)
{
    return (USBUART_GetConfiguration() != 0u) ? 1u : 0u;
}

uint8_t USBSerial_SerialAvailable(void)
{
    /*
     * This returns whether a complete line command is available,
     * not whether raw bytes are available.
     */
    return rx_line_ready ? 1u : 0u;
}

uint16_t USBSerial_Available(void)
{
    if (!USBSerial_IsConnected())
    {
        return 0u;
    }

    return USBUART_GetCount();
}

uint8_t USBSerial_ReadLine(char *buffer, uint16_t max_length)
{
    if ((buffer == NULL) || (max_length == 0u))
    {
        return 0u;
    }

    if (!rx_line_ready)
    {
        buffer[0] = '\0';
        return 0u;
    }

    strncpy(buffer, rx_line_buffer, max_length);
    buffer[max_length - 1u] = '\0';

    rx_line_ready = 0u;
    rx_line_buffer[0] = '\0';

    return 1u;
}

const char *USBSerial_ReadString(void)
{
    /*
     * Returns internal buffer pointer.
     * The next received command may eventually overwrite it, so copy it
     * if you need long-term storage.
     */
    if (!rx_line_ready)
    {
        return NULL;
    }

    rx_line_ready = 0u;
    return rx_line_buffer;
}

uint8_t USBSerial_ReadByte(uint8_t *byte)
{
    if (byte == NULL)
    {
        return 0u;
    }

    if (!USBSerial_IsConnected())
    {
        return 0u;
    }

    if (USBUART_GetCount() == 0u)
    {
        return 0u;
    }

    *byte = USBUART_GetChar();
    return 1u;
}

uint16_t USBSerial_ReadData(uint8_t *buffer, uint16_t max_length)
{
    uint16_t count;
    uint16_t i;

    if ((buffer == NULL) || (max_length == 0u))
    {
        return 0u;
    }

    if (!USBSerial_IsConnected())
    {
        return 0u;
    }

    count = USBUART_GetCount();

    if (count > max_length)
    {
        count = max_length;
    }

    for (i = 0u; i < count; i++)
    {
        buffer[i] = USBUART_GetChar();
    }

    return count;
}

void USBSerial_WriteString(const char *str)
{
    if (str == NULL)
    {
        return;
    }

    if (!USBSerial_IsConnected())
    {
        return;
    }

    USBSerial_WaitForTxReady();

    if (USBSerial_IsConnected())
    {
        USBUART_PutString(str);
    }
}

void USBSerial_WriteLine(const char *str)
{
    USBSerial_WriteString(str);
    USBSerial_WriteString("\r\n");
}

void USBSerial_WriteData(const uint8_t *data, uint16_t length)
{
    if ((data == NULL) || (length == 0u))
    {
        return;
    }

    if (!USBSerial_IsConnected())
    {
        return;
    }

    USBSerial_WaitForTxReady();

    if (USBSerial_IsConnected())
    {
        USBUART_PutData(data, length);
    }
}

void USBSerial_WriteChar(char c)
{
    if (!USBSerial_IsConnected())
    {
        return;
    }

    USBSerial_WaitForTxReady();

    if (USBSerial_IsConnected())
    {
        USBUART_PutChar((uint8_t)c);
    }
}

void USBSerial_Printf(const char *fmt, ...)
{
    char buffer[USB_SERIAL_TX_BUFFER_SIZE];
    va_list args;
    int len;

    if (fmt == NULL)
    {
        return;
    }

    va_start(args, fmt);
    len = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (len <= 0)
    {
        return;
    }

    USBSerial_WriteString(buffer);
}

void USBSerial_ClearRxBuffer(void)
{
    uint8_t dummy;

    if (USBSerial_IsConnected())
    {
        while (USBUART_GetCount() > 0u)
        {
            dummy = USBUART_GetChar();
            (void)dummy;
        }
    }

    rx_build_index = 0u;
    rx_build_buffer[0] = '\0';

    rx_line_buffer[0] = '\0';
    rx_line_ready = 0u;
}