
#include "value.h"
#include "module.h"
#include "error.h"


const char* const _MG_UNARY_OP_NAMES[] = {
#define _MG_OP(op, name) name,
	_MG_UNARY_OP_TYPES
#undef _MG_OP
};


const char* const _MG_BIN_OP_NAMES[] = {
#define _MG_OP(op, name) name,
	_MG_BIN_OP_TYPES
#undef _MG_OP
};


MGValue *_mgNullValue = NULL;


MGValue* mgCreateValueEx(MGType type)
{
	MGValue *value = (MGValue*) malloc(sizeof(MGValue));
	MG_ASSERT(value);

	value->type = type;
	value->refCount = 1;

	const MGTypeData *_type = mgGetType(value->type);
	if (_type && _type->create)
		_type->create(value);

	return value;
}


void mgDestroyValue(MGValue *value)
{
	MG_ASSERT(value);

	if (--value->refCount > 0)
		return;

	const MGTypeData *type = mgGetType(value->type);
	if (type && type->destroy)
		type->destroy(value);

	free(value);
}


MGValue* mgDeepCopyValue(const MGValue *value)
{
	MG_ASSERT(value);
	MG_ASSERT(value->type != MG_TYPE_MODULE);

	MGValue *copy = (MGValue*) malloc(sizeof(MGValue));
	MG_ASSERT(copy);

	*copy = *value;
	copy->refCount = 1;

	const MGTypeData *type = mgGetType(value->type);
	if (type && type->copy)
		type->copy(copy, value);

	return copy;
}


MGValue* mgReferenceValue(const MGValue *value)
{
	MG_ASSERT(value);

	MGValue *referenced = (MGValue*) value;
	++referenced->refCount;

	return referenced;
}


MGbool mgValueTruthValue(const MGValue *value)
{
	MG_ASSERT(value);

	const MGTypeData *type = mgGetType(value->type);
	if (type && type->truth)
		return type->truth(value);

	return MG_FALSE;
}


char* mgValueToString(const MGValue *value)
{
	MG_ASSERT(value);

	const MGTypeData *type = mgGetType(value->type);
	if (type && type->str)
		return type->str(value);

	return NULL;
}


MGValue* mgValueUnaryOp(const MGValue *value, MGUnaryOpType operation)
{
	MG_ASSERT(value);

	const MGTypeData *type = mgGetType(value->type);
	MGTypeUnaryOp unary = NULL;

	switch (operation)
	{
	case MG_UNARY_OP_POSITIVE:
		unary = type->pos;
		break;
	case MG_UNARY_OP_NEGATIVE:
		unary = type->neg;
		break;
	case MG_UNARY_OP_INVERSE:
		unary = type->inv;
		break;
	default:
		break;
	}

	MGValue *result = NULL;

	if (!unary || !(result = unary(value)))
		mgFatalError("Error: Unsupported unary operator %s for type %s", _MG_UNARY_OP_NAMES[operation], mgGetTypeName(value->type));

	return result;
}


static inline MGbool _mgValueCompareUnknownAnd(const MGValue *lhs, const MGValue *rhs, const MGTypeBinOpCompare include, const MGTypeBinOpCompare exclude)
{
	MGbool result = exclude(lhs, rhs);
	if (result == -1)
		return -1;
	else if (result == MG_TRUE)
		return MG_FALSE;

	result = include(lhs, rhs);
	if (result == -1)
		return -1;
	return result;
}


static inline MGbool _mgValueCompareUnknownOr(const MGValue *lhs, const MGValue *rhs, const MGTypeBinOpCompare a, const MGTypeBinOpCompare b)
{
	MGbool result = a(lhs, rhs);
	if (result == -1)
		return -1;
	else if (result == MG_TRUE)
		return MG_TRUE;

	result = b(lhs, rhs);
	if (result == -1)
		return -1;
	return result;
}


static inline MGbool _mgValueCompareUnknown(const MGValue *lhs, const MGValue *rhs, MGBinOpType operation)
{
	const MGTypeData *lhsType = mgGetType(lhs->type);
	const MGTypeData *rhsType = mgGetType(rhs->type);

	switch (operation)
	{
	case MG_BIN_OP_EQ:
	case MG_BIN_OP_NOT_EQ:
		if (lhsType->lt && lhsType->le)
			return _mgValueCompareUnknownAnd(lhs, rhs, lhsType->le, lhsType->lt);
		else if (rhsType->lt && rhsType->le)
			return _mgValueCompareUnknownAnd(lhs, rhs, rhsType->le, rhsType->lt);
		break;
	case MG_BIN_OP_LESS:
	case MG_BIN_OP_GREATER_EQ:
		if (lhsType->eq && lhsType->le)
			return _mgValueCompareUnknownAnd(lhs, rhs, lhsType->le, lhsType->eq);
		else if (rhsType->eq && rhsType->le)
			return _mgValueCompareUnknownAnd(lhs, rhs, rhsType->le, rhsType->eq);
	case MG_BIN_OP_LESS_EQ:
	case MG_BIN_OP_GREATER:
		if (lhsType->eq && lhsType->lt)
			return _mgValueCompareUnknownOr(lhs, rhs, lhsType->lt, lhsType->eq);
		else if (rhsType->eq && rhsType->lt)
			return _mgValueCompareUnknownOr(lhs, rhs, rhsType->lt, rhsType->eq);
	default:
		break;
	}

	return -1;
}


