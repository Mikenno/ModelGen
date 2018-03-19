
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "modelgen.h"
#include "inspect.h"

#ifdef _WIN32
#   include <stdint.h>
#   include <windows.h>
#endif


static inline void _mgFail(const char *format, ...)
{
	fflush(stdout);

	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);

	putc('\n', stderr);
	fflush(stderr);

	exit(1);
}

#define MG_FAIL(...) _mgFail(__VA_ARGS__)


static MGValue* _mg_print(size_t argc, MGValue **argv)
{
	for (size_t i = 0; i < argc; ++i)
	{
		if (i > 0)
			putchar(' ');

		MGValue *value = argv[i];

		if (value->type != MG_VALUE_STRING)
			_mgInspectValue(argv[i]);
		else if (value->data.s)
			fputs(value->data.s, stdout);
	}

	putchar('\n');

	MGValue *value = mgCreateValue(MG_VALUE_INTEGER);
	value->data.i = 0;

	return value;
}


static MGValue* _mg_range(size_t argc, MGValue **argv)
{
	if (argc != 1)
		MG_FAIL("Error: range expected 1 argument, received %zu", argc);

	MGValue *_stop = argv[0];

	if (_stop->type != MG_VALUE_INTEGER)
		MG_FAIL("Error: range expected argument 1 as \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[_stop->type]);

	int stop = _stop->data.i;
	stop = (stop > 0) ? stop : 0;

	MGValue *value = mgCreateValue(MG_VALUE_TUPLE);
	value->data.a.items = (stop > 0) ? ((MGValue**) malloc(stop * sizeof(MGValue*))) : NULL;
	value->data.a.length = (size_t) stop;
	value->data.a.capacity = (size_t) stop;

	for (int i = 0; i < stop; ++i)
	{
		MGValue *item = mgCreateValue(MG_VALUE_INTEGER);
		item->data.i = i;

		value->data.a.items[i] = item;
	}

	return value;
}


void usage(void)
{
	printf(
		"Usage: modelgen [options] [--] [files]\n"
		"\n"
		"    -h, --help   Print this help message and exit\n"
		"    --version    Print ModelGen version and exit\n"
		"    - --stdin    Read stdin as a file\n"
		"    --tokens     Print tokens and exit\n"
		"    --ast        Print ast and exit\n"
		"\n"
		"Introspection:\n"
		"\n"
		"    --profile     Print elapsed time\n"
		"    --dump-module Print module contents on exit\n"
		"\n"
		"Debugging:\n"
		"\n"
		"    --debug-read  Print file contents and exit\n"
	);
}


int main(int argc, char *argv[])
{
	MGbool runStdin = MG_FALSE;
	MGbool debugRead = MG_FALSE;
	MGbool debugTokens = MG_FALSE;
	MGbool debugAST = MG_FALSE;
	MGbool profileTime = MG_FALSE;
	MGbool dumpModule = MG_FALSE;

	int i = 1;
	const char *arg;

	for (; i < argc; ++i)
	{
		arg = argv[i];

		if (!strcmp("--version", arg))
		{
			fputs("ModelGen " MG_VERSION, stdout);

#ifdef MG_DEBUG
			fputs(" (Debug Build)", stdout);
#endif

#if defined(__i386__)
			fputs(" [32-bit]", stdout);
#elif defined(__x86_64__)
			fputs(" [64-bit]", stdout);
#endif

			putc('\n', stdout);

			return EXIT_SUCCESS;
		}
		else if (!strcmp("-h", arg) || !strcmp("--help", arg))
		{
			usage();
			return EXIT_SUCCESS;
		}
		else if (!strcmp("-", arg) || !strcmp("--stdin", arg))
			runStdin = MG_TRUE;
		else if (!strcmp("--tokens", arg))
			debugTokens = MG_TRUE;
		else if (!strcmp("--ast", arg))
			debugAST = MG_TRUE;
		else if (!strcmp("--debug-read", arg))
			debugRead = MG_TRUE;
		else if (!strcmp("--profile", arg))
			profileTime = MG_TRUE;
		else if (!strcmp("--dump-module", arg))
			dumpModule = MG_TRUE;
		else if (!strcmp("--", arg))
		{
			++i;
			break;
		}
		else if (arg[0] == '-')
		{
			fprintf(stderr, "Unknown option: %s", arg);
			return EXIT_FAILURE;
		}
		else
			break;
	}

	int err = EXIT_SUCCESS;

#ifdef _WIN32
	LARGE_INTEGER timeStart, timeStop;
	LARGE_INTEGER timerResolution;

	if (profileTime)
		QueryPerformanceCounter(&timeStart);
#endif

	if (debugRead)
	{
		if (runStdin)
			if (!mgDebugReadHandle(stdin, "<stdin>"))
				err = 1;

		for (; i < argc; ++i)
			if (!mgDebugRead(argv[i]))
				err = 1;
	}
	else if (debugTokens)
	{
		if (runStdin)
			if (!mgDebugTokenizeHandle(stdin, "<stdin>"))
				err = 1;

		for (; i < argc; ++i)
			if (!mgDebugTokenize(argv[i]))
				err = 1;
	}
	else if (debugAST)
	{
		MGParser parser;
		MGNode *root;

		if (runStdin)
		{
			mgCreateParser(&parser);

			if ((root = mgParseFileHandle(&parser, stdin)))
				mgInspectNode(root);
			else
				err = 1;

			mgDestroyParser(&parser);
		}

		for (; i < argc; ++i)
		{
			const char *filename = argv[i];

			mgCreateParser(&parser);

			if ((root = mgParseFile(&parser, filename)))
				mgInspectNode(root);
			else
				err = 1;

			mgDestroyParser(&parser);
		}
	}
	else
	{
		MGModule module;
		MGValue *result;

		if (runStdin)
		{
			mgCreateModule(&module);
			module.filename = "<stdin>";

			mgSetValueInteger(&module, "false", 0);
			mgSetValueInteger(&module, "true", 1);
			mgSetValueCFunction(&module, "print", _mg_print);
			mgSetValueCFunction(&module, "range", _mg_range);

			if ((result = mgRunFileHandle(&module, stdin)))
				mgDestroyValue(result);
			else
				err = 1;

			if (dumpModule)
			{
				putchar('\n');
				mgInspectModule(&module);
			}

			mgDestroyModule(&module);
		}

		for (; i < argc; ++i)
		{
			const char *filename = argv[i];

			mgCreateModule(&module);
			module.filename = filename;

			mgSetValueInteger(&module, "false", 0);
			mgSetValueInteger(&module, "true", 1);
			mgSetValueCFunction(&module, "print", _mg_print);
			mgSetValueCFunction(&module, "range", _mg_range);

			if ((result = mgRunFile(&module, filename)))
				mgDestroyValue(result);
			else
				err = 1;

			if (dumpModule)
			{
				putchar('\n');
				mgInspectModule(&module);
			}

			mgDestroyModule(&module);
		}
	}

#ifdef _WIN32
	if (profileTime)
	{
		QueryPerformanceCounter(&timeStop);
		QueryPerformanceFrequency(&timerResolution);

		const int64_t timeInterval = timeStop.QuadPart - timeStart.QuadPart;
		const double timeSeconds = (double) timeInterval / (double) timerResolution.QuadPart;

#if MG_ANSI_COLORS
		fputs("\e[90m", stdout);
#endif

		putchar('\n');
		printf("Time Elapsed: %.6fms\n", timeSeconds * 1000.0);

#if MG_ANSI_COLORS
		fputs("\e[0m", stdout);
#endif
	}
#endif

	return err;
}
