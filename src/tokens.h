#ifndef MODELGEN_TOKEN_H
#define MODELGEN_TOKEN_H

#define _MG_TOKENS \
	_MG_T(INVALID, "invalid") \
	_MG_T(EOF, "end-of-file") \
	_MG_T(IDENTIFIER, "identifier") \
	_MG_T(INTEGER, "integer") \
	_MG_T(FLOAT, "float") \
	_MG_T(COMMENT, "comment") \
	_MG_T(NEWLINE, "newline") \
	_MG_T(WHITESPACE, "whitespace")


#define _MG_LONGEST_TOKEN_NAME_LENGTH 11


typedef enum MGTokenType {
#define _MG_T(token, name) MG_TOKEN_##token,
	_MG_TOKENS
#undef _MG_T
} MGTokenType;


static char *_MG_TOKEN_NAMES[] = {
#define _MG_T(token, name) name,
	_MG_TOKENS
#undef _MG_T
};

#endif