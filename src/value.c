
#include "value.h"
#include "error.h"


const char* const _MG_UNARY_OP_NAMES[] = {
#define _MG_OP(op, name) name,
	_MG_UNARY_OP_TYPES
#undef _MG_OP
};


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
