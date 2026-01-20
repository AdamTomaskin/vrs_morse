#ifndef LOG_H
#define LOG_H

#include "main.h"
#include <stdarg.h>

void log_init(UART_HandleTypeDef *huart);
void log_print(const char *s);
void log_printf(const char *fmt, ...);

#endif
