
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "debug.h"
#include "utilities.h"


#define _MG_INT_COUNT_DIGITS(x) ((int) floorf(log10f((float) abs(x))) + 1)

#define _MG_FILENAME_PADDING 4


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


void _mgDebugTokenPrint(const char *filename, MGToken *token)
{
	const unsigned int len = token->end.string - token->begin.string;

	char *string2 = NULL;

	if (len)
	{
		string2 = (char*) malloc((mgInlineRepresentationLength(token->begin.string, token->end.string) + 1) * sizeof(char));
		mgInlineRepresentation(string2, token->begin.string, token->end.string);
	}

	int padding = _MG_INT_COUNT_DIGITS(token->begin.line) + _MG_INT_COUNT_DIGITS(token->begin.character);
	padding = (padding > _MG_FILENAME_PADDING) ? 0 : (_MG_FILENAME_PADDING + 1 - padding);

	printf("%s:%u:%u:%*s %*-s \"%s\"\n",
	       filename,
	       token->begin.line, token->begin.character,
	       padding, "",
	       _MG_LONGEST_TOKEN_NAME_LENGTH, _MG_TOKEN_NAMES[token->type],
	       string2 ? string2 : "");

	if (string2)
		free(string2);
}


void _mgDebugTokenizePrint(const char *filename, char *str, size_t len)
{
	if (str)
	{
		printf("Tokenizing: %s\n", filename);

		MGToken token;
		mgTokenReset(str, &token);

		filename = mgBasename(filename);

		do
		{
			mgTokenizeNext(&token);
			_mgDebugTokenPrint(filename, &token);
		}
		// while (token.type > MG_TOKEN_EOF);
		while (token.type != MG_TOKEN_EOF);
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


MGbool mgDebugTokenize(const char *filename)
{
	size_t len;
	char *str = mgReadFile(filename, &len);

	_mgDebugTokenizePrint(filename, str, len);

	free(str);

	return str ? MG_TRUE : MG_FALSE;
}


MGbool mgDebugTokenizeHandle(FILE *file, const char *filename)
{
	size_t len;
	char *str = mgReadFileHandle(file, &len);

	_mgDebugTokenizePrint(filename, str, len);

	free(str);

	return str ? MG_TRUE : MG_FALSE;
}
