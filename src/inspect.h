#ifndef MODELGEN_DEBUG_H
#define MODELGEN_DEBUG_H

#include <stdio.h>

#include "modelgen.h"

void mgInspectToken(const MGToken *token, const char *filename, MGbool justify);
void mgInspectNode(const MGNode *node);
void _mgInspectValue(const MGValue *value);
void mgInspectValue(const MGValue *value);
void mgInspectModule(const MGModule *module);

MGbool mgDebugRead(const char *filename);
MGbool mgDebugReadHandle(FILE *file, const char *filename);

MGbool mgDebugTokenize(const char *filename);
MGbool mgDebugTokenizeHandle(FILE *file, const char *filename);

#endif