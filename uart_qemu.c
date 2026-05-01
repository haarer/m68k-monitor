#include <stdint.h>
#include <platform.h>

void v_uartInit(void)
{
    // nothing needed
}

int i_uartGetch(void)
{
    while (!(UART_STATUS & UART_STATUS_RX_READY)) {
    }
    return UART_DATA & 0xff;
}

#define UART_BASE 0xff008000

 inline void v_uartPutch(unsigned int ch)
{
    *(volatile uint32_t *)(UART_BASE + 0x00) = ch;
}