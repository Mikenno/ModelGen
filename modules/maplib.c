
#include <stdarg.h>

#include "modelgen.h"
#include "module.h"


extern MGValue* mg_len(MGInstance *instance, size_t argc, MGValue **argv);


static inline void _mgFail(const char *format, ...)
{
	fflush(stdout);

	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);

	putc('\n', stderr);
	fflush(stderr);

	exit(1);
}

#define MG_FAIL(...) _mgFail(__VA_ARGS__)


static MGValue* mg_map_has(MGInstance *instance, size_t argc, MGValue **argv)
{
	if (argc != 2)
		MG_FAIL("Error: has expects exactly 2 arguments, received %zu", argc);
	else if (argv[0]->type != MG_VALUE_MAP)
		MG_FAIL("Error: has expected argument %zu as \"%s\", received \"%s\"",
		        1, _MG_VALUE_TYPE_NAMES[MG_VALUE_MAP], _MG_VALUE_TYPE_NAMES[argv[0]->type]);
	else if (argv[1]->type != MG_VALUE_STRING)
		MG_FAIL("Error: has expected argument %zu as \"%s\", received \"%s\"",
		        2, _MG_VALUE_TYPE_NAMES[MG_VALUE_STRING], _MG_VALUE_TYPE_NAMES[argv[0]->type]);

	MGValue *map = argv[0];
	const char *key = argv[1]->data.str.s;

	for (size_t i = 0; i < _mgListLength(map->data.m); ++i)
		if (strcmp(_mgListGet(map->data.m, i).key, key) == 0)
			return mgCreateValueInteger((int) MG_TRUE);

	return mgCreateValueInteger((int) MG_FALSE);
}


static MGValue* mg_map_clear(MGInstance *instance, size_t argc, MGValue **argv)
{
	if (argc != 1)
		MG_FAIL("Error: clear expects exactly 1 argument, received %zu", argc);
	else if (argv[0]->type != MG_VALUE_MAP)
		MG_FAIL("Error: clear expected argument as \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_MAP], _MG_VALUE_TYPE_NAMES[argv[0]->type]);

	mgMapClear(argv[0]);

	return mgCreateValueNull();
}


static MGValue* mg_map_keys(MGInstance *instance, size_t argc, MGValue **argv)
{
	if (argc != 1)
		MG_FAIL("Error: keys expects exactly 1 argument, received %zu", argc);
	else if (argv[0]->type != MG_VALUE_MAP)
		MG_FAIL("Error: keys expected argument as \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_MAP], _MG_VALUE_TYPE_NAMES[argv[0]->type]);

	MGValue *map = argv[0];
	MGValue *keys = mgCreateValueTuple(mgMapSize(argv[0]));

	for (size_t i = 0; i < _mgListLength(map->data.m); ++i)
		mgTupleAdd(keys, mgCreateValueString(_mgListGet(map->data.m, i).key));

	return keys;
}


static MGValue* mg_map_values(MGInstance *instance, size_t argc, MGValue **argv)
{
	if (argc != 1)
		MG_FAIL("Error: values expects exactly 1 argument, received %zu", argc);
	else if (argv[0]->type != MG_VALUE_MAP)
		MG_FAIL("Error: values expected argument as \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_MAP], _MG_VALUE_TYPE_NAMES[argv[0]->type]);

	MGValue *map = argv[0];
	MGValue *values = mgCreateValueTuple(mgMapSize(argv[0]));

	for (size_t i = 0; i < _mgListLength(map->data.m); ++i)
		mgTupleAdd(values, mgReferenceValue(_mgListGet(map->data.m, i).value));

	return values;
}


static MGValue* mg_map_pairs(MGInstance *instance, size_t argc, MGValue **argv)
{
	if (argc != 1)
		MG_FAIL("Error: pairs expects exactly 1 argument, received %zu", argc);
	else if (argv[0]->type != MG_VALUE_MAP)
		MG_FAIL("Error: pairs expected argument as \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_MAP], _MG_VALUE_TYPE_NAMES[argv[0]->type]);

	MGValue *map = argv[0];
	MGValue *pairs = mgCreateValueTuple(mgMapSize(argv[0]));

	for (size_t i = 0; i < _mgListLength(map->data.m); ++i)
	{
		MGValue *pair = mgCreateValueTuple(2);
		mgTupleAdd(pair, mgCreateValueString(_mgListGet(map->data.m, i).key));
		mgTupleAdd(pair, mgReferenceValue(_mgListGet(map->data.m, i).value));

		mgTupleAdd(pairs, pair);
	}

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
