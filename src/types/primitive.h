#ifndef MODELGEN_PRIMITIVE_TYPES_H
#define MODELGEN_PRIMITIVE_TYPES_H

#include "value.h"

#define mgCreateValueNull() mgCreateValue(MG_TYPE_NULL)
#define mgCreateValueBoolean(b) mgCreateValueInteger(b)

MGValue* mgCreateValueInteger(int i);
#define mgIntegerSet(value, _i) value->data.i = _i
#define mgIntegerGet(value) value->data.i

MGValue* mgCreateValueFloat(float f);
#define mgFloatSet(value, _f) value->data.f = _f
#define mgFloatGet(value) value->data.f

#define mgCreateValueString(s) mgCreateValueStringEx(s, MG_STRING_USAGE_COPY)
MGValue* mgCreateValueStringEx(const char *s, MGStringUsage usage);
#define mgStringSet(value, s) mgStringSetEx(value, s, MG_STRING_USAGE_COPY)
void mgStringSetEx(MGValue *value, const char *s, MGStringUsage usage);
#define mgStringGet(value) ((const char*) (value)->data.str.s)
#define mgStringLength(value) (value)->data.str.length

#endif