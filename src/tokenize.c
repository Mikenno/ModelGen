
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


void mgCreateTokenizer(MGTokenizer *tokenizer)
{
	memset(tokenizer, 0, sizeof(MGTokenizer));
}


void mgDestroyTokenizer(MGTokenizer *tokenizer)
{
	free(tokenizer->string);
	free(tokenizer->tokens);
}


static inline MGToken* _mgTokenizeString(MGTokenizer *tokenizer, size_t *tokenCount)
{
	size_t capacity = 0;
	size_t count = 0;

	MGToken *tokens = NULL;

	MGToken token;
	mgTokenReset(tokenizer->string, &token);

	do
	{
		if (capacity == count)
		{
			// Given SIZE_MAX matches the architecture, then reaching an integer overflow is impossible
			// On a 32-bit system (with a SIZE_MAX accordingly) 2^31 tokens would require 50+ GB of RAM
			// Thus memory allocation would fail a long time before any possibility of overflowing
			capacity = capacity ? capacity << 1 : 2;

			// Out of memory is an unrecoverable state and will currently result in a graceless crash
			tokens = (MGToken*) realloc(tokens, capacity * sizeof(MGToken));
		}

		mgTokenizeNext(&token);
		memcpy(tokens + count, &token, sizeof(MGToken));

		++count;
	}
	while (token.type != MG_TOKEN_EOF);

	tokenizer->tokens = tokens;
	tokenizer->tokenCount = count;

	if (tokenCount)
		*tokenCount = count;

	return tokens;
}


MGToken* mgTokenizeFile(MGTokenizer *tokenizer, const char *filename, size_t *tokenCount)
{
	tokenizer->filename = filename;
	tokenizer->string = mgReadFile(filename, NULL);

	if (!tokenizer->string)
		return NULL;

	return _mgTokenizeString(tokenizer, tokenCount);
}


MGToken* mgTokenizeFileHandle(MGTokenizer *tokenizer, FILE *file, size_t *tokenCount)
{
	tokenizer->string = mgReadFileHandle(file, NULL);

	if (!tokenizer->string)
		return NULL;

	return _mgTokenizeString(tokenizer, tokenCount);
}


MGToken* mgTokenizeString(MGTokenizer *tokenizer, const char *string, size_t *tokenCount)
{
	tokenizer->string = strcpy(malloc((strlen(string) + 1) * sizeof(char)), string);

	return _mgTokenizeString(tokenizer, tokenCount);
}
