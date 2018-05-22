
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "error.h"
#include "debug.h"


void mgTraceback(const MGInstance *instance)
{
	MG_ASSERT(instance);

	printf("Traceback:\n");

	const MGStackFrame *frame = instance->callStackTop;

	if (frame)
	{
		while (frame->last)
			frame = frame->last;

		size_t depth = 0;

		while (frame)
		{
			if (frame->callerName || (frame->caller && frame->caller->tokenBegin))
			{
				printf("%zu:", depth);

				if (frame->callerName)
					printf(" %s", frame->callerName);

				if (frame->caller && frame->caller->tokenBegin)
				{
					MGToken *token = frame->caller->tokenBegin ? frame->caller->tokenBegin : frame->caller->token;

					if (token)
					{
						MG_ASSERT(frame->module);
						MG_ASSERT(frame->module->type == MG_TYPE_MODULE);
						MG_ASSERT(frame->module->data.module.filename);

						if (frame->callerName)
							fputs(" at", stdout);

						printf(" %s:%u:%u",
						       frame->module->data.module.filename,
						       frame->caller->tokenBegin->begin.line,
						       frame->caller->tokenBegin->begin.character);
					}
				}

				putchar('\n');
			}

			frame = frame->next;
			++depth;
		}
	}

	fflush(stdout);
}


void _mgFatalError(const char *file, int line, const MGInstance *instance, const char *format, ...)
{
	fflush(stdout);

	if (instance)
		mgTraceback(instance);

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
