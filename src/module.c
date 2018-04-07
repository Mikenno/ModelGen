
#include <stdlib.h>
#include <string.h>

#include "modelgen.h"
#include "module.h"
#include "utilities.h"


MGModule* mgCreateModule(void)
{
	MGModule *module = (MGModule*) calloc(1, sizeof(MGModule));
	MG_ASSERT(module);

	mgCreateParser(&module->parser);
	module->globals = mgCreateValueMap(1 << 4);

	return module;
}


void mgDestroyModule(MGModule *module)
{
	MG_ASSERT(module);
	MG_ASSERT(module->globals);

	// If refCount > 1 that implies the module is still in use
	// However modules are only destroyed when destroying the
	// instance, so this currently isn't a problem
	// MG_ASSERT(module->globals->refCount == 1);

	mgDestroyParser(&module->parser);
	mgDestroyValue(module->globals);
	free(module->filename);

	free(module);
}


void mgModuleSet(MGModule *module, const char *name, MGValue *value)
{
	MG_ASSERT(module);
	MG_ASSERT(name);

	_mgMapSet(&module->globals->data.m, name, value);
}


inline MGValue* mgModuleGet(MGModule *module, const char *name)
{
	MG_ASSERT(module);
	MG_ASSERT(name);

	return _mgMapGet(&module->globals->data.m, name);
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


static inline void _mgDestroyMapValuePair(MGValueMapPair *pair)
{
	MG_ASSERT(pair);
	MG_ASSERT(pair->key);
	MG_ASSERT(pair->value);

	free(pair->key);
	mgDestroyValue(pair->value);
}


void _mgCreateMap(MGValueMap *map, size_t capacity)
{
	MG_ASSERT(map);

	if (capacity > 0)
		_mgListCreate(MGValueMapPair, *map, capacity);
	else
		_mgListInitialize(*map);
}


void _mgDestroyMap(MGValueMap *map)
{
	MG_ASSERT(map);

	_mgMapClear(map);
	_mgListDestroy(*map);
}


void _mgMapClear(MGValueMap *map)
{
	MG_ASSERT(map);

	for (size_t i = 0; i < _mgListLength(*map); ++i)
		_mgDestroyMapValuePair(&_mgListGet(*map, i));

	_mgListLength(*map) = 0;
}


static inline void _mgMapSetSet(MGValueMapPair *names, size_t i, const char *key, MGValue *value)
{
	MG_ASSERT(names);
	MG_ASSERT(key);
	MG_ASSERT(value);

	names[i].key = mgStringDuplicate(key);
	names[i].value = value;
}


static inline void _mgMapSetAdd(MGValueMap *map, const char *key, MGValue *value)
{
	_mgListAddUninitialized(MGValueMapPair, *map);

	_mgMapSetSet(_mgListItems(*map), _mgListLength(*map)++, key, value);
}


static inline void _mgMapSetSwap(MGValueMapPair *pairs, size_t i, size_t i2)
{
	MGValueMapPair temp;
	memcpy(&temp, &pairs[i], sizeof(MGValueMapPair));
	memcpy(&pairs[i], &pairs[i2], sizeof(MGValueMapPair));
	memcpy(&pairs[i2], &temp, sizeof(MGValueMapPair));
}


void _mgMapSet(MGValueMap *map, const char *key, MGValue *value)
{
	MG_ASSERT(map);
	MG_ASSERT(key);

	MGValueMapPair *pair = NULL;
	for (size_t i = 0; (pair == NULL) && (i < _mgListLength(*map)); ++i)
		if (strcmp(_mgListGet(*map, i).key, key) == 0)
			pair = &_mgListGet(*map, i);

	if (value)
	{
		if (pair)
		{
			_mgDestroyMapValuePair(pair);
			_mgMapSetSet(_mgListItems(*map), (size_t) (pair - _mgListItems(*map)), key, value);
		}
		else
			_mgMapSetAdd(map, key, value);
	}
	else if (pair)
	{
		_mgDestroyMapValuePair(pair);

		if (_mgListLength(*map) > 1)
			if (pair != &_mgListItems(*map)[_mgListLength(*map) - 1])
				_mgMapSetSwap(_mgListItems(*map), (size_t) (pair - _mgListItems(*map)), _mgListLength(*map) - 1);

		--_mgListLength(*map);
	}
}


MGValue* _mgMapGet(const MGValueMap *map, const char *key)
{
	MG_ASSERT(map);
	MG_ASSERT(key);

	for (size_t i = 0; i < _mgListLength(*map); ++i)
		if (strcmp(_mgListGet(*map, i).key, key) == 0)
			return _mgListGet(*map, i).value;

	return NULL;
}


inline MGValue* mgCreateValue(MGValueType type)
{
	MGValue *value = (MGValue*) malloc(sizeof(MGValue));
	MG_ASSERT(value);

	value->type = type;
	value->refCount = 1;

	return value;
}


void mgDestroyValue(MGValue *value)
{
	MG_ASSERT(value);

	if (--value->refCount > 0)
		return;

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
	case MG_VALUE_MAP:
		_mgDestroyMap(&value->data.m);
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


inline MGValue* mgCreateValueMap(size_t capacity)
{
	MGValue *value = mgCreateValue(MG_VALUE_MAP);
	_mgCreateMap(&value->data.m, capacity > 0 ? (size_t) mgNextPowerOfTwo((uint32_t) capacity) : 0);
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
		_mgListCreate(MGValue*, value->data.a, (size_t) mgNextPowerOfTwo((uint32_t) capacity));
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

	for (size_t i = 0; i < _mgListLength(list->data.a); ++i)
		mgDestroyValue(_mgListGet(list->data.a, i));

	_mgListClear(list->data.a);
}
