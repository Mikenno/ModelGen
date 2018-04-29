
#include "value.h"


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
