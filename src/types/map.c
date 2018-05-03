
#include "modelgen.h"
#include "value.h"
#include "module.h"
#include "callable.h"
#include "error.h"


// map.has(key): bool
// map.clear(map)
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


static MGValue* mg_map_has(MGInstance *instance, const MGValue *map, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, 1);
	mgCheckArgumentTypes(instance, argc, argv, 1, MG_TYPE_STRING);

	const char *key = argv[0]->data.str.s;

	for (size_t i = 0; i < _mgListLength(map->data.m); ++i)
		if (!strcmp(_mgListGet(map->data.m, i).key, key))
			return mgCreateValueInteger(MG_TRUE);

	return mgCreateValueInteger(MG_FALSE);
}


static MGValue* mg_map_clear(MGInstance *instance, const MGValue *map, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 0, 0);

	mgMapClear((MGValue*) map);

	return MG_NULL_VALUE;
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
	if (!strcmp("has", key))
		return mgCreateValueBoundCFunction(mg_map_has, mgReferenceValue(map));
	else if (!strcmp("clear", key))
		return mgCreateValueBoundCFunction(mg_map_clear, mgReferenceValue(map));
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
