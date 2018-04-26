
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "error.h"
#include "debug.h"


void _mgFatalError(const char *file, int line, const char *format, ...)
{
	fflush(stdout);

	fprintf(stderr, "%s:%d: ", file, line);

	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);

	putc('\n', stderr);
	fflush(stderr);

#ifdef MG_DEBUG
	MG_DEBUG_BREAK();
#endif

	exit(1);
}
