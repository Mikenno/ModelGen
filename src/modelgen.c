
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "modelgen.h"
#include "debug.h"


void usage(void)
{
	printf(
		"Usage: modelgen [options] [--] [files]\n"
		"\n"
		"    -h, --help   Show this help message\n"
		"    --version    Show the version\n"
		"    - --stdin    Read stdin as input data\n"
		"\n"
		"Debugging:\n"
		"    --debug-read Print input data and exit\n"
	);
}


int main(int argc, char *argv[])
{
	int runStdin = 0;
	int debugRead = 0;

	int i = 1;
	const char *arg;

	for (; i < argc; ++i)
	{
		arg = argv[i];

		if (!strcmp("--version", arg))
		{
#ifdef MG_DEBUG
			printf("ModelGen %s (Debug Build)\n", MG_VERSION);
#else
			printf("ModelGen %s\n", MG_VERSION);
#endif
			return EXIT_SUCCESS;
		}
		else if (!strcmp("-h", arg) || !strcmp("--help", arg))
		{
			usage();
			return EXIT_SUCCESS;
		}
		else if (!strcmp("-", arg) || !strcmp("--stdin", arg))
			runStdin = 1;
		else if (!strcmp("--debug-read", arg))
			debugRead = 1;
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

	if (debugRead)
	{
		int err = 0;

		if (runStdin)
			if (!mgDebugReadHandle(stdin, "<stdin>"))
				err = 1;

		for (; i < argc; ++i)
			if (!mgDebugRead(argv[i]))
				err = 1;

		return err;
	}

	return EXIT_SUCCESS;
}
