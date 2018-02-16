
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "modelgen.h"


void usage(void)
{
	printf(
		"Usage: modelgen [options]\n"
		"\n"
		"    -h, --help   Show this help message\n"
		"    --version    Show the version\n"
	);
}


int main(int argc, char *argv[])
{
	for (int i = 0; i < argc; ++i)
	{
		if (!strcmp("--version", argv[i]))
		{
			printf("ModelGen %s\n", MG_VERSION);
			return EXIT_SUCCESS;
		}
		else if (!strcmp("-h", argv[i]) || !strcmp("--help", argv[i]))
		{
			usage();
			return EXIT_SUCCESS;
		}
		else if (argv[i][0] == '-')
		{
			printf("Unknown option: %s", argv[i]);
			return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;
}
