
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
