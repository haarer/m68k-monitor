/**
 * @file platform.h
 * @brief Hardware platform abstraction for m68k-monitor
 *
 * This header defines the UART base address and register access macros
 * for different target platforms:
 *   - PLATFORM_QEMU:  QEMU virt machine with Goldfish TTY
 *   - PLATFORM_REALHW: MC68331 real hardware with QSM module
 */

#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef PLATFORM_QEMU

/**
 * QEMU virt machine configuration:
 *   - CPU: m68020
 *   - UART: Goldfish TTY at 0xff008000
 *   - RAM: 64MB starting at 0x00000000
 *
 * Goldfish TTY Register Map (all 32-bit):
 *   - 0x00 (PUT_CHAR): Write to send character
 *   - 0x04 (BYTES_READY): Read to get number of bytes available
 *   - 0x08 (CMD): Write command code:
 *         CMD_READ_BUFFER = 3: Copy received bytes to buffer
 *         CMD_WRITE_BUFFER = 2: Send buffer contents
 *   - 0x10 (DATA_PTR): Physical address of buffer
 *   - 0x14 (DATA_LEN): Length of buffer
 */
#define UART_BASE 0xff008000

/* Goldfish TTY registers (matches QEMU implementation) */
#define PUT_CHAR        (*(volatile uint32_t *)(UART_BASE + 0x00))  /**< Write: send char, Read: - */
#define BYTES_READY     (*(volatile uint32_t *)(UART_BASE + 0x04))  /**< Read: bytes available */
#define CMD             (*(volatile uint32_t *)(UART_BASE + 0x08))  /**< Write: command code */
#define DATA_PTR        (*(volatile uint32_t *)(UART_BASE + 0x10))  /**< Write: buffer physical addr */
#define DATA_LEN        (*(volatile uint32_t *)(UART_BASE + 0x14))  /**< Write: buffer length */

/* Command codes */
#define CMD_WRITE_BUFFER 2  /**< Write buffer contents to TTY */
#define CMD_READ_BUFFER  3  /**< Read TTY input into buffer */

/* Backward compatibility aliases */
#define UART_DATA       PUT_CHAR
#define UART_STATUS     BYTES_READY
#define UART_STATUS_RX_READY 1  /**< Alias for BYTES_READY check */

void v_uartInit(void);
int i_uartGetch(void);
void v_uartPutch(unsigned int ch);

#else /* PLATFORM_REALHW */

/**
 * MC68331 real hardware configuration:
 *   - CPU: CPU32 (MC68331)
 *   - UART: QSM module at 0xFFFC00
 *   - RAM: 192KB external SRAM at 0x100000
 */
#define UART_BASE 0x00FFFC00

/* QSM UART registers */
#define SCDR  (*(volatile uint16_t *)(UART_BASE + 0x0E))  /**< Serial Data Register */
#define SCSR  (*(volatile uint16_t *)(UART_BASE + 0x0C))  /**< Serial Status Register */
#define SCCR0 (*(volatile uint16_t *)(UART_BASE + 0x08))  /**< Serial Control Reg 0 */
#define SCCR1 (*(volatile uint16_t *)(UART_BASE + 0x0A))  /**< Serial Control Reg 1 */

void v_uartInit(void);
int i_uartGetch(void);
void v_uartPutch(unsigned int ch);

#endif /* PLATFORM_REALHW */

#endif /* PLATFORM_H */