#ifndef MODELGEN_MODULE_TYPE_H
#define MODELGEN_MODULE_TYPE_H

#include "value.h"
#include "primitive.h"

MGValue* mgCreateValueModule(void);

void mgModuleSet(MGValue *module, const char *name, MGValue *value);
const MGValue* mgModuleGet(const MGValue *module, const char *name);

#define mgModuleSetInteger(module, name, i) mgModuleSet(module, name, mgCreateValueInteger(i))
#define mgModuleSetFloat(module, name, f) mgModuleSet(module, name, mgCreateValueFloat(f))
#define mgModuleSetString(module, name, s) mgModuleSet(module, name, mgCreateValueString(s))
#define mgModuleSetCFunction(module, name, cfunc) mgModuleSet(module, name, mgCreateValueCFunction(cfunc))

int mgModuleGetInteger(MGValue *module, const char *name, int defaultValue);
float mgModuleGetFloat(MGValue *module, const char *name, float defaultValue);
const char* mgModuleGetString(MGValue *module, const char *name, const char *defaultValue);

#endif