#ifndef MODELGEN_INTERPRET_H
#define MODELGEN_INTERPRET_H

#include "value.h"
#include "instance.h"
#include "frame.h"

MGValue* mgInterpret(MGValue *module);
MGValue* mgInterpretFile(MGValue *module, const char *filename);
MGValue* mgInterpretFileHandle(MGValue *module, FILE *file, const char *filename);
MGValue* mgInterpretString(MGValue *module, const char *string, const char *filename);

void mgRunFile(MGInstance *instance, const char *filename, const char *name);
void mgRunFileHandle(MGInstance *instance, FILE *file, const char *name);
void mgRunString(MGInstance *instance, const char *string, const char *name);

MGValue* mgImportModule(MGInstance *instance, const char *name);

#endif