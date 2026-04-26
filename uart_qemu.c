#include <stdint.h>
#include <platform.h>

void v_uartInit(void)
{
    UART_REG_CMD = UART_CMD_INT_DISABLE;
}

int i_uartGetch(void)
{
    while (UART_REG_BYTES_READY == 0) {
    }
    return UART_REG_PUT_CHAR & 0xff;
}

void v_uartPutch(unsigned int ch)
{
    while (UART_REG_BYTES_READY != 0) {
    }
    UART_REG_PUT_CHAR = ch;
}