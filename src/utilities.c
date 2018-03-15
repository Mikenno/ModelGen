
#include <string.h>

#include "utilities.h"


const char* mgBasename(const char *filename)
{
	const char* basename1 = strrchr(filename, '/');
	const char* basename2 = strrchr(filename, '\\');

	return ((basename1 != basename2) ? ((basename1 > basename2) ? (basename1 + 1) : (basename2 + 1)) : filename);
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
