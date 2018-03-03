
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "modelgen.h"


#define _mg_sizeof_field(type, field) (sizeof(((type*)0)->field))


void mgTokenReset(const char *string, MGToken *token)
{
	memset(token, 0, sizeof(MGToken));

	// C99: If the first enumerator has no =, the value of its enumeration constant is 0.
	// token->type = MG_TOKEN_INVALID;

	token->end.line = 1;
	token->end.character = 1;

	token->begin.string = string;
	token->end.string = string;
}


static inline void _mgTokenNextCharacter(MGToken *token)
{
	if (*token->end.string == '\n')
	{
		++token->end.line;
		token->end.character = 1;
	}
	else
		++token->end.character;

	++token->end.string;
}


void mgTokenizeNext(MGToken *token)
{
	// TODO: Assert _mg_sizeof_field(MGToken, begin) == _mg_sizeof_field(MGToken, end)
	memcpy(&token->begin, &token->end, _mg_sizeof_field(MGToken, begin));

	char c = *token->end.string;
	if (c == '\0')
	{
		token->type = MG_TOKEN_EOF;
		return;
	}

	token->type = MG_TOKEN_INVALID;

	if (isalpha(c))
	{
		token->type = MG_TOKEN_IDENTIFIER;
		while (isalnum(*token->end.string))
			_mgTokenNextCharacter(token);
	}
	else if (isdigit(c))
	{
		token->type = MG_TOKEN_INTEGER;
		while (isdigit(*token->end.string))
			_mgTokenNextCharacter(token);

		if (*token->end.string == '.')
		{
			token->type = MG_TOKEN_FLOAT;
			do
				_mgTokenNextCharacter(token);
			while (isdigit(*token->end.string));
		}
	}
	else if (c == '#')
	{
		token->type = MG_TOKEN_COMMENT;
		while ((*token->end.string != '\n') && (*token->end.string != '\0'))
			_mgTokenNextCharacter(token);
	}
	else if (isspace(c))
	{
		token->type = (c == '\n') ? MG_TOKEN_NEWLINE : MG_TOKEN_WHITESPACE;
		_mgTokenNextCharacter(token);
	}
	else
		_mgTokenNextCharacter(token);
}


MGToken* mgTokenizeFile(const char *filename, size_t *tokenCount)
{
	char *str = mgReadFile(filename, NULL);

	if (!str)
		return NULL;

	MGToken *tokens = mgTokenizeString(str, tokenCount);

	free(str);

	return tokens;
}


MGToken* mgTokenizeFileHandle(FILE *file, size_t *tokenCount)
{
	char *str = mgReadFileHandle(file, NULL);

	if (!str)
		return NULL;

	MGToken *tokens = mgTokenizeString(str, tokenCount);

	free(str);

	return tokens;
}


MGToken* mgTokenizeString(const char *string, size_t *tokenCount)
{
	size_t capacity = 0;
	size_t count = 0;

	MGToken *tokens = NULL;

	MGToken token;
	mgTokenReset(string, &token);

	do
	{
		if (capacity == count)
		{
			// 4294967295 tokens is unlikely to be reached, but it should be noted that an integer overflow isn't handled currently
			capacity = capacity ? capacity << 1 : 2;

			// realloc returning NULL makes it impossible to free tokens
			// Out of memory is not handled, as it is a royally screwed situation to be in
			tokens = (MGToken*) realloc(tokens, capacity * sizeof(MGToken));
		}

		mgTokenizeNext(&token);
		memcpy(tokens + count, &token, sizeof(MGToken));

		++count;
	}
	while (token.type != MG_TOKEN_EOF);

	if (tokenCount)
		*tokenCount = count;

	return  tokens;
}
