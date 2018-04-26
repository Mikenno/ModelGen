
#include <stdarg.h>

#include "modelgen.h"
#include "module.h"
#include "error.h"


extern MGValue* mg_len(MGInstance *instance, size_t argc, const MGValue* const* argv);


static MGValue* mg_map_has(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	if (argc != 2)
		mgFatalError("Error: has expects exactly 2 arguments, received %zu", argc);
	else if (argv[0]->type != MG_VALUE_MAP)
		mgFatalError("Error: has expected argument %zu as \"%s\", received \"%s\"",
		        1, _MG_VALUE_TYPE_NAMES[MG_VALUE_MAP], _MG_VALUE_TYPE_NAMES[argv[0]->type]);
	else if (argv[1]->type != MG_VALUE_STRING)
		mgFatalError("Error: has expected argument %zu as \"%s\", received \"%s\"",
		        2, _MG_VALUE_TYPE_NAMES[MG_VALUE_STRING], _MG_VALUE_TYPE_NAMES[argv[0]->type]);

	MGValue *map = (MGValue*) argv[0];
	const char *key = argv[1]->data.str.s;

	for (size_t i = 0; i < _mgListLength(map->data.m); ++i)
		if (strcmp(_mgListGet(map->data.m, i).key, key) == 0)
			return mgCreateValueInteger((int) MG_TRUE);

	return mgCreateValueInteger((int) MG_FALSE);
}


static MGValue* mg_map_clear(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	if (argc != 1)
		mgFatalError("Error: clear expects exactly 1 argument, received %zu", argc);
	else if (argv[0]->type != MG_VALUE_MAP)
		mgFatalError("Error: clear expected argument as \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_MAP], _MG_VALUE_TYPE_NAMES[argv[0]->type]);

	mgMapClear((MGValue*) argv[0]);

	return mgCreateValueNull();
}


static MGValue* mg_map_keys(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	if (argc != 1)
		mgFatalError("Error: keys expects exactly 1 argument, received %zu", argc);
	else if (argv[0]->type != MG_VALUE_MAP)
		mgFatalError("Error: keys expected argument as \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_MAP], _MG_VALUE_TYPE_NAMES[argv[0]->type]);

	MGValue *map = (MGValue*) argv[0];
	MGValue *keys = mgCreateValueList(mgMapSize(argv[0]));

	for (size_t i = 0; i < _mgListLength(map->data.m); ++i)
		mgListAdd(keys, mgCreateValueString(_mgListGet(map->data.m, i).key));

	return keys;
}


static MGValue* mg_map_values(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	if (argc != 1)
		mgFatalError("Error: values expects exactly 1 argument, received %zu", argc);
	else if (argv[0]->type != MG_VALUE_MAP)
		mgFatalError("Error: values expected argument as \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_MAP], _MG_VALUE_TYPE_NAMES[argv[0]->type]);

	MGValue *map = (MGValue*) argv[0];
	MGValue *values = mgCreateValueList(mgMapSize(argv[0]));

	for (size_t i = 0; i < _mgListLength(map->data.m); ++i)
		mgListAdd(values, mgReferenceValue(_mgListGet(map->data.m, i).value));

	return values;
}


static MGValue* mg_map_pairs(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	if (argc != 1)
		mgFatalError("Error: pairs expects exactly 1 argument, received %zu", argc);
	else if (argv[0]->type != MG_VALUE_MAP)
		mgFatalError("Error: pairs expected argument as \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_MAP], _MG_VALUE_TYPE_NAMES[argv[0]->type]);

	MGValue *map = (MGValue*) argv[0];
	MGValue *pairs = mgCreateValueList(mgMapSize(argv[0]));

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
