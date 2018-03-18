#ifndef MODELGEN_DEBUG_H
#define MODELGEN_DEBUG_H

#include <stdio.h>

#include "modelgen.h"

void mgInspectToken(MGToken *token, const char *filename, MGbool justify);
void mgInspectNode(MGNode *node);

MGbool mgDebugRead(const char *filename);
MGbool mgDebugReadHandle(FILE *file, const char *filename);

MGbool mgDebugTokenize(const char *filename);
MGbool mgDebugTokenizeHandle(FILE *file, const char *filename);

#endif