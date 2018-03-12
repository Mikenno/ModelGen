
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "modelgen.h"
#include "debug.h"

#ifdef _WIN32
#   include <stdint.h>
#   include <windows.h>
#endif


void usage(void)
{
	printf(
		"Usage: modelgen [options] [--] [files]\n"
		"\n"
		"    -h, --help   Print this help message and exit\n"
		"    --version    Print ModelGen version and exit\n"
		"    - --stdin    Read stdin as a file\n"
		"    --tokens     Print tokens and exit\n"
		"\n"
		"Debugging:\n"
		"\n"
		"    --debug-read Print file contents and exit\n"
		"    --profile    Print elapsed time\n"
	);
}


int main(int argc, char *argv[])
{
	MGbool runStdin = MG_FALSE;
	MGbool debugRead = MG_FALSE;
	MGbool debugTokens = MG_FALSE;
	MGbool profileTime = MG_FALSE;

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
		else if (!strcmp("--debug-read", arg))
			debugRead = MG_TRUE;
		else if (!strcmp("--profile", arg))
			profileTime = MG_TRUE;
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
	else
	{
#ifdef _WIN32
		LARGE_INTEGER timeStart, timeStop;
		LARGE_INTEGER timerResolution;

		if (profileTime)
			QueryPerformanceCounter(&timeStart);
#endif

		MGParser parser;
		MGNode *root;

		if (runStdin)
		{
			mgCreateParser(&parser);

			if ((root = mgParseFileHandle(&parser, stdin)))
				mgDebugInspectNode(root);
			else
				err = 1;

			mgDestroyParser(&parser);
		}

		for (; i < argc; ++i)
		{
			const char *filename = argv[i];

			mgCreateParser(&parser);

			if ((root = mgParseFile(&parser, filename)))
				mgDebugInspectNode(root);
			else
				err = 1;

			mgDestroyParser(&parser);
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
	}

	return err;
}
