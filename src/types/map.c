
#include "value.h"
#include "module.h"
#include "callable.h"
#include "error.h"


// map.pop(key): any
// map.clear()
// map.has(key): bool
// map.contains(value): bool
// map.keys(): list<string>
// map.values(): list<any>
// map.pairs(): list<tuple<string, any>>


MGValue* mgMapAdd(const MGValue *lhs, const MGValue *rhs)
{
	if ((lhs->type == MG_TYPE_MAP) && (rhs->type == MG_TYPE_MAP))
	{
		MGValue *map = mgCreateValueMap(mgMapSize(lhs) + mgMapSize(rhs));

		mgMapMerge(map, lhs, MG_TRUE);
		mgMapMerge(map, rhs, MG_TRUE);

		return map;
	}

	return NULL;
}


MGValue* mgMapSubscriptGet(const MGValue *map, const MGValue *key)
{
	if (key->type != MG_TYPE_STRING)
		return NULL;

	MGValue *value = mgMapGet(map, key->data.str.s);
	return value ? mgReferenceValue(value) : MG_NULL_VALUE;
}


MGbool mgMapSubscriptSet(const MGValue *map, const MGValue *key, MGValue *value)
{
	if (key->type != MG_TYPE_STRING)
		return MG_FALSE;

	mgMapSet((MGValue*) map, key->data.str.s, value);

	return MG_TRUE;
}


static MGValue* mg_map_pop(MGInstance *instance, const MGValue *map, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, 1);
	mgCheckArgumentTypes(instance, argc, argv, 1, MG_TYPE_STRING);

	const MGValue *key = argv[0];
	MGValue *item = mgMapGet(map, key->data.str.s);

	if (item == NULL)
		return MG_NULL_VALUE;

	item = mgReferenceValue(item);
	mgMapSet((MGValue*) map, key->data.str.s, NULL);

	return item;
}


static MGValue* mg_map_clear(MGInstance *instance, const MGValue *map, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 0, 0);

	mgMapClear((MGValue*) map);

	return MG_NULL_VALUE;
}


static MGValue* mg_map_shallow_copy(MGInstance *instance, const MGValue *map, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 0, 0);

	return mgMapShallowCopy(map);
}


static MGValue* mg_map_has(MGInstance *instance, const MGValue *map, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, 1);
	mgCheckArgumentTypes(instance, argc, argv, 1, MG_TYPE_STRING);

	const char *key = argv[0]->data.str.s;

	for (size_t i = 0; i < _mgListLength(map->data.m); ++i)
		if (!strcmp(_mgListGet(map->data.m, i).key, key))
			return mgCreateValueBoolean(MG_TRUE);

	return mgCreateValueBoolean(MG_FALSE);
}


static MGValue* mg_map_contains(MGInstance *instance, const MGValue *map, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, 1);

	for (size_t i = 0; i < _mgListLength(map->data.m); ++i)
		if (mgValueCompare(argv[0], _mgListGet(map->data.m, i).value, MG_BIN_OP_EQ))
			return mgCreateValueBoolean(MG_TRUE);

	return mgCreateValueBoolean(MG_FALSE);
}


static MGValue* mg_map_keys(MGInstance *instance, const MGValue *map, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 0, 0);

	MGValue *keys = mgCreateValueList(mgMapSize(map));

	for (size_t i = 0; i < _mgListLength(map->data.m); ++i)
		mgListAdd(keys, mgCreateValueString(_mgListGet(map->data.m, i).key));

	return keys;
}


static MGValue* mg_map_values(MGInstance *instance, const MGValue *map, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 0, 0);

	MGValue *values = mgCreateValueList(mgMapSize(map));

	for (size_t i = 0; i < _mgListLength(map->data.m); ++i)
		mgListAdd(values, mgReferenceValue(_mgListGet(map->data.m, i).value));

	return values;
}


static MGValue* mg_map_pairs(MGInstance *instance, const MGValue *map, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 0, 0);

	MGValue *pairs = mgCreateValueList(mgMapSize(map));

	for (size_t i = 0; i < _mgListLength(map->data.m); ++i)
		mgListAdd(pairs, mgCreateValueTupleEx(2, mgCreateValueString(_mgListGet(map->data.m, i).key), mgReferenceValue(_mgListGet(map->data.m, i).value)));

	return pairs;
}


MGValue* mgMapAttributeGet(const MGValue *map, const char *key)
{
	if (!strcmp("size", key))
		return mgCreateValueInteger((int) mgMapSize(map));
	else if (!strcmp("pop", key))
		return mgCreateValueBoundCFunction(mg_map_pop, mgReferenceValue(map));
	else if (!strcmp("clear", key))
		return mgCreateValueBoundCFunction(mg_map_clear, mgReferenceValue(map));
	else if (!strcmp("copy", key))
		return mgCreateValueBoundCFunction(mg_map_shallow_copy, mgReferenceValue(map));
	else if (!strcmp("has", key))
		return mgCreateValueBoundCFunction(mg_map_has, mgReferenceValue(map));
	else if (!strcmp("contains", key))
		return mgCreateValueBoundCFunction(mg_map_contains, mgReferenceValue(map));
	else if (!strcmp("keys", key))
		return mgCreateValueBoundCFunction(mg_map_keys, mgReferenceValue(map));
	else if (!strcmp("values", key))
		return mgCreateValueBoundCFunction(mg_map_values, mgReferenceValue(map));
	else if (!strcmp("pairs", key))
		return mgCreateValueBoundCFunction(mg_map_pairs, mgReferenceValue(map));

	MGValue *value = mgMapGet(map, key);
	return value ? mgReferenceValue(value) : MG_NULL_VALUE;
}


MGbool mgMapAttributeSet(const MGValue *map, const char *key, MGValue *value)
{
	mgMapSet((MGValue*) map, key, value);

	return MG_TRUE;
}
