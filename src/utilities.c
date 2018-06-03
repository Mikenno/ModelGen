
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#   include <windows.h>
#else
#   include <unistd.h>
#endif

#include "utilities.h"
#include "debug.h"


inline uint32_t mgNextPowerOfTwo(uint32_t x)
{
	MG_ASSERT(x > 0);
	MG_ASSERT(x <= ((UINT32_MAX / 2) + 1));

	--x;
	x |= x >> 1u;
	x |= x >> 2u;
	x |= x >> 4u;
	x |= x >> 8u;
	x |= x >> 16u;

	return x + 1u;
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


char* mgStringDuplicateEx(const char *str, size_t length)
{
	char *s = (char*) malloc((length + 1) * sizeof(char));
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


char* mgIntToString(int i)
{
	size_t len = (size_t) snprintf(NULL, 0, "%d", i);
	char *s = (char*) malloc((len + 1) * sizeof(char));
	snprintf(s, len + 1, "%d", i);
	s[len] = '\0';
	return s;
}


char* mgFloatToString(float f)
{
	size_t len = (size_t) snprintf(NULL, 0, "%G", f);
	char *s = (char*) malloc((len + 1) * sizeof(char));
	snprintf(s, len + 1, "%f", f);
	s[len] = '\0';

	char *end = s + len - 1;
	while ((*end == '0') && (*end--) != '.');
	*(end + 1) = '\0';

	if (*end == '.')
		*end = '\0';

	return s;
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
