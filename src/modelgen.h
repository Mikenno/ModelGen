#ifndef MODELGEN_H
#define MODELGEN_H

#include <stddef.h>

#include "debug.h"

#include "tokens.h"
#include "ast.h"
#include "value.h"
#include "frame.h"
#include "collections.h"

#include <stdio.h>

#define MG_MAJOR_VERSION 0
#define MG_MINOR_VERSION 1
#define MG_PATCH_VERSION 0

#define MG_VERSION "0.1.0"

#define MG_FALSE 0
#define MG_TRUE 1

typedef unsigned char MGbool;

typedef struct MGInstance MGInstance;
typedef struct MGValue MGValue;

typedef MGValue* (*MGCFunction)(MGInstance *instance, size_t argc, const MGValue* const* argv);

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
	MGNode *parent;
	_MGList(MGNode*) children;
} MGNode;

typedef struct MGParser {
	MGTokenizer tokenizer;
	MGNode *root;
} MGParser;

typedef _MGList(MGValue*) MGValueList;

typedef _MGPair(char*, MGValue*) MGValueMapPair;
typedef _MGList(MGValueMapPair) MGValueMap;

typedef struct MGValue {
	MGValueType type;
	size_t refCount;
	union {
		int i;
		float f;
		struct {
			char *s;
			size_t length;
		} str;
		MGCFunction cfunc;
		MGValueList a;
		MGValueMap m;
		struct {
			MGValue *module;
			MGNode *node;
			MGValue *locals;
		} func;
		struct {
			MGInstance *instance;
			MGParser parser;
			char *filename;
			MGValue *globals;
			MGbool isStatic;
		} module;
	} data;
} MGValue;

typedef struct MGStackFrame MGStackFrame;

typedef struct MGStackFrame {
	MGStackFrameState state;
	MGStackFrame *last;
	MGStackFrame *next;
	MGValue *module;
	const MGNode *caller;
	const char *callerName;
	MGValue *value;
	MGValue *locals;
} MGStackFrame;

typedef float MGVertex[3 + 3];

typedef struct MGInstance {
	MGStackFrame *callStackTop;
	_MGList(char*) path;
	MGValue *modules;
	MGValue *staticModules;
	MGValue *base;
	_MGList(MGVertex) vertices;
	struct {
		unsigned int position : 3;
		unsigned int uv : 2;
		unsigned int normal : 3;
		unsigned int color : 3;
	} vertexSize;
} MGInstance;

#define mgInstanceGetVertexSize(instance) ((instance)->vertexSize.position + (instance)->vertexSize.uv + (instance)->vertexSize.normal + (instance)->vertexSize.color)

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

MGNode* mgCreateNode(MGToken *token, MGNodeType type);
void mgDestroyNode(MGNode *node);

MGNode* mgParse(MGParser *parser);
MGNode* mgParseFile(MGParser *parser, const char *filename);
MGNode* mgParseFileHandle(MGParser *parser, FILE *file);
MGNode* mgParseString(MGParser *parser, const char *string);

MGValue* mgInterpret(MGValue *module);
MGValue* mgInterpretFile(MGValue *module, const char *filename);
MGValue* mgInterpretFileHandle(MGValue *module, FILE *file, const char *filename);
MGValue* mgInterpretString(MGValue *module, const char *string, const char *filename);

void mgCreateInstance(MGInstance *instance);
void mgDestroyInstance(MGInstance *instance);

void mgCreateStackFrame(MGStackFrame *frame, MGValue *module);
void mgCreateStackFrameEx(MGStackFrame *frame, MGValue *module, MGValue *locals);
void mgDestroyStackFrame(MGStackFrame *frame);

void mgPushStackFrame(MGInstance *instance, MGStackFrame *frame);
void mgPopStackFrame(MGInstance *instance, MGStackFrame *frame);

void mgRunFile(MGInstance *instance, const char *filename, const char *name);
void mgRunFileHandle(MGInstance *instance, FILE *file, const char *name);
void mgRunString(MGInstance *instance, const char *string, const char *name);

MGValue* mgImportModule(MGInstance *instance, const char *name);

#endif