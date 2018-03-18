#ifndef MODELGEN_H
#define MODELGEN_H

#include "tokens.h"
#include "ast.h"
#include "value.h"

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
		int i;
		float f;
		char *s;
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

typedef struct MGValue MGValue;

typedef MGValue* (*MGCFunction)(size_t argc, MGValue **argv);

typedef struct MGValue {
	MGValueType type;
	union {
		int i;
		float f;
		char *s;
		MGCFunction cfunc;
		struct {
			MGValue **items;
			size_t length;
			size_t capacity;
		} a;
	} data;
} MGValue;

typedef struct MGName {
	char *name;
	MGValue *value;
} MGName;

typedef struct MGModule {
	const char *filename;
	MGName *names;
	size_t length;
	size_t capacity;
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

void mgCreateModule(MGModule *module);
void mgDestroyModule(MGModule *module);

MGValue* mgCreateValue(MGValueType type);
void mgDestroyValue(MGValue *value);

void mgSetValue(MGModule *module, const char *name, MGValue *value);
MGValue* mgGetValue(MGModule *module, const char *name);

void mgSetValueInteger(MGModule *module, const char *name, int i);
void mgSetValueFloat(MGModule *module, const char *name, float f);
void mgSetValueString(MGModule *module, const char *name, const char *s);
void mgSetValueCFunction(MGModule *module, const char *name, MGCFunction cfunc);

int mgGetValueInteger(MGModule *module, const char *name, int defaultValue);
float mgGetValueFloat(MGModule *module, const char *name, float defaultValue);
const char* mgGetValueString(MGModule *module, const char *name, const char *defaultValue);

MGValue* mgRunFile(MGModule *module, const char *filename);
MGValue* mgRunFileHandle(MGModule *module, FILE *file);
MGValue* mgRunString(MGModule *module, const char *string);

#endif