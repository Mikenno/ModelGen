
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"


void _mgDebugReadPrint(const char *filename, char *str, size_t len)
{
	if (str)
	{
		printf("Reading: %s\n", filename);

		int lineCount = 0;

		if (len)
		{
			++lineCount;

			for (char *c = str; *c; ++c)
				if (*c == '\n')
					++lineCount;
		}

		printf("Length: %zu\n", len);
		printf("Lines: %d\n", lineCount);

		if (lineCount > 0)
		{
			char *currentLine = str;
			int line = 1;

			while (currentLine)
			{
				char *nextLine = strchr(currentLine, '\n');

				if (nextLine)
					*nextLine = '\0';

				printf("%d: %s\n", line, currentLine);

				if (nextLine)
					*nextLine = '\n';

				currentLine = nextLine ? (nextLine + 1) : NULL;
				++line;
			}
		}
	}
	else
	{
		fprintf(stderr, "Failed Reading: %s\n", filename);
	}
}


MGbool mgDebugRead(const char *filename)
{
	size_t len;
	char *str = mgReadFile(filename, &len);

	_mgDebugReadPrint(filename, str, len);

	free(str);

	return str ? MG_TRUE : MG_FALSE;
}


MGbool mgDebugReadHandle(FILE *file, const char *filename)
{
	size_t len;
	char *str = mgReadFileHandle(file, &len);

	_mgDebugReadPrint(filename, str, len);

	free(str);

	return str ? MG_TRUE : MG_FALSE;
}
