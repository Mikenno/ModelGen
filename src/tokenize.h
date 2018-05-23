#ifndef MODELGEN_TOKENIZE_H
#define MODELGEN_TOKENIZE_H

#include <stddef.h>
#include <stdio.h>

#include "tokens.h"
#include "collections.h"

typedef struct MGTokenizer {
	char *filename;
	char *string;
	_MGList(MGToken) tokens;
} MGTokenizer;

void mgTokenReset(const char *string, MGToken *token);
void mgTokenizeNext(MGToken *token);

void mgCreateTokenizer(MGTokenizer *tokenizer);
void mgDestroyTokenizer(MGTokenizer *tokenizer);

MGToken* mgTokenizeFile(MGTokenizer *tokenizer, const char *filename, size_t *tokenCount);
MGToken* mgTokenizeFileHandle(MGTokenizer *tokenizer, FILE *file, size_t *tokenCount);
MGToken* mgTokenizeString(MGTokenizer *tokenizer, const char *string, size_t *tokenCount);

#endif