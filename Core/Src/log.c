#include "log.h"
#include <string.h>
#include <stdio.h>

static UART_HandleTypeDef *s_uart = NULL;

void log_init(UART_HandleTypeDef *huart)
{
    s_uart = huart;
}

void log_print(const char *s)
{
    if (!s_uart || !s) return;
    HAL_UART_Transmit(s_uart, (uint8_t*)s, (uint16_t)strlen(s), 100);
}

void log_printf(const char *fmt, ...)
{
    if (!s_uart || !fmt) return;

    char buf[128];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (n < 0) return;
    if (n > (int)sizeof(buf)) n = (int)sizeof(buf);

    HAL_UART_Transmit(s_uart, (uint8_t*)buf, (uint16_t)n, 100);
}
