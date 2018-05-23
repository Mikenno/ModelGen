#ifndef MODELGEN_PARSE_H
#define MODELGEN_PARSE_H

#include "tokenize.h"
#include "ast.h"

typedef struct MGParser {
	MGTokenizer tokenizer;
	MGNode *root;
} MGParser;

void mgCreateParser(MGParser *parser);
void mgDestroyParser(MGParser *parser);

MGNode* mgCreateNode(MGToken *token, MGNodeType type);
void mgDestroyNode(MGNode *node);

MGNode* mgParse(MGParser *parser);
MGNode* mgParseFile(MGParser *parser, const char *filename);
MGNode* mgParseFileHandle(MGParser *parser, FILE *file);
MGNode* mgParseString(MGParser *parser, const char *string);

#endif