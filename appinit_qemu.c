#include <stdint.h>
#include <platform.h>

void v_uartInit(void);

void init_main(void)
{
    volatile uint32_t *dbg = (volatile uint32_t *)0x40000FF0;
    *dbg = 0xDEAD0001;
    v_uartInit();
    *dbg = 0xDEAD0002;
}