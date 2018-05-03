
#include <stdarg.h>

#include "modelgen.h"
#include "value.h"
#include "module.h"
#include "callable.h"
#include "error.h"


extern MGValue* mg_len(MGInstance *instance, size_t argc, const MGValue* const* argv);


static MGValue* mg_list_add(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 2, 8);
	mgCheckArgumentTypes(instance, argc, argv, 1, MG_TYPE_LIST, 0, 0, 0, 0, 0, 0, 0);

	if (argc < 2)
		mgFatalError("Error: add expected at least 2 arguments, received %zu", argc);
	else if (argv[0]->type != MG_TYPE_LIST)
		mgFatalError("Error: add expected argument %zu as \"%s\", received \"%s\"",
		        1, mgGetTypeName(MG_TYPE_LIST), mgGetTypeName(argv[0]->type));

	for (size_t i = 1; i < argc; ++i)
		mgListAdd((MGValue*) argv[0], mgReferenceValue(argv[i]));

	return MG_NULL_VALUE;
}


static MGValue* mg_list_add_from(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 2, 2);
	mgCheckArgumentTypes(instance, argc, argv, 1, MG_TYPE_LIST, 2, MG_TYPE_TUPLE, MG_TYPE_LIST);

	for (size_t i = 0, end = mgListLength(argv[1]); i < end; ++i)
		mgListAdd((MGValue*) argv[0], mgReferenceValue(_mgListGet(argv[1]->data.a, i)));

	return MG_NULL_VALUE;
}


static MGValue* mg_list_insert(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 3, 3);
	mgCheckArgumentTypes(instance, argc, argv, 1, MG_TYPE_LIST, 1, MG_TYPE_INTEGER, 0);

	intmax_t index = _mgListIndexRelativeToAbsolute(argv[0]->data.a, argv[1]->data.i);
	index = (index > 0) ? index : 0;
	index = (index > (intmax_t) mgListLength(argv[0])) ? (intmax_t) mgListLength(argv[0]) : index;

	mgListInsert((MGValue*) argv[0], index, mgReferenceValue(argv[2]));

	return MG_NULL_VALUE;
}


static MGValue* mg_list_clear(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, 1);
	mgCheckArgumentTypes(instance, argc, argv, 1, MG_TYPE_LIST);

	if (argc != 1)
		mgFatalError("Error: clear expects exactly 1 argument, received %zu", argc);
	else if (argv[0]->type != MG_TYPE_LIST)
		mgFatalError("Error: clear expected argument as \"%s\", received \"%s\"",
		        mgGetTypeName(MG_TYPE_LIST), mgGetTypeName(argv[0]->type));

	mgListClear((MGValue*) argv[0]);

	return MG_NULL_VALUE;
}


static MGValue* mg_list_slice(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, 4);
	mgCheckArgumentTypes(instance, argc, argv, 2, MG_TYPE_TUPLE, MG_TYPE_LIST, 1, MG_TYPE_INTEGER, 1, MG_TYPE_INTEGER, 1, MG_TYPE_INTEGER);

	const MGValue *list = argv[0];
	const intmax_t length = (intmax_t) mgListLength(list);

	intmax_t start = 0;
	intmax_t stop = length;
	intmax_t step = (argc > 3) ? (intmax_t) argv[3]->data.i : 0;

	if (argc > 1)
	{
		start = _mgListIndexRelativeToAbsolute(argv[0]->data.a, argv[1]->data.i);
		start = (start > 0) ? start : 0;
	}

	if (argc > 2)
	{
		stop = _mgListIndexRelativeToAbsolute(argv[0]->data.a, argv[2]->data.i);
		stop = (stop > (intmax_t) mgListLength(argv[0])) ? length : stop;
	}

	intmax_t difference = stop - start;

	if (difference == 0)
		return mgCreateValueList(0);

	if (step == 0)
		step = (difference > 0) - (difference < 0);

	if ((difference ^ step) < 0)
		return mgCreateValueList(0);

	intmax_t sliceLength = difference / step + ((difference % step) != 0);

	MGValue *slice = mgCreateValueList((size_t) sliceLength);
	for (intmax_t i = start; i < stop; i += step)
		mgListAdd(slice, mgReferenceValue(_mgListGet(argv[0]->data.a, i)));

	return slice;
}


static MGValue* mg_list_reverse(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, 1);
	mgCheckArgumentTypes(instance, argc, argv, 1, MG_TYPE_LIST);

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
	mgCheckArgumentCount(instance, argc, 2, 2);
	mgCheckArgumentTypes(instance, argc, argv, 1, MG_TYPE_LIST, 3, MG_TYPE_CFUNCTION, MG_TYPE_BOUND_CFUNCTION, MG_TYPE_FUNCTION);

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
			MG_ASSERT(comparison->type == MG_TYPE_INTEGER);

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
