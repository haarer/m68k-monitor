#include <stdint.h>
#include <platform.h>

void v_uartInit(void)
{
    // nothing needed
}

int i_uartGetch(void)
{
    static uint8_t buf[1];

    while (*(volatile uint32_t *)(UART_BASE + 0x04) == 0) {
        // wait for bytes
    }

    *(volatile uint32_t *)(UART_BASE + 0x10) = (uint32_t)buf;
    *(volatile uint32_t *)(UART_BASE + 0x14) = 1;
    *(volatile uint32_t *)(UART_BASE + 0x08) = 0x03; // CMD_READ_BUFFER

    return buf[0];
}


 inline void v_uartPutch(unsigned int ch)
{
    *(volatile uint32_t *)(UART_BASE + 0x00) = ch;
}