
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
		"    -h, --help   Print this help message and exit\n"
		"    --version    Print ModelGen version and exit\n"
		"    - --stdin    Read stdin as a file\n"
		"    --tokens     Print tokens and exit\n"
		"\n"
		"Debugging:\n"
		"    --debug-read Print file contents and exit\n"
	);
}


int main(int argc, char *argv[])
{
	int runStdin = 0;
	int debugRead = 0;
	int tokens = 0;

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
		else if (!strcmp("--tokens", arg))
			tokens = 1;
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
	else if (tokens)
	{
		if (runStdin)
			if (!mgDebugTokenizeHandle(stdin, "<stdin>"))
				err = 1;

		for (; i < argc; ++i)
			if (!mgDebugTokenize(argv[i]))
				err = 1;
	}

	return err;
}
