#ifndef MODELGEN_VALUE_H
#define MODELGEN_VALUE_H

#include "modelgen.h"

#define _MG_UNARY_OP_TYPES \
	_MG_OP(POSITIVE, "+") \
	_MG_OP(NEGATIVE, "-") \
	_MG_OP(INVERSE, "not")

#define _MG_LONGEST_UNARY_OP_NAME_LENGTH 3

typedef enum MGUnaryOpType {
#define _MG_OP(op, name) MG_UNARY_OP_##op,
	_MG_UNARY_OP_TYPES
#undef _MG_OP
} MGUnaryOpType;

extern const char* const _MG_UNARY_OP_NAMES[];

#define mgCreateValue(type) mgCreateValueEx(type, NULL)
MGValue* mgCreateValueEx(MGType type);
void mgDestroyValue(MGValue *value);

MGValue* mgDeepCopyValue(const MGValue *value);
MGValue* mgReferenceValue(const MGValue *value);

MGbool mgValueTruthValue(const MGValue *value);

char* mgValueToString(const MGValue *value);

MGValue* mgValueUnaryOp(const MGValue *value, MGUnaryOpType operation);
#define mgValuePositive(value) mgValueUnaryOp(value, MG_UNARY_OP_POSITIVE)
#define mgValueNegative(value) mgValueUnaryOp(value, MG_UNARY_OP_NEGATIVE)
#define mgValueInverse(value) mgValueUnaryOp(value, MG_UNARY_OP_INVERSE)

#endif