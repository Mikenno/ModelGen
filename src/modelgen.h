#ifndef MODELGEN_H
#define MODELGEN_H

#include <stddef.h>

#include "debug.h"

#include "tokens.h"
#include "ast.h"
#include "value.h"
#include "collections.h"

#include <stdio.h>

#define MG_MAJOR_VERSION 0
#define MG_MINOR_VERSION 1
#define MG_PATCH_VERSION 0

#define MG_VERSION "0.1.0"

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
		int i;
		float f;
		char *s;
	} value;
} MGToken;

typedef struct MGTokenizer {
	const char *filename;
	char *string;
	_MGList(MGToken) tokens;
} MGTokenizer;

typedef struct MGNode MGNode;

typedef struct MGNode {
	MGNodeType type;
	MGToken *token;
	MGToken *tokenBegin;
	MGToken *tokenEnd;
	_MGList(MGNode*) children;
	MGNode *parent;
} MGNode;

typedef struct MGParser {
	MGTokenizer tokenizer;
	MGNode *root;
} MGParser;

typedef struct MGValue MGValue;

typedef MGValue* (*MGCFunction)(size_t argc, MGValue **argv);

typedef struct MGValue {
	MGValueType type;
	union {
		int i;
		float f;
		char *s;
		MGCFunction cfunc;
		_MGList(MGValue*) a;
		MGNode *func;
	} data;
} MGValue;

typedef struct MGName {
	char *name;
	MGValue *value;
} MGName;

typedef struct MGModule {
	char *filename;
	_MGList(MGName) names;
} MGModule;

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

MGValue* mgRunFile(MGModule *module, const char *filename);
MGValue* mgRunFileHandle(MGModule *module, FILE *file, const char *filename);
MGValue* mgRunString(MGModule *module, const char *string, const char *filename);

#endif