#ifndef MODELGEN_H
#define MODELGEN_H

#include "tokens.h"

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
} MGToken;

char* mgReadFile(const char *filename, size_t *length);
char* mgReadFileHandle(FILE *file, size_t *length);

void mgTokenReset(const char *string, MGToken *token);
void mgTokenizeNext(MGToken *token);

MGToken* mgTokenizeFile(const char *filename, size_t *tokenCount);
MGToken* mgTokenizeFileHandle(FILE *file, size_t *tokenCount);
MGToken* mgTokenizeString(const char *string, size_t *tokenCount);

#endif