/**
 * @file uart_qemu.c
 * @brief UART driver for QEMU virt machine (Goldfish TTY)
 *
 * This driver implements UART access for the QEMU virt machine
 * using the Goldfish TTY emulation at base address 0xff008000.
 *
 * Goldfish TTY Protocol:
 * ======================
 * 
 * Registers (all 32-bit):
 *   - 0x00 (PUT_CHAR): Write character to send
 *   - 0x04 (BYTES_READY): Read to get number of bytes available (RX_READY)
 *   - 0x08 (CMD): Write command code
 *   - 0x10 (DATA_PTR): Physical address of buffer for read/write
 *   - 0x14 (DATA_LEN): Length of buffer
 *
 * Reading data (polling):
 *   1. Poll BYTES_READY (0x04) until non-zero
 *   2. Set DATA_PTR (0x10) to buffer physical address
 *   3. Set DATA_LEN (0x14) to buffer size (usually 1 for char reading)
 *   4. Write CMD_READ_BUFFER (0x03) to CMD register (0x08)
 *   5. Buffer now contains the received character
 *
 * Writing data:
 *   - Simply write character to PUT_CHAR (0x00)
 *
 * Note: This is a polling-based driver (no interrupts).
 */

#include <stdint.h>
#include <platform.h>

/**
 * @brief Initialize UART (no initialization needed for Goldfish TTY)
 */
void v_uartInit(void)
{
    // nothing needed
}

/**
 * @brief Read a character from UART (blocking)
 * @return Character read (0-255)
 *
 * Implements Goldfish TTY read protocol:
 *   1. Poll BYTES_READY until data available
 *   2. Set up buffer address and length
 *   3. Issue CMD_READ_BUFFER command
 *   4. Return character from buffer
 */
int i_uartGetch(void)
{
    static uint8_t buf[1];

    // Step 1: Wait for data to be ready
    // BYTES_READY (0x04) returns number of bytes available
    while (*(volatile uint32_t *)(UART_BASE + 0x04) == 0) {
        // wait for bytes
    }

    // Step 2: Set up read buffer
    // DATA_PTR (0x10) = physical address of buffer
    *(volatile uint32_t *)(UART_BASE + 0x10) = (uint32_t)buf;
    // DATA_LEN (0x14) = number of bytes to read
    *(volatile uint32_t *)(UART_BASE + 0x14) = 1;

    // Step 3: Issue CMD_READ_BUFFER (0x03) command
    // This tells QEMU to copy received bytes into our buffer
    *(volatile uint32_t *)(UART_BASE + 0x08) = 0x03; // CMD_READ_BUFFER

    // Step 4: Return the character
    return buf[0];
}


/**
 * @brief Write a character to UART
 * @param ch Character to write
 *
 * Simply writes to PUT_CHAR register (0x00).
 * This is the Goldfish TTY output mechanism.
 */
 inline void v_uartPutch(unsigned int ch)
{
    *(volatile uint32_t *)(UART_BASE + 0x00) = ch;
}