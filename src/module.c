
#include <stdlib.h>
#include <string.h>

#include "modelgen.h"
#include "module.h"
#include "utilities.h"


static inline void _mgDestroyNameValue(MGNameValue *pair)
{
	MG_ASSERT(pair);
	MG_ASSERT(pair->key);
	MG_ASSERT(pair->value);

	free(pair->key);
	mgDestroyValue(pair->value);
}


void mgCreateModule(MGModule *module)
{
	MG_ASSERT(module);

	memset(module, 0, sizeof(MGModule));

	mgCreateParser(&module->parser);
}


void mgDestroyModule(MGModule *module)
{
	MG_ASSERT(module);

	for (size_t i = 0; i < _mgListLength(module->names); ++i)
		_mgDestroyNameValue(&_mgListGet(module->names, i));
	_mgListDestroy(module->names);

	mgDestroyParser(&module->parser);

	free(module->filename);
}


static inline void _mgSetValue(MGNameValue *names, size_t i, const char *name, MGValue *value)
{
	names[i].key = mgStringDuplicate(name);
	names[i].value = value;
}


static inline void _mgAddValue(MGModule *module, const char *name, MGValue *value)
{
	_mgListAddUninitialized(MGNameValue, module->names);

	_mgSetValue(_mgListItems(module->names), _mgListLength(module->names)++, name, value);
}


static inline void _mgSwapNames(MGNameValue *names, size_t i, size_t i2)
{
	MGNameValue temp;
	memcpy(&temp, &names[i], sizeof(MGNameValue));
	memcpy(&names[i], &names[i2], sizeof(MGNameValue));
	memcpy(&names[i2], &temp, sizeof(MGNameValue));
}


void mgModuleSet(MGModule *module, const char *name, MGValue *value)
{
	MG_ASSERT(module);
	MG_ASSERT(name);

	MGNameValue *pair = NULL;
	for (size_t i = 0; (pair == NULL) && (i < _mgListLength(module->names)); ++i)
		if (strcmp(_mgListGet(module->names, i).key, name) == 0)
			pair = &_mgListGet(module->names, i);

	if (value)
	{
		if (pair)
		{
			_mgDestroyNameValue(pair);
			_mgSetValue(_mgListItems(module->names), (size_t) (pair - _mgListItems(module->names)), name, value);
		}
		else
			_mgAddValue(module, name, value);
	}
	else if (pair)
	{
		_mgDestroyNameValue(pair);

		if (_mgListLength(module->names) > 1)
			if (pair != &_mgListItems(module->names)[_mgListLength(module->names) - 1])
				_mgSwapNames(_mgListItems(module->names), (size_t) (pair - _mgListItems(module->names)), _mgListLength(module->names) - 1);

		--_mgListLength(module->names);
	}
}


inline MGValue* mgModuleGet(MGModule *module, const char *name)
{
	MG_ASSERT(module);
	MG_ASSERT(name);

	for (size_t i = 0; i < _mgListLength(module->names); ++i)
		if (strcmp(_mgListGet(module->names, i).key, name) == 0)
			return _mgListGet(module->names, i).value;

	return NULL;
}


inline int mgModuleGetInteger(MGModule *module, const char *name, int defaultValue)
{
	MGValue *value = mgModuleGet(module, name);
	return (value && (value->type == MG_VALUE_INTEGER)) ? value->data.i : defaultValue;
}


inline float mgModuleGetFloat(MGModule *module, const char *name, float defaultValue)
{
	MGValue *value = mgModuleGet(module, name);
	return (value && (value->type == MG_VALUE_FLOAT)) ? value->data.f : defaultValue;
}


inline const char* mgModuleGetString(MGModule *module, const char *name, const char *defaultValue)
{
	MGValue *value = mgModuleGet(module, name);
	return (value && (value->type == MG_VALUE_STRING)) ? value->data.s : defaultValue;
}


inline MGValue* mgCreateValue(MGValueType type)
{
	MGValue *value = (MGValue*) malloc(sizeof(MGValue));
	value->type = type;
	return value;
}


void mgDestroyValue(MGValue *value)
{
	MG_ASSERT(value);

	switch (value->type)
	{
	case MG_VALUE_STRING:
		free(value->data.s);
		break;
	case MG_VALUE_TUPLE:
	case MG_VALUE_LIST:
		for (size_t i = 0; i < _mgListLength(value->data.a); ++i)
			mgDestroyValue(_mgListGet(value->data.a, i));
		_mgListDestroy(value->data.a);
		break;
	default:
		break;
	}

	free(value);
}


