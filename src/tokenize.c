
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


static inline int _mgIsHexadecimal(char c)
{
	return ((c >= '0') && (c <= '9'))
	    || ((c >= 'A') && (c <= 'F'))
	    || ((c >= 'a') && (c <= 'f'));
}


static inline int _mgIsOctal(char c)
{
	return (c >= '0') && (c <= '7');
}


static inline int _mgIsBinary(char c)
{
	return (c == '0') || (c == '1');
}


void _mgParseString(MGToken *token)
{
	const size_t len = token->end.string - token->begin.string - 2;

	if (len < 1)
	{
		token->value.s = NULL;
		return;
	}

	token->value.s = (char*) malloc((len + 1) * sizeof(char));
	token->value.s[0] = '\0';

	char c, *str = token->value.s;

	for (size_t i = 0; i < len; ++i)
	{
		c = token->begin.string[i + 1];

		if (c == '\\')
		{
			c = token->begin.string[++i + 1];

			switch (c)
			{
			case 'a':
				c = '\a';
				break;
			case 'b':
				c = '\b';
				break;
			case 'f':
				c = '\f';
				break;
			case 'n':
				c = '\n';
				break;
			case 'r':
				c = '\r';
				break;
			case 't':
				c = '\t';
				break;
			case 'v':
				c = '\v';
				break;
			case '\\':
			case '"':
				break;
			default:
				c = '\\';
				--i;
				break;
			}
		}

		*str++ = c;
	}

	*str = '\0';
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
		case '/':
			_mgTokenNextCharacter(token);
			switch (*token->end.string) {
			case '=':
				token->type = MG_TOKEN_INT_DIV_ASSIGN;
				_mgTokenNextCharacter(token);
				return;
			default:
				token->type = MG_TOKEN_INT_DIV;
				return;
			}
		case '=':
			token->type = MG_TOKEN_DIV_ASSIGN;
			_mgTokenNextCharacter(token);
			return;
		default:
			token->type = MG_TOKEN_DIV;
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
	case '?':
		_mgTokenNextCharacter(token);
		switch (*token->end.string) {
		case '?':
			_mgTokenNextCharacter(token);
			switch (*token->end.string) {
			case '=':
				token->type = MG_TOKEN_COALESCE_ASSIGN;
				_mgTokenNextCharacter(token);
				return;
			default:
				token->type = MG_TOKEN_COALESCE;
				return;
			}
		default:
			token->type = MG_TOKEN_OPTIONAL;
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
	case '!':
		_mgTokenNextCharacter(token);
		switch (*token->end.string) {
		case '=':
			token->type = MG_TOKEN_NOT_EQUAL;
			_mgTokenNextCharacter(token);
			return;
		default:
			token->type = MG_TOKEN_NOT;
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
		return;
	default:
		break;
	}

	token->type = MG_TOKEN_INVALID;

	if (isalpha(c) || (c == '_'))
	{
		token->type = MG_TOKEN_NAME;
		while (isalnum(*token->end.string) || (*token->end.string == '_'))
			_mgTokenNextCharacter(token);

		const unsigned long int len = token->end.string - token->begin.string;

		switch (len)
		{
		case 2:
			if (!strncmp("if", token->begin.string, 2))
				token->type = MG_TOKEN_IF;
			else if (!strncmp("or", token->begin.string, 2))
				token->type = MG_TOKEN_OR;
			else if (!strncmp("in", token->begin.string, 2))
				token->type = MG_TOKEN_IN;
			else if (!strncmp("as", token->begin.string, 2))
				token->type = MG_TOKEN_AS;
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
			if (!strncmp("else", token->begin.string, 4))
				token->type = MG_TOKEN_ELSE;
			else if (!strncmp("proc", token->begin.string, 4))
				token->type = MG_TOKEN_PROC;
			else if (!strncmp("emit", token->begin.string, 4))
				token->type = MG_TOKEN_EMIT;
			else if (!strncmp("func", token->begin.string, 4))
				token->type = MG_TOKEN_FUNC;
			else if (!strncmp("from", token->begin.string, 4))
				token->type = MG_TOKEN_FROM;
			else if (!strncmp("null", token->begin.string, 4))
				token->type = MG_TOKEN_NULL;
			break;
		case 5:
			if (!strncmp("while", token->begin.string, 5))
				token->type = MG_TOKEN_WHILE;
			else if (!strncmp("break", token->begin.string, 5))
				token->type = MG_TOKEN_BREAK;
			break;
		case 6:
			if (!strncmp("return", token->begin.string, 6))
				token->type = MG_TOKEN_RETURN;
			else if (!strncmp("delete", token->begin.string, 6))
				token->type = MG_TOKEN_DELETE;
			else if (!strncmp("import", token->begin.string, 6))
				token->type = MG_TOKEN_IMPORT;
			else if (!strncmp("assert", token->begin.string, 6))
				token->type = MG_TOKEN_ASSERT;
			break;
		case 8:
			if (!strncmp("continue", token->begin.string, 8))
				token->type = MG_TOKEN_CONTINUE;
			break;
		default:
			break;
		}
	}
	else if (isdigit(c))
	{
		token->type = MG_TOKEN_INTEGER;

		if (c == '0')
		{
			_mgTokenNextCharacter(token);

			if ((*token->end.string == 'x') || (*token->end.string == 'X'))
			{
				_mgTokenNextCharacter(token);

				while (_mgIsHexadecimal(*token->end.string))
					_mgTokenNextCharacter(token);

				return;
			}
			else if ((*token->end.string == 'b') || (*token->end.string == 'B'))
			{
				_mgTokenNextCharacter(token);

				while (_mgIsBinary(*token->end.string))
					_mgTokenNextCharacter(token);

				return;
			}
			else if ((*token->end.string == 'o') || (*token->end.string == 'O'))
			{
				_mgTokenNextCharacter(token);

				while (_mgIsOctal(*token->end.string))
					_mgTokenNextCharacter(token);

				return;
			}
		}

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

		if ((*token->end.string == 'E') || (*token->end.string == 'e'))
		{
			_mgTokenNextCharacter(token);

			if ((*token->end.string == '+') || (*token->end.string == '-'))
				_mgTokenNextCharacter(token);

			while (isdigit(*token->end.string))
				_mgTokenNextCharacter(token);
		}
	}
	else if (c == '"')
	{
		token->type = MG_TOKEN_STRING;

		for (;;)
		{
			_mgTokenNextCharacter(token);

			if (*token->end.string == '\\')
				_mgTokenNextCharacter(token);
			else if ((*token->end.string == '"') || (*token->end.string == '\n'))
				break;

			if (*token->end.string == '\0')
				break;
		}

		if (*token->end.string == '"')
		{
			_mgTokenNextCharacter(token);
			_mgParseString(token);
		}
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
	for (size_t i = 0; i < _mgListLength(tokenizer->tokens); ++i)
		if ((_mgListGet(tokenizer->tokens, i).type == MG_TOKEN_STRING) && _mgListGet(tokenizer->tokens, i).value.s)
			free(_mgListGet(tokenizer->tokens, i).value.s);

	_mgListDestroy(tokenizer->tokens);

	free(tokenizer->string);
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

	_mgListItems(tokenizer->tokens) = tokens;
	_mgListLength(tokenizer->tokens) = count;
	_mgListCapacity(tokenizer->tokens) = count;

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
