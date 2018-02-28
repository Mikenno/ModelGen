
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "modelgen.h"


#define _MG_STREAM_BUFFER_LENGTH 1024


char* mgReadFile(const char *filename, size_t *length)
{
	FILE *f = fopen(filename, "r");

	if (!f)
		return NULL;

	char *str = mgReadFileHandle(f, length);

	fclose(f);

	return str;
}


char* mgReadFileHandle(FILE *file, size_t *length)
{
	fseek(file, 0, SEEK_END);
	const long int len = ftell(file);
	fseek(file, 0, SEEK_SET);

	char *str = NULL;

	if (len >= 0)
	{
		str = (char*) malloc((len + 1) * sizeof(char));

		if (!str)
			return NULL;

		size_t read = 0;
		if (len > 0)
			read = fread(str, sizeof(char), len, file);

		str[read] = '\0';

		if (length)
			*length = read;
	}
	else if (len < 0)
	{
		char buffer[_MG_STREAM_BUFFER_LENGTH];

		size_t strLen = 0;
		str = (char*) malloc(sizeof(char));

		if (!str)
			return NULL;

		char *strTemp;

		// TODO: To avoid a lot of reallocating, allocate a bigger buffer and realloc it smaller after

		while (!feof(file) && !ferror(file))
		{
			size_t read = fread(buffer, sizeof(char), _MG_STREAM_BUFFER_LENGTH, file);

			strLen += read;
			strTemp = (char*) realloc(str, (strLen + 1) * sizeof(char));

			if (!strTemp)
			{
				free(str);
				return NULL;
			}

			str = strTemp;

			strncpy(str + strLen - read, buffer, read);
		}

		str[strLen] = '\0';

		if (length)
			*length = strLen;
	}

	return str;
}
