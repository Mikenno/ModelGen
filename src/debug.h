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

static inline void _mgAssert(const char *expression, const char *file, int line)
{
	fprintf(stderr, "%s:%d: Assertion Failed: %s\n", file, line, expression);
	exit(1);
}

#   define MG_ASSERT(expression) ((expression) ? ((void)0) : _mgAssert(#expression, __FILE__, __LINE__))
#else
#   define MG_ASSERT(expression) ((void)0)
#endif

#endif