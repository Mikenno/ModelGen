
#include <string.h>
#include <stdarg.h>

#include "primitive.h"
#include "composite.h"
#include "utilities.h"
#include "debug.h"


MGValue* mgCreateValueCFunction(MGCFunction cfunc)
{
	MG_ASSERT(cfunc);

	MGValue *value = mgCreateValue(MG_TYPE_CFUNCTION);
	value->data.cfunc = cfunc;
	return value;
}


MGValue* mgCreateValueBoundCFunction(MGBoundCFunction cfunc, MGValue *value)
{
	MG_ASSERT(cfunc);
	MG_ASSERT(value);

	MGValue *bound = mgCreateValue(MG_TYPE_BOUND_CFUNCTION);
	bound->data.bcfunc.cfunc = cfunc;
	bound->data.bcfunc.bound = value;
	return bound;
}


MGValue* mgCreateValueTuple(size_t capacity)
{
	MGValue *value = mgCreateValueList(capacity);
	value->type = MG_TYPE_TUPLE;
	return value;
}


MGValue* mgCreateValueTupleEx(size_t n, ...)
{
	MGValue *tuple = mgCreateValueTuple(n);
	MG_ASSERT(tuple);

	va_list args;
	va_start(args, n);

	for (size_t i = 0; i < n; ++i)
		_mgListSet(tuple->data.a, i, va_arg(args, MGValue*));

	va_end(args);

	mgListLength(tuple) = n;

	return tuple;
}


MGValue* mgCreateValueList(size_t capacity)
{
	MGValue *value = mgCreateValue(MG_TYPE_LIST);

	if (capacity > 0)
		_mgListCreate(MGValue*, value->data.a, (size_t) mgNextPowerOfTwo((uint32_t) capacity));
	else
		_mgListInitialize(value->data.a);

	return value;
}


void mgListAdd(MGValue *list, MGValue *value)
{
	MG_ASSERT(list);
	MG_ASSERT((list->type == MG_TYPE_TUPLE) || (list->type == MG_TYPE_LIST));
	MG_ASSERT(value);

	_mgListAdd(MGValue*, list->data.a, value);
}


void mgListInsert(MGValue *list, intmax_t index, MGValue *value)
{
	MG_ASSERT(list);
	MG_ASSERT((list->type == MG_TYPE_TUPLE) || (list->type == MG_TYPE_LIST));
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
	MG_ASSERT((list->type == MG_TYPE_TUPLE) || (list->type == MG_TYPE_LIST));

	index = _mgListIndexRelativeToAbsolute(list->data.a, index);
	MG_ASSERT((index >= 0) && (index < _mgListLength(list->data.a)));

	mgDestroyValue(_mgListGet(list->data.a, index));

	_mgListRemove(list->data.a, index);
}


void mgListRemoveRange(MGValue *list, intmax_t begin, intmax_t end)
{
	MG_ASSERT(list);
	MG_ASSERT((list->type == MG_TYPE_TUPLE) || (list->type == MG_TYPE_LIST));

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


void mgListClear(MGValue *list)
{
	MG_ASSERT(list);
	MG_ASSERT((list->type == MG_TYPE_TUPLE) || (list->type == MG_TYPE_LIST));

	for (size_t i = 0; i < _mgListLength(list->data.a); ++i)
		mgDestroyValue(_mgListGet(list->data.a, i));

	_mgListClear(list->data.a);
}


MGValue* mgListShallowCopy(const MGValue *list)
{
	MG_ASSERT(list);
	MG_ASSERT((list->type == MG_TYPE_TUPLE) || (list->type == MG_TYPE_LIST));

	const size_t length = mgListLength(list);
	MGValue *copy = mgCreateValueList(length);

	for (size_t i = 0; i < length; ++i)
		mgListAdd(copy, _mgListGet(list->data.a, i));

	return copy;
}


static inline void _mgDestroyMapValuePair(MGValueMapPair *pair)
{
	MG_ASSERT(pair);
	MG_ASSERT(pair->key);
	MG_ASSERT(pair->value);

	free(pair->key);
	mgDestroyValue(pair->value);
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


const MGValue* _mgMapGet(const MGValueMap *map, const char *key)
{
	MG_ASSERT(map);
	MG_ASSERT(key);

	for (size_t i = 0; i < _mgListLength(*map); ++i)
		if (strcmp(_mgListGet(*map, i).key, key) == 0)
			return _mgListGet(*map, i).value;

	return NULL;
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


MGValue* mgCreateValueMap(size_t capacity)
{
	MGValue *value = mgCreateValue(MG_TYPE_MAP);
	_mgCreateMap(&value->data.m, capacity > 0 ? (size_t) mgNextPowerOfTwo((uint32_t) capacity) : 0);
	return value;
}


void mgMapMerge(MGValue *destination, const MGValue *source, MGbool replace)
{
	MG_ASSERT(destination);
	MG_ASSERT(destination->type == MG_TYPE_MAP);
	MG_ASSERT(source);
	MG_ASSERT(source->type == MG_TYPE_MAP);

	MGMapIterator iterator;
	mgCreateMapIterator(&iterator, source);

	const MGValue *k, *v;

	if (replace)
		while (mgMapIteratorNext(&iterator, &k, &v))
			mgMapSet(destination, k->data.str.s, mgReferenceValue(v));
	else
		while (mgMapIteratorNext(&iterator, &k, &v))
			if (mgMapGet(destination, k->data.str.s) == NULL)
				mgMapSet(destination, k->data.str.s, mgReferenceValue(v));

	mgDestroyMapIterator(&iterator);
}


MGValue* mgMapShallowCopy(const MGValue *map)
{
	MG_ASSERT(map);
	MG_ASSERT(map->type == MG_TYPE_MAP);

	MGValue *copy = mgCreateValueMap(mgMapSize(map));
	MG_ASSERT(copy);

	mgMapMerge(copy, map, MG_TRUE);

	return copy;
}


void mgCreateMapIterator(MGMapIterator *iterator, const MGValue *map)
{
	MG_ASSERT(iterator);
	MG_ASSERT(map);
	MG_ASSERT(map->type == MG_TYPE_MAP);

	memset(iterator, 0, sizeof(MGMapIterator));

	iterator->map = mgReferenceValue(map);
}


void mgDestroyMapIterator(MGMapIterator *iterator)
{
	MG_ASSERT(iterator);

	mgDestroyValue(iterator->map);

	if (iterator->key)
		mgDestroyValue(iterator->key);
}


MGbool mgMapIteratorNext(MGMapIterator *iterator, const MGValue **key, const MGValue **value)
{
	MG_ASSERT(iterator);
	MG_ASSERT(iterator->map);

	if (iterator->index == mgMapSize(iterator->map))
		return MG_FALSE;

	MGValueMapPair *pair = &_mgListGet(iterator->map->data.m, iterator->index);

	if (key)
	{
		if (iterator->key)
			mgDestroyValue(iterator->key);

		iterator->key = mgCreateValueString(pair->key);

		*key = iterator->key;
	}

	if (value)
		*value = pair->value;

	++iterator->index;

	return MG_TRUE;
}
