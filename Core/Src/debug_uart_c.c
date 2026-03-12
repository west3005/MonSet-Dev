<<<<<<< HEAD
/*
 * debug_uart_c.c
 *
 *  Created on: Mar 9, 2026
 *      Author: My
 */




=======
>>>>>>> e0288c31b99ce296d5e80e4e68e4796860bf38b1
#include "debug_uart_c.h"
#include <stdarg.h>
#include <stdio.h>
#include "main.h"

<<<<<<< HEAD
/* Forward declaration of the C++ logging back-end */
extern void DebugUart_SendString(const char *s);

=======
>>>>>>> e0288c31b99ce296d5e80e4e68e4796860bf38b1
static void _uart_log(const char *level, const char *fmt, va_list ap)
{
    char buf[256];
    int n = snprintf(buf, sizeof(buf), "[%s] ", level);
    vsnprintf(buf + n, sizeof(buf) - n, fmt, ap);
<<<<<<< HEAD
    /* Append newline */
=======
>>>>>>> e0288c31b99ce296d5e80e4e68e4796860bf38b1
    int len = 0;
    while (buf[len] && len < (int)sizeof(buf) - 2) len++;
    buf[len++] = '\r';
    buf[len++] = '\n';
    buf[len]   = '\0';
<<<<<<< HEAD
    HAL_UART_Transmit(&huart1, (uint8_t *)buf, len, 100);
=======
    HAL_UART_Transmit(&huart1, (uint8_t *)buf, (uint16_t)len, 100);
>>>>>>> e0288c31b99ce296d5e80e4e68e4796860bf38b1
}

void uart_log_info(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    _uart_log("INF", fmt, ap);
    va_end(ap);
}

void uart_log_warn(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    _uart_log("WRN", fmt, ap);
    va_end(ap);
}

void uart_log_error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    _uart_log("ERR", fmt, ap);
    va_end(ap);
}
