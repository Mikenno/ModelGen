#ifndef MODELGEN_H
#define MODELGEN_H

#include "tokens.h"
#include "ast.h"

#include <stdio.h>

#define MG_MAJOR_VERSION 0
#define MG_MINOR_VERSION 1
#define MG_PATCH_VERSION 0

#define MG_VERSION "0.1.0"

#ifdef DEBUG
#	define MG_DEBUG
#endif

#define MG_FALSE 0
#define MG_TRUE 1

typedef unsigned char MGbool;

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
		char *s;
		int i;
		float f;
	} value;
} MGToken;

typedef struct MGTokenizer {
	const char *filename;
	char *string;
	MGToken *tokens;
	size_t tokenCount;
} MGTokenizer;

typedef struct MGNode MGNode;

typedef struct MGNode {
	MGNodeType type;
	MGToken *token;
	MGToken *tokenBegin;
	MGToken *tokenEnd;
	MGNode **children;
	size_t childCount;
	size_t childCapacity;
	MGNode *parent;
} MGNode;

typedef struct MGParser {
	MGTokenizer tokenizer;
	MGNode *root;
} MGParser;

char* mgReadFile(const char *filename, size_t *length);
char* mgReadFileHandle(FILE *file, size_t *length);

void mgTokenReset(const char *string, MGToken *token);
void mgTokenizeNext(MGToken *token);

void mgCreateTokenizer(MGTokenizer *tokenizer);
void mgDestroyTokenizer(MGTokenizer *tokenizer);

MGToken* mgTokenizeFile(MGTokenizer *tokenizer, const char *filename, size_t *tokenCount);
MGToken* mgTokenizeFileHandle(MGTokenizer *tokenizer, FILE *file, size_t *tokenCount);
MGToken* mgTokenizeString(MGTokenizer *tokenizer, const char *string, size_t *tokenCount);

void mgCreateParser(MGParser *parser);
void mgDestroyParser(MGParser *parser);

MGNode* mgCreateNode(MGToken *token);
void mgDestroyNode(MGNode *node);

MGNode* mgParseFile(MGParser *parser, const char *filename);
MGNode* mgParseFileHandle(MGParser *parser, FILE *file);
MGNode* mgParseString(MGParser *parser, const char *string);

#endif