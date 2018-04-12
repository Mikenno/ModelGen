#ifndef MODELGEN_INSPECT_H
#define MODELGEN_INSPECT_H

#include <stdio.h>

#include "modelgen.h"

void mgInspectToken(const MGToken *token);
void mgInspectNode(const MGNode *node);
void mgInspectValue(const MGValue *value);
void mgInspectInstance(const MGInstance *instance);
void mgInspectStackFrame(const MGStackFrame *frame);

void mgInspectTokenEx(const MGToken *token, const char *filename, MGbool justify);
void mgInspectValueEx(const MGValue *value, MGbool end);

void mgInspectStringLines(const char *str);

MGbool mgDebugRead(const char *filename);
MGbool mgDebugReadHandle(FILE *file, const char *filename);

MGbool mgDebugTokenize(const char *filename);
MGbool mgDebugTokenizeHandle(FILE *file, const char *filename);

#endif