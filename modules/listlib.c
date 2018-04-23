
#include <stdarg.h>

#include "modelgen.h"
#include "module.h"
#include "callable.h"


extern MGValue* mg_len(MGInstance *instance, size_t argc, const MGValue* const* argv);


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


static MGValue* mg_list_add(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	if (argc < 2)
		MG_FAIL("Error: add expected at least 2 arguments, received %zu", argc);
	else if (argv[0]->type != MG_VALUE_LIST)
		MG_FAIL("Error: add expected argument %zu as \"%s\", received \"%s\"",
		        1, _MG_VALUE_TYPE_NAMES[MG_VALUE_LIST], _MG_VALUE_TYPE_NAMES[argv[0]->type]);

	for (size_t i = 1; i < argc; ++i)
		mgListAdd((MGValue*) argv[0], mgReferenceValue(argv[i]));

	return mgCreateValueNull();
}


static MGValue* mg_list_add_from(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	if (argc != 2)
		MG_FAIL("Error: add_from expects exactly 2 argument, received %zu", argc);
	else if (argv[0]->type != MG_VALUE_LIST)
		MG_FAIL("Error: add_from expected argument %zu as \"%s\", received \"%s\"",
		        1, _MG_VALUE_TYPE_NAMES[MG_VALUE_LIST], _MG_VALUE_TYPE_NAMES[argv[0]->type]);
	else if ((argv[1]->type != MG_VALUE_TUPLE) && (argv[1]->type != MG_VALUE_LIST))
		MG_FAIL("Error: add_from expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
		        2, _MG_VALUE_TYPE_NAMES[MG_VALUE_TUPLE], _MG_VALUE_TYPE_NAMES[MG_VALUE_LIST], _MG_VALUE_TYPE_NAMES[argv[1]->type]);

	for (size_t i = 0, end = mgListLength(argv[1]); i < end; ++i)
		mgListAdd((MGValue*) argv[0], mgReferenceValue(_mgListGet(argv[1]->data.a, i)));

	return mgCreateValueNull();
}


static MGValue* mg_list_insert(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	if (argc != 3)
		MG_FAIL("Error: insert expects exactly 3 arguments, received %zu", argc);
	else if (argv[0]->type != MG_VALUE_LIST)
		MG_FAIL("Error: insert expected argument %zu as \"%s\", received \"%s\"",
		        1, _MG_VALUE_TYPE_NAMES[MG_VALUE_LIST], _MG_VALUE_TYPE_NAMES[argv[0]->type]);
	else if (argv[1]->type != MG_VALUE_INTEGER)
		MG_FAIL("Error: index expected argument %zu as \"%s\", received \"%s\"",
		        2, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[argv[0]->type]);

	intmax_t index = _mgListIndexRelativeToAbsolute(argv[0]->data.a, argv[1]->data.i);
	index = (index > 0) ? index : 0;
	index = (index > (intmax_t) mgListLength(argv[0])) ? (intmax_t) mgListLength(argv[0]) : index;

	mgListInsert((MGValue*) argv[0], index, mgReferenceValue(argv[2]));

	return mgCreateValueNull();
}


