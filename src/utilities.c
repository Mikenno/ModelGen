
#include <stdlib.h>
#include <string.h>

#include "utilities.h"
#include "debug.h"


inline uint32_t mgNextPowerOfTwo(uint32_t x)
{
	MG_ASSERT(x > 0);
	MG_ASSERT(x <= ((UINT32_MAX / 2) + 1));

	--x;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;

	return x + 1;
}


int mgStringEndsWith(const char *string, const char *suffix)
{
	size_t stringLength = strlen(string);
	size_t suffixLength = strlen(suffix);

	return (stringLength >= suffixLength)
	    && (memcmp(suffix, string + (stringLength - suffixLength), suffixLength) == 0);
}


inline char* mgStringReplaceCharacter(char *str, char find, char replace)
{
	for (char *p = str; *p; ++p)
		if (*p == find)
			*p = replace;

	return str;
}


char* mgStringDuplicate(const char *str)
{
	char *s = (char*) malloc((strlen(str) + 1) * sizeof(char));
	strcpy(s, str);
	return s;
}


char* mgStringDuplicateFixed(const char *str, size_t count)
{
	char *s = (char*) malloc((count + 1) * sizeof(char));
	strncpy(s, str, count);
	s[count] = '\0';
	return s;
}


inline char* mgStringRepeat(char *destination, const char *source, size_t length, size_t times)
{
	for (size_t i = 0; i < times; ++i)
		strcpy(destination + length * i, source);

	return destination;
}


char* mgStringRepeatDuplicate(const char *str, size_t length, size_t times)
{
	char *s = (char*) malloc((length * times + 1) * sizeof(char));

	return mgStringRepeat(s, str, length, times);
}


const char* mgBasename(const char *filename)
{
	const char* basename1 = strrchr(filename, '/');
	const char* basename2 = strrchr(filename, '\\');

	return ((basename1 != basename2) ? ((basename1 > basename2) ? (basename1 + 1) : (basename2 + 1)) : filename);
}


size_t mgDirnameEnd(const char *filename)
{
	const char *slash = strrchr(filename, '/');
	const char *backslash = strrchr(filename, '\\');

	if (slash && backslash && (backslash > slash))
		slash = backslash;
	else if (!slash)
		slash = backslash;

	return slash ? (slash - filename) : 0;
}


char* mgDirname(char *dirname, const char *filename)
{
	const size_t end = mgDirnameEnd(filename);

	if (end)
		strncpy(dirname, filename, end);

	dirname[end] = '\0';

	return dirname;
}


static const char escapes[] = { 'a', 'b', 't', 'n', 'v', 'f', 'r' };


unsigned int mgInlineRepresentationLength(const char *str, const char *end)
{
	unsigned int len = 0;

	while (*str)
	{
		if (str == end)
			break;

		switch (*str)
		{
		case '\a':
		case '\b':
		case '\t':
		case '\n':
		case '\v':
		case '\f':
		case '\r':
		case '\\':
		case '"':
			len += 2;
			break;
		default:
			++len;
			break;
		}

		++str;
	}

	return len;
}


char* mgInlineRepresentation(char *destination, const char *source, const char *end)
{
	char *out = destination;

	while (*source)
	{
		if (source == end)
			break;

		switch (*source)
		{
		case '\a':
		case '\b':
		case '\t':
		case '\n':
		case '\v':
		case '\f':
		case '\r':
			*destination++ = '\\';
			*destination++ = escapes[*source - 7];
			break;
		case '\\':
			*destination++ = '\\';
			*destination++ = '\\';
			break;
		case '"':
			*destination++ = '\\';
			*destination++ = '"';
			break;
		default:
			*destination++ = *source;
			break;
		}

		++source;
	}

	*destination = '\0';

	return out;
}
