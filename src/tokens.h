#ifndef MODELGEN_TOKEN_H
#define MODELGEN_TOKEN_H

#define _MG_TOKENS \
	_MG_T(INVALID, "invalid") \
	_MG_T(EOF, "end-of-file") \
	_MG_T(IDENTIFIER, "identifier") \
	_MG_T(INTEGER, "integer") \
	_MG_T(FLOAT, "float") \
	_MG_T(STRING, "string") \
	_MG_T(COMMENT, "comment") \
	_MG_T(NEWLINE, "newline") \
	_MG_T(WHITESPACE, "whitespace") \
	_MG_T(PROC, "proc") \
	_MG_T(EMIT, "emit") \
	_MG_T(FOR, "for") \
	_MG_T(IN, "in") \
	_MG_T(IF, "if") \
	_MG_T(ELSE, "else") \
	_MG_T(LPAREN, "(") \
	_MG_T(RPAREN, ")") \
	_MG_T(LSQUARE, "[") \
	_MG_T(RSQUARE, "]") \
	_MG_T(LBRACE, "{") \
	_MG_T(RBRACE, "}") \
	_MG_T(DOT, ".") \
	_MG_T(COMMA, ",") \
	_MG_T(COLON, ":") \
	_MG_T(ADD, "+") \
	_MG_T(SUB, "-") \
	_MG_T(MUL, "*") \
	_MG_T(DIV, "/") \
	_MG_T(MOD, "%") \
	_MG_T(ASSIGN, "=") \
	_MG_T(ADD_ASSIGN, "+=") \
	_MG_T(SUB_ASSIGN, "-=") \
	_MG_T(MUL_ASSIGN, "*=") \
	_MG_T(DIV_ASSIGN, "/=") \
	_MG_T(MOD_ASSIGN, "%=") \
	_MG_T(EQUAL, "==") \
	_MG_T(NOT_EQUAL, "!=") \
	_MG_T(LESS, "<") \
	_MG_T(GREATER, ">") \
	_MG_T(LESS_EQUAL, "<=") \
	_MG_T(GREATER_EQUAL, ">=") \
	_MG_T(NOT, "not") \
	_MG_T(AND, "and") \
	_MG_T(OR, "or")


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