inline MGValue* mgCreateValueInteger(int i)
{
	MGValue *value = mgCreateValue(MG_VALUE_INTEGER);
	value->data.i = i;
	return value;
}


inline MGValue* mgCreateValueFloat(float f)
{
	MGValue *value = mgCreateValue(MG_VALUE_FLOAT);
	value->data.f = f;
	return value;
}


inline MGValue* mgCreateValueString(const char *s)
{
	MGValue *value = mgCreateValue(MG_VALUE_STRING);
	value->data.s = mgStringDuplicate(s ? s : "");
	return value;
}


inline MGValue* mgCreateValueCFunction(MGCFunction cfunc)
{
	MG_ASSERT(cfunc);

	MGValue *value = mgCreateValue(MG_VALUE_CFUNCTION);
	value->data.cfunc = cfunc;
	return value;
}


inline void mgStringSet(MGValue *value, const char *s)
{
	free(value->data.s);
	value->data.s = mgStringDuplicate(s ? s : "");
}


inline const char* mgStringGet(MGValue *value)
{
	return value->data.s;
}


inline MGValue* mgCreateValueTuple(size_t capacity)
{
	MGValue *value = mgCreateValueList(capacity);
	value->type = MG_VALUE_TUPLE;
	return value;
}


MGValue* mgCreateValueList(size_t capacity)
{
	MGValue *value = mgCreateValue(MG_VALUE_LIST);

	if (capacity > 0)
		_mgListCreate(MGValue*, value->data.a, capacity);
	else
		_mgListInitialize(value->data.a);

	return value;
}


void mgListAdd(MGValue *list, MGValue *value)
{
	MG_ASSERT(list);
	MG_ASSERT((list->type == MG_VALUE_TUPLE) || (list->type == MG_VALUE_LIST));
	MG_ASSERT(value);

	_mgListAdd(MGValue*, list->data.a, value);
}


void mgListInsert(MGValue *list, intmax_t index, MGValue *value)
{
	MG_ASSERT(list);
	MG_ASSERT((list->type == MG_VALUE_TUPLE) || (list->type == MG_VALUE_LIST));
	MG_ASSERT(value);

	index = _mgListIndexRelativeToAbsolute(list->data.a, index);
	MG_ASSERT((index >= 0) && (index <= _mgListLength(list->data.a)));

	if (_mgListLength(list->data.a) == 0)
		mgListAdd(list, value);
	else
		_mgListInsert(MGValue*, list->data.a, index, value);
}


void mgListRemove(MGValue *list, intmax_t index)
{
	MG_ASSERT(list);
	MG_ASSERT((list->type == MG_VALUE_TUPLE) || (list->type == MG_VALUE_LIST));

	index = _mgListIndexRelativeToAbsolute(list->data.a, index);
	MG_ASSERT((index >= 0) && (index < _mgListLength(list->data.a)));

	mgDestroyValue(_mgListGet(list->data.a, index));

	_mgListRemove(list->data.a, index);
}


void mgListRemoveRange(MGValue *list, intmax_t begin, intmax_t end)
{
	MG_ASSERT(list);
	MG_ASSERT((list->type == MG_VALUE_TUPLE) || (list->type == MG_VALUE_LIST));

	begin = _mgListIndexRelativeToAbsolute(list->data.a, begin);
	end = _mgListIndexRelativeToAbsolute(list->data.a, end);
	MG_ASSERT(begin <= end);
	MG_ASSERT((begin >= 0) && (end < _mgListLength(list->data.a)));

	if ((begin == 0) && (end == (_mgListLength(list->data.a) - 1)))
	{
		mgListClear(list);
		return;
	}

	for (size_t i = (size_t) begin; i <= (size_t) end; ++i)
		mgDestroyValue(_mgListGet(list->data.a, i));

	_mgListRemoveRange(list->data.a, begin, end);
}


inline void mgListClear(MGValue *list)
{
	MG_ASSERT(list);
	MG_ASSERT((list->type == MG_VALUE_TUPLE) || (list->type == MG_VALUE_LIST));

	for (size_t i = 0; i < list->data.a.length; ++i)
		mgDestroyValue(_mgListGet(list->data.a, i));

	_mgListClear(list->data.a);
}
