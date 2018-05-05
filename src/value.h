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

#define _MG_BIN_OP_TYPES \
	_MG_OP(ADD, "+") \
	_MG_OP(SUB, "-") \
	_MG_OP(MUL, "*") \
	_MG_OP(DIV, "/") \
	_MG_OP(INT_DIV, "//") \
	_MG_OP(MOD, "%") \
	_MG_OP(EQ, "==") \
	_MG_OP(NOT_EQ, "!=") \
	_MG_OP(LESS, "<") \
	_MG_OP(LESS_EQ, "<=") \
	_MG_OP(GREATER, ">") \
	_MG_OP(GREATER_EQ, ">=")

#define _MG_LONGEST_BIN_OP_NAME_LENGTH 2

typedef enum MGBinOpType {
#define _MG_OP(op, name) MG_BIN_OP_##op,
	_MG_BIN_OP_TYPES
#undef _MG_OP
} MGBinOpType;

extern const char* const _MG_BIN_OP_NAMES[];

extern MGValue *_mgNullValue;
#define _MG_NULL_VALUE _mgNullValue
#define MG_NULL_VALUE mgReferenceValue(_MG_NULL_VALUE)

MGValue* mgCreateValue(MGType type);
void mgDestroyValue(MGValue *value);

MGValue* mgCopyValue(const MGValue *value, MGbool shallow);
#define mgDeepCopyValue(value) mgCopyValue(value, MG_FALSE)
#define mgShallowCopyValue(value) mgCopyValue(value, MG_TRUE)

MGValue* mgReferenceValue(const MGValue *value);

MGValue *mgValueConvert(const MGValue *value, MGType type);

MGbool mgValueTruthValue(const MGValue *value);

char* mgValueToString(const MGValue *value);

MGValue* mgValueUnaryOp(const MGValue *value, MGUnaryOpType operation);
#define mgValuePositive(value) mgValueUnaryOp(value, MG_UNARY_OP_POSITIVE)
#define mgValueNegative(value) mgValueUnaryOp(value, MG_UNARY_OP_NEGATIVE)
#define mgValueInverse(value) mgValueUnaryOp(value, MG_UNARY_OP_INVERSE)

MGbool mgValueCompare(const MGValue *lhs, const MGValue *rhs, MGBinOpType operation);

MGValue* mgValueBinaryOp(const MGValue *lhs, const MGValue *rhs, MGBinOpType operation);
#define mgValueAdd(lhs, rhs) mgValueBinaryOp(lhs, rhs, MG_BIN_OP_ADD)
#define mgValueSub(lhs, rhs) mgValueBinaryOp(lhs, rhs, MG_BIN_OP_SUB)
#define mgValueMul(lhs, rhs) mgValueBinaryOp(lhs, rhs, MG_BIN_OP_MUL)
#define mgValueDiv(lhs, rhs) mgValueBinaryOp(lhs, rhs, MG_BIN_OP_DIV)
#define mgValueIntDiv(lhs, rhs) mgValueBinaryOp(lhs, rhs, MG_BIN_OP_INT_DIV)
#define mgValueMod(lhs, rhs) mgValueBinaryOp(lhs, rhs, MG_BIN_OP_MOD)
#define mgValueEqual(lhs, rhs) mgValueBinaryOp(lhs, rhs, MG_BIN_OP_EQ)
#define mgValueNotEqual(lhs, rhs) mgValueBinaryOp(lhs, rhs, MG_BIN_OP_NOT_EQ)
#define mgValueLess(lhs, rhs) mgValueBinaryOp(lhs, rhs, MG_BIN_OP_LESS)
#define mgValueLessEqual(lhs, rhs) mgValueBinaryOp(lhs, rhs, MG_BIN_OP_LESS_EQ)
#define mgValueGreater(lhs, rhs) mgValueBinaryOp(lhs, rhs, MG_BIN_OP_GREATER)
#define mgValueGreaterEqual(lhs, rhs) mgValueBinaryOp(lhs, rhs, MG_BIN_OP_GREATER_EQ)

MGValue* mgValueSubscriptGet(const MGValue *collection, const MGValue *index);
MGbool mgValueSubscriptSet(const MGValue *collection, const MGValue *index, MGValue *value);

MGValue* mgValueAttributeGet(const MGValue *collection, const char *key);
MGbool mgValueAttributeSet(const MGValue *collection, const char *key, MGValue *value);

#endif