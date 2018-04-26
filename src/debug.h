#ifndef MODELGEN_DEBUG_H
#define MODELGEN_DEBUG_H

#ifdef DEBUG
#   ifndef MG_DEBUG
#       define MG_DEBUG 1
#   endif
#endif

#if MG_DEBUG
#include <stdlib.h>
#include <stdio.h>

#if defined(_MSC_VER)
#   define MG_DEBUG_BREAK() __debugbreak()
#elif defined(__GNUC__)
#   define MG_DEBUG_BREAK() __asm__ volatile("int $0x03")
#endif

static inline void _mgAssert(const char *expression, const char *file, int line)
{
	fflush(stdout);
	fprintf(stderr, "%s:%d: Assertion Failed: %s\n", file, line, expression);
	fflush(stderr);

	MG_DEBUG_BREAK();
	exit(1);
}

#   define MG_ASSERT(expression) ((expression) ? ((void)0) : _mgAssert(#expression, __FILE__, __LINE__))
#else
#   define MG_DEBUG_BREAK() ((void) 0)
#   define MG_ASSERT(expression) ((void) 0)
#endif

#endif