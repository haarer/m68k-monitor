#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef PLATFORM_QEMU


#define UART_BASE 0xff008000



#define UART_DATA (*(volatile uint32_t *)(UART_BASE + 0x00))
#define UART_STATUS (*(volatile uint32_t *)(UART_BASE + 0x04))

#define UART_STATUS_RX_READY 1


#define GOLDFISH_DATA        (*(volatile uint32_t *)(UART_BASE + 0x00))
#define GOLDFISH_BYTES_READY (*(volatile uint32_t *)(UART_BASE + 0x04))

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