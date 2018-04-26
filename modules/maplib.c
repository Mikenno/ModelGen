
#include <stdarg.h>

#include "modelgen.h"
#include "module.h"
#include "callable.h"
#include "error.h"


extern MGValue* mg_len(MGInstance *instance, size_t argc, const MGValue* const* argv);


static MGValue* mg_map_has(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 2, 2);
	mgCheckArgumentTypes(instance, argc, argv, 1, MG_VALUE_MAP, 1, MG_VALUE_STRING);

	MGValue *map = (MGValue*) argv[0];
	const char *key = argv[1]->data.str.s;

	for (size_t i = 0; i < _mgListLength(map->data.m); ++i)
		if (strcmp(_mgListGet(map->data.m, i).key, key) == 0)
			return mgCreateValueInteger((int) MG_TRUE);

	return mgCreateValueInteger((int) MG_FALSE);
}


static MGValue* mg_map_clear(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, 1);
	mgCheckArgumentTypes(instance, argc, argv, 1, MG_VALUE_MAP);

	mgMapClear((MGValue*) argv[0]);

	return mgCreateValueNull();
}


static MGValue* mg_map_keys(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, 1);
	mgCheckArgumentTypes(instance, argc, argv, 1, MG_VALUE_MAP);

	MGValue *map = (MGValue*) argv[0];
	MGValue *keys = mgCreateValueList(mgMapSize(map));

	for (size_t i = 0; i < _mgListLength(map->data.m); ++i)
		mgListAdd(keys, mgCreateValueString(_mgListGet(map->data.m, i).key));

	return keys;
}


static MGValue* mg_map_values(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, 1);
	mgCheckArgumentTypes(instance, argc, argv, 1, MG_VALUE_MAP);

	MGValue *map = (MGValue*) argv[0];
	MGValue *values = mgCreateValueList(mgMapSize(map));

	for (size_t i = 0; i < _mgListLength(map->data.m); ++i)
		mgListAdd(values, mgReferenceValue(_mgListGet(map->data.m, i).value));

	return values;
}


static MGValue* mg_map_pairs(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, 1);
	mgCheckArgumentTypes(instance, argc, argv, 1, MG_VALUE_MAP);

	MGValue *map = (MGValue*) argv[0];
	MGValue *pairs = mgCreateValueList(mgMapSize(map));

	for (size_t i = 0; i < _mgListLength(map->data.m); ++i)
		mgListAdd(pairs, mgCreateValueTupleEx(2, mgCreateValueString(_mgListGet(map->data.m, i).key), mgReferenceValue(_mgListGet(map->data.m, i).value)));

	return pairs;
}


MGValue* mgCreateMapLib(void)
{
	MGValue *module = mgCreateValueModule();

	MG_ASSERT(module);
	MG_ASSERT(module->type == MG_VALUE_MODULE);

	mgModuleSetCFunction(module, "has", mg_map_has); // map.has(map, key): bool

	mgModuleSetCFunction(module, "clear", mg_map_clear); // map.clear(map)
	mgModuleSetCFunction(module, "size", mg_len); // map.size(map): int

	mgModuleSetCFunction(module, "keys", mg_map_keys); // map.keys(map): tuple<string>
	mgModuleSetCFunction(module, "values", mg_map_values); // map.values(map): tuple
	mgModuleSetCFunction(module, "pairs", mg_map_pairs); // map.pairs(map): tuple

	return module;
}