static MGValue* mg_list_clear(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	if (argc != 1)
		MG_FAIL("Error: clear expects exactly 1 argument, received %zu", argc);
	else if (argv[0]->type != MG_VALUE_LIST)
		MG_FAIL("Error: clear expected argument as \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_LIST], _MG_VALUE_TYPE_NAMES[argv[0]->type]);

	mgListClear((MGValue*) argv[0]);

	return mgCreateValueNull();
}


static MGValue* mg_list_slice(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	if (argc < 1)
		MG_FAIL("Error: slice expected at least 1 argument, received %zu", argc);
	else if (argc > 4)
		MG_FAIL("Error: slice expected at most 4 arguments, received %zu", argc);
	else if ((argv[0]->type != MG_VALUE_TUPLE) && (argv[0]->type != MG_VALUE_LIST))
		MG_FAIL("Error: slice expected argument %zu as \"%s\", received \"%s\"",
		        1, _MG_VALUE_TYPE_NAMES[MG_VALUE_LIST], _MG_VALUE_TYPE_NAMES[argv[0]->type]);

	intmax_t start = 0;
	intmax_t stop = (intmax_t) mgListLength(argv[0]);
	intmax_t step = 0;

	if (argc > 1)
	{
		if (argv[1]->type != MG_VALUE_INTEGER)
			MG_FAIL("Error: slice expected argument %zu as \"%s\", received \"%s\"",
			        2, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[argv[1]->type]);

		start = _mgListIndexRelativeToAbsolute(argv[0]->data.a, argv[1]->data.i);
		start = (start > 0) ? start : 0;
	}

	if (argc > 2)
	{
		if (argv[2]->type != MG_VALUE_INTEGER)
			MG_FAIL("Error: slice expected argument %zu as \"%s\", received \"%s\"",
			        3, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[argv[2]->type]);

		stop = _mgListIndexRelativeToAbsolute(argv[0]->data.a, argv[2]->data.i);
		stop = (stop > (intmax_t) mgListLength(argv[0])) ? (intmax_t) mgListLength(argv[0]) : stop;
	}

	if (argc > 3)
	{
		if (argv[3]->type != MG_VALUE_INTEGER)
			MG_FAIL("Error: slice expected argument %zu as \"%s\", received \"%s\"",
			        4, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[argv[3]->type]);

		step = (intmax_t) argv[3]->data.i;
	}

	intmax_t difference = stop - start;

	if (difference == 0)
		return mgCreateValueList(0);

	if (step == 0)
		step = (difference > 0) - (difference < 0);

	if ((difference ^ step) < 0)
		return mgCreateValueList(0);

	intmax_t length = difference / step + ((difference % step) != 0);

	MGValue *slice = mgCreateValueList((size_t) length);
	for (intmax_t i = start; i < stop; i += step)
		mgListAdd(slice, mgReferenceValue(_mgListGet(argv[0]->data.a, i)));

	return slice;
}


static MGValue* mg_list_reverse(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	if (argc != 1)
		MG_FAIL("Error: reverse expects exactly 1 argument, received %zu", argc);
	else if ((argv[0]->type != MG_VALUE_TUPLE) && (argv[0]->type != MG_VALUE_LIST))
		MG_FAIL("Error: reverse expected argument as \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_LIST], _MG_VALUE_TYPE_NAMES[argv[0]->type]);

	const MGValue *list = argv[0];
	const size_t length = mgListLength(list);

	for (size_t i = 0; i < length / 2; ++i)
	{
		MGValue *item1 = _mgListGet(list->data.a, i);
		MGValue *item2 = _mgListGet(list->data.a, length - i - 1);

		_mgListSet(list->data.a, i, item2);
		_mgListSet(list->data.a, length - i - 1, item1);
	}

	return mgReferenceValue(list);
}


static MGValue* mg_list_sort(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	if (argc != 2)
		MG_FAIL("Error: sort expects exactly 2 arguments, received %zu", argc);
	else if ((argv[0]->type != MG_VALUE_TUPLE) && (argv[0]->type != MG_VALUE_LIST))
		MG_FAIL("Error: sort expected argument %zu as \"%s\", received \"%s\"",
		        1, _MG_VALUE_TYPE_NAMES[MG_VALUE_LIST], _MG_VALUE_TYPE_NAMES[argv[0]->type]);
	else if (!mgIsCallable(argv[1]))
		MG_FAIL("Error: sort expected argument %zu as \"%s\", received \"%s\"",
		        2, _MG_VALUE_TYPE_NAMES[MG_VALUE_FUNCTION], _MG_VALUE_TYPE_NAMES[argv[1]->type]);

	const MGValue *list = argv[0];
	const intmax_t length = (intmax_t) mgListLength(list);

	const MGValue *comparator = argv[1];

	for (intmax_t i = length - 1; i >= 0; --i)
	{
		for (intmax_t j = 1; j <= i; ++j)
		{
			MGValue *item1 = _mgListGet(list->data.a, j - 1);
			MGValue *item2 = _mgListGet(list->data.a, j);

			const MGValue* const argv2[2] = { item1, item2 }; // Purposely not referenced

			MGValue *comparison = mgCall(instance, comparator, 2, argv2);
			MG_ASSERT(comparison);
			MG_ASSERT(comparison->type == MG_VALUE_INTEGER);

			if (comparison->data.i)
			{
				_mgListSet(list->data.a, j - 1, item2);
				_mgListSet(list->data.a, j, item1);
			}

			mgDestroyValue(comparison);
		}
	}

	return mgReferenceValue(list);
}


MGValue* mgCreateListLib(void)
{
	MGValue *module = mgCreateValueModule();

	mgModuleSetCFunction(module, "add", mg_list_add); // list.add(list, item [, item...])
	mgModuleSetCFunction(module, "add_from", mg_list_add_from); // list.add_from(list, iterable)
	mgModuleSetCFunction(module, "insert", mg_list_insert); // list.insert(list, index, item)

	mgModuleSetCFunction(module, "clear", mg_list_clear); // list.clear(list)
	mgModuleSetCFunction(module, "size", mg_len); // list.size(list): int

	mgModuleSetCFunction(module, "slice", mg_list_slice); // list.size(list, begin = 0, end = len(list), step = 0): list

	mgModuleSetCFunction(module, "reverse", mg_list_reverse); // list.reverse(list): list
	mgModuleSetCFunction(module, "sort", mg_list_sort); // list.reverse(list, comparator): list

	return module;
}
