#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef PLATFORM_QEMU

#define UART_BASE 0xff008000

#define UART_REG_PUT_CHAR   (*(volatile uint32_t *)(UART_BASE + 0x00))
#define UART_REG_BYTES_READY (*(volatile uint32_t *)(UART_BASE + 0x04))
#define UART_REG_CMD        (*(volatile uint32_t *)(UART_BASE + 0x08))
#define UART_REG_VERSION    (*(volatile uint32_t *)(UART_BASE + 0x20))

#define UART_CMD_INT_DISABLE  0x00
#define UART_CMD_INT_ENABLE   0x01
#define UART_CMD_WRITE_BUFFER 0x02
#define UART_CMD_READ_BUFFER  0x03

void v_uartInit(void);
int i_uartGetch(void);
void v_uartPutch(unsigned int ch);

#else

#define UART_BASE 0x00FFFC00

#define SCDR (*(volatile uint16_t *)(UART_BASE + 0x0E))
#define SCSR (*(volatile uint16_t *)(UART_BASE + 0x0C))
#define SCCR0 (*(volatile uint16_t *)(UART_BASE + 0x08))
#define SCCR1 (*(volatile uint16_t *)(UART_BASE + 0x0A))

void v_uartInit(void);
int i_uartGetch(void);
void v_uartPutch(unsigned int ch);

#endif

#endif