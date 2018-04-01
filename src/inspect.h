#ifndef MODELGEN_INSPECT_H
#define MODELGEN_INSPECT_H

#include <stdio.h>

#include "modelgen.h"

void mgInspectToken(const MGToken *token, const char *filename, MGbool justify);
void mgInspectNode(const MGNode *node);
void _mgInspectValue(const MGValue *value);
void mgInspectValue(const MGValue *value);
void mgInspectModule(const MGModule *module);
void mgInspectInstance(const MGInstance *instance);
void mgInspectStackFrame(const MGStackFrame *frame);

void mgInspectStringLines(const char *str);

MGbool mgDebugRead(const char *filename);
MGbool mgDebugReadHandle(FILE *file, const char *filename);

MGbool mgDebugTokenize(const char *filename);
MGbool mgDebugTokenizeHandle(FILE *file, const char *filename);

#endif