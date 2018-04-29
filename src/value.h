#ifndef MODELGEN_VALUE_H
#define MODELGEN_VALUE_H

#include "modelgen.h"

#define mgCreateValue(type) mgCreateValueEx(type, NULL)
MGValue* mgCreateValueEx(MGType type);
void mgDestroyValue(MGValue *value);

MGValue* mgDeepCopyValue(const MGValue *value);
MGValue* mgReferenceValue(const MGValue *value);

MGbool mgValueTruthValue(const MGValue *value);

#endif