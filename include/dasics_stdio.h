#ifndef INCLUDE_MY_STDIO_H_
#define INCLUDE_MY_STDIO_H_

#include "dasics_stdarg.h"

int dasics_printf(const char *fmt, ...);
int dasics_vprintf(const char *fmt, va_list va);

#endif