static inline MGbool _mgValueCompare(const MGValue *lhs, const MGValue *rhs, MGBinOpType operation)
{
	const MGTypeData *lhsType = mgGetType(lhs->type);
	const MGTypeData *rhsType = mgGetType(rhs->type);

	MGbool result = -1;

	switch (operation)
	{
	case MG_BIN_OP_EQ:
	case MG_BIN_OP_NOT_EQ:
		if (lhsType->eq || rhsType->eq)
		{
			if (lhsType->eq)
				result = lhsType->eq(lhs, rhs);
			if ((result == -1) && rhsType->eq)
				result = rhsType->eq(lhs, rhs);
		}
		else
			result = _mgValueCompareUnknown(lhs, rhs, operation);
		if ((result != -1) && (operation == MG_BIN_OP_NOT_EQ))
			result = !result;
		break;
	case MG_BIN_OP_LESS:
	case MG_BIN_OP_GREATER_EQ:
		if (lhsType->eq || rhsType->eq)
		{
			if (lhsType->lt)
				result = lhsType->lt(lhs, rhs);
			if ((result == -1) && rhsType->lt)
				result = rhsType->lt(lhs, rhs);
		}
		else
			result = _mgValueCompareUnknown(lhs, rhs, operation);
		if ((result != -1) && (operation == MG_BIN_OP_GREATER_EQ))
			result = !result;
		break;
	case MG_BIN_OP_LESS_EQ:
	case MG_BIN_OP_GREATER:
		if (lhsType->le || rhsType->le)
		{
			if (lhsType->le)
				result = lhsType->le(lhs, rhs);
			if ((result == -1) && rhsType->le)
				result = rhsType->le(lhs, rhs);
		}
		else
			result = _mgValueCompareUnknown(lhs, rhs, operation);
		if ((result != -1) && (operation == MG_BIN_OP_GREATER))
			result = !result;
		break;
	default:
		break;
	}

	return result;
}


MGbool mgValueCompare(const MGValue *lhs, const MGValue *rhs, MGBinOpType operation)
{
	MGbool result = _mgValueCompare(lhs, rhs, operation);

	return (result != -1) ? result : MG_FALSE;
}


static inline MGTypeBinOp _mgTypeGetBinaryOpArithmetic(const MGTypeData *type, const MGBinOpType operation)
{
	switch (operation)
	{
	case MG_BIN_OP_ADD:
		return type->add;
	case MG_BIN_OP_SUB:
		return type->sub;
	case MG_BIN_OP_MUL:
		return type->mul;
	case MG_BIN_OP_DIV:
		return type->div;
	case MG_BIN_OP_INT_DIV:
		return type->intdiv;
	case MG_BIN_OP_MOD:
		return type->mod;
	default:
		return NULL;
	}
}


static inline MGValue* _mgValueBinaryOpArithmetic(const MGValue *lhs, const MGValue *rhs, MGBinOpType operation)
{
	const MGTypeData *lhsType = mgGetType(lhs->type);
	const MGTypeData *rhsType = mgGetType(rhs->type);

	MGValue *result = NULL;

	MGTypeBinOp binary = _mgTypeGetBinaryOpArithmetic(lhsType, operation);
	if (binary)
		result = binary(lhs, rhs);

	if (result == NULL)
	{
		binary = _mgTypeGetBinaryOpArithmetic(rhsType, operation);
		if (binary)
			result = binary(lhs, rhs);
	}

	return result;
}


MGValue* mgValueBinaryOp(const MGValue *lhs, const MGValue *rhs, MGBinOpType operation)
{
	MG_ASSERT(lhs);
	MG_ASSERT(rhs);

	MGValue *result = NULL;
	MGbool _result;

	switch (operation)
	{
	case MG_BIN_OP_ADD:
	case MG_BIN_OP_SUB:
	case MG_BIN_OP_MUL:
	case MG_BIN_OP_DIV:
	case MG_BIN_OP_INT_DIV:
	case MG_BIN_OP_MOD:
		result = _mgValueBinaryOpArithmetic(lhs, rhs, operation);
		break;
	case MG_BIN_OP_EQ:
	case MG_BIN_OP_NOT_EQ:
	case MG_BIN_OP_LESS:
	case MG_BIN_OP_LESS_EQ:
	case MG_BIN_OP_GREATER:
	case MG_BIN_OP_GREATER_EQ:
		_result = _mgValueCompare(lhs, rhs, operation);
		if (_result != -1)
			result = mgCreateValueInteger(_result);
		break;
	default:
		break;
	}

	if (result == NULL)
		mgFatalError("Error: Unsupported binary operator %s for left-hand type %s and right-hand type %s",
		             _MG_BIN_OP_NAMES[operation], mgGetTypeName(lhs->type), mgGetTypeName(rhs->type));

	return result;
}


MGValue* mgValueSubscriptGet(const MGValue *collection, const MGValue *index)
{
	MG_ASSERT(collection);
	MG_ASSERT(index);

	const MGTypeData *type = mgGetType(collection->type);
	if (type && type->subGet)
		return type->subGet(collection, index);

	return NULL;
}


MGbool mgValueSubscriptSet(const MGValue *collection, const MGValue *index, MGValue *value)
{
	MG_ASSERT(collection);
	MG_ASSERT(index);

	const MGTypeData *type = mgGetType(collection->type);
	if (type && type->subSet)
		return type->subSet(collection, index, value);

	return MG_FALSE;
}


MGValue* mgValueAttributeGet(const MGValue *collection, const char *key)
{
	MG_ASSERT(collection);
	MG_ASSERT(key);

	const MGTypeData *type = mgGetType(collection->type);
	if (type && type->attrGet)
		return type->attrGet(collection, key);

	return NULL;
}


MGbool mgValueAttributeSet(const MGValue *collection, const char *key, MGValue *value)
{
	MG_ASSERT(collection);
	MG_ASSERT(key);

	const MGTypeData *type = mgGetType(collection->type);
	if (type && type->attrSet)
		return type->attrSet(collection, key, value);

	return MG_FALSE;
}
