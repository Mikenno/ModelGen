#ifndef MODELGEN_TOKEN_H
#define MODELGEN_TOKEN_H

#define _MG_TOKENS \
	_MG_T(INVALID, "invalid") \
	_MG_T(EOF, "end-of-file") \
	_MG_T(NAME, "name") \
	_MG_T(NULL, "null") \
	_MG_T(INTEGER, "integer") \
	_MG_T(FLOAT, "float") \
	_MG_T(STRING, "string") \
	_MG_T(COMMENT, "comment") \
	_MG_T(NEWLINE, "newline") \
	_MG_T(WHITESPACE, "whitespace") \
	_MG_T(FOR, "for") \
	_MG_T(WHILE, "while") \
	_MG_T(BREAK, "break") \
	_MG_T(CONTINUE, "continue") \
	_MG_T(IF, "if") \
	_MG_T(ELSE, "else") \
	_MG_T(PROC, "proc") \
	_MG_T(EMIT, "emit") \
	_MG_T(FUNC, "func") \
	_MG_T(RETURN, "return") \
	_MG_T(DELETE, "delete") \
	_MG_T(IMPORT, "import") \
	_MG_T(FROM, "from") \
	_MG_T(AS, "as") \
	_MG_T(IN, "in") \
	_MG_T(ASSERT, "assert") \
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
	_MG_T(INT_DIV, "//") \
	_MG_T(MOD, "%") \
	_MG_T(EQUAL, "==") \
	_MG_T(NOT_EQUAL, "!=") \
	_MG_T(LESS, "<") \
	_MG_T(GREATER, ">") \
	_MG_T(LESS_EQUAL, "<=") \
	_MG_T(GREATER_EQUAL, ">=") \
	_MG_T(NOT, "not") \
	_MG_T(AND, "and") \
	_MG_T(OR, "or") \
	_MG_T(QUESTION, "?") \
	_MG_T(ELVIS, "?:") \
	_MG_T(COALESCE, "\?\?") \
	_MG_T(ARROW, "->") \
	_MG_T(ASSIGN, "=") \
	_MG_T(ADD_ASSIGN, "+=") \
	_MG_T(SUB_ASSIGN, "-=") \
	_MG_T(MUL_ASSIGN, "*=") \
	_MG_T(DIV_ASSIGN, "/=") \
	_MG_T(INT_DIV_ASSIGN, "//=") \
	_MG_T(MOD_ASSIGN, "%=") \

#define _MG_LONGEST_TOKEN_NAME_LENGTH 11

extern const char* const _MG_TOKEN_NAMES[];

typedef enum MGTokenType {
#define _MG_T(token, name) MG_TOKEN_##token,
	_MG_TOKENS
#undef _MG_T
} MGTokenType;

typedef struct MGToken {
	MGTokenType type;
	struct {
		const char *string;
		unsigned int line;
		unsigned int character;
	} begin;
	struct {
		const char *string;
		unsigned int line;
		unsigned int character;
	} end;
	union {
		int i;
		float f;
		char *s;
	} value;
} MGToken;

#endif