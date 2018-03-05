
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

	switch (c)
	{
	case '(':
		token->type = MG_TOKEN_LPAREN;
		_mgTokenNextCharacter(token);
		return;
	case ')':
		token->type = MG_TOKEN_RPAREN;
		_mgTokenNextCharacter(token);
		return;
	case '[':
		token->type = MG_TOKEN_LSQUARE;
		_mgTokenNextCharacter(token);
		return;
	case ']':
		token->type = MG_TOKEN_RSQUARE;
		_mgTokenNextCharacter(token);
		return;
	case '{':
		token->type = MG_TOKEN_LBRACE;
		_mgTokenNextCharacter(token);
		return;
	case '}':
		token->type = MG_TOKEN_RBRACE;
		_mgTokenNextCharacter(token);
		return;
	case '.':
		if (isdigit(*(token->end.string + 1)))
			goto decimal;
		token->type = MG_TOKEN_DOT;
		_mgTokenNextCharacter(token);
		return;
	case ',':
		token->type = MG_TOKEN_COMMA;
		_mgTokenNextCharacter(token);
		return;
	case ':':
		token->type = MG_TOKEN_COLON;
		_mgTokenNextCharacter(token);
		return;
	case '+':
		_mgTokenNextCharacter(token);
		switch (*token->end.string) {
		case '=':
			token->type = MG_TOKEN_ADD_ASSIGN;
			_mgTokenNextCharacter(token);
			return;
		default:
			token->type = MG_TOKEN_ADD;
			return;
		}
	case '-':
		_mgTokenNextCharacter(token);
		switch (*token->end.string) {
		case '=':
			token->type = MG_TOKEN_SUB_ASSIGN;
			_mgTokenNextCharacter(token);
			return;
		default:
			token->type = MG_TOKEN_SUB;
			return;
		}
	case '*':
		_mgTokenNextCharacter(token);
		switch (*token->end.string) {
		case '=':
			token->type = MG_TOKEN_MUL_ASSIGN;
			_mgTokenNextCharacter(token);
			return;
		default:
			token->type = MG_TOKEN_MUL;
			return;
		}
	case '/':
		_mgTokenNextCharacter(token);
		switch (*token->end.string) {
		case '=':
			token->type = MG_TOKEN_SUB_ASSIGN;
			_mgTokenNextCharacter(token);
			return;
		default:
			token->type = MG_TOKEN_SUB;
			return;
		}
	case '=':
		_mgTokenNextCharacter(token);
		switch (*token->end.string) {
		case '=':
			token->type = MG_TOKEN_EQUAL;
			_mgTokenNextCharacter(token);
			return;
		default:
			token->type = MG_TOKEN_ASSIGN;
			return;
		}
	case '%':
		_mgTokenNextCharacter(token);
		switch (*token->end.string) {
		case '=':
			token->type = MG_TOKEN_MOD_ASSIGN;
			_mgTokenNextCharacter(token);
			return;
		default:
			token->type = MG_TOKEN_MOD;
			return;
		}
	case '<':
		_mgTokenNextCharacter(token);
		switch (*token->end.string) {
		case '=':
			token->type = MG_TOKEN_LESS_EQUAL;
			_mgTokenNextCharacter(token);
			return;
		default:
			token->type = MG_TOKEN_LESS;
			return;
		}
	case '>':
		_mgTokenNextCharacter(token);
		switch (*token->end.string) {
		case '=':
			token->type = MG_TOKEN_GREATER_EQUAL;
			_mgTokenNextCharacter(token);
			return;
		default:
			token->type = MG_TOKEN_GREATER;
			return;
		}
	case '\0':
		token->type = MG_TOKEN_EOF;
		_mgTokenNextCharacter(token);
		return;
	default:
		break;
	}

	token->type = MG_TOKEN_INVALID;

	if (isalpha(c))
	{
		token->type = MG_TOKEN_IDENTIFIER;
		while (isalnum(*token->end.string) || (*token->end.string == '_'))
			_mgTokenNextCharacter(token);

		const unsigned long int len = token->end.string - token->begin.string;

		switch (len)
		{
		case 2:
			if (!strncmp("in", token->begin.string, 2))
				token->type = MG_TOKEN_IN;
			else if (!strncmp("if", token->begin.string, 2))
				token->type = MG_TOKEN_IF;
			else if (!strncmp("or", token->begin.string, 2))
				token->type = MG_TOKEN_OR;
			break;
		case 3:
			if (!strncmp("for", token->begin.string, 3))
				token->type = MG_TOKEN_FOR;
			else if (!strncmp("and", token->begin.string, 3))
				token->type = MG_TOKEN_AND;
			else if (!strncmp("not", token->begin.string, 3))
				token->type = MG_TOKEN_NOT;
			break;
		case 4:
			if (!strncmp("proc", token->begin.string, 4))
				token->type = MG_TOKEN_PROC;
			else if (!strncmp("emit", token->begin.string, 4))
				token->type = MG_TOKEN_EMIT;
			else if (!strncmp("else", token->begin.string, 4))
				token->type = MG_TOKEN_ELSE;
			break;
		default:
			break;
		}
	}
	else if (isdigit(c))
	{
		token->type = MG_TOKEN_INTEGER;
		while (isdigit(*token->end.string))
			_mgTokenNextCharacter(token);

		if (*token->end.string == '.')
		{
decimal:
			token->type = MG_TOKEN_FLOAT;
			do
				_mgTokenNextCharacter(token);
			while (isdigit(*token->end.string));
		}
	}
	else if (c == '"')
	{
		token->type = MG_TOKEN_COMMENT;
		do
		{
			_mgTokenNextCharacter(token);
			if (*token->end.string == '\\')
			{
				_mgTokenNextCharacter(token);
				if (*token->end.string == '\0')
					break;
				_mgTokenNextCharacter(token);
			}
		}
		while ((*token->end.string != '"') && (*token->end.string != '\n') && *token->end.string);

		if (*token->end.string == '"')
			_mgTokenNextCharacter(token);
		else
			token->type = MG_TOKEN_INVALID;
	}
	else if (c == '#')
	{
		token->type = MG_TOKEN_COMMENT;
		_mgTokenNextCharacter(token);

		if (*token->end.string == '[')
		{
			for (;;)
			{
				_mgTokenNextCharacter(token);
				c = *token->end.string;

				if (c == '\0')
					break;
				else if (c != '#')
					continue;

				_mgTokenNextCharacter(token);
				c = *token->end.string;

				if ((c == ']') || (c == '\0'))
					break;
			}

			if (c == ']')
				_mgTokenNextCharacter(token);
		}
		else
		{
			while ((*token->end.string != '\n') && (*token->end.string != '\0'))
				_mgTokenNextCharacter(token);
		}
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
