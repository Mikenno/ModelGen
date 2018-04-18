
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


static MGValue* mg_list_add(MGInstance *instance, size_t argc, MGValue **argv)
{
	if (argc < 2)
		MG_FAIL("Error: add expected at least 2 arguments, received %zu", argc);
	else if (argv[0]->type != MG_VALUE_LIST)
		MG_FAIL("Error: add expected argument %zu as \"%s\", received \"%s\"",
		        1, _MG_VALUE_TYPE_NAMES[MG_VALUE_LIST], _MG_VALUE_TYPE_NAMES[argv[0]->type]);

	for (size_t i = 1; i < argc; ++i)
		mgListAdd(argv[0], mgReferenceValue(argv[i]));

	return mgCreateValueNull();
}


static MGValue* mg_list_add_from(MGInstance *instance, size_t argc, MGValue **argv)
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
		mgListAdd(argv[0], mgReferenceValue(_mgListGet(argv[1]->data.a, i)));

	return mgCreateValueNull();
}


static MGValue* mg_list_insert(MGInstance *instance, size_t argc, MGValue **argv)
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

	mgListInsert(argv[0], index, mgReferenceValue(argv[2]));

	return mgCreateValueNull();
}


static MGValue* mg_list_clear(MGInstance *instance, size_t argc, MGValue **argv)
{
	if (argc != 1)
		MG_FAIL("Error: clear expects exactly 1 argument, received %zu", argc);
	else if (argv[0]->type != MG_VALUE_LIST)
		MG_FAIL("Error: clear expected argument as \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_LIST], _MG_VALUE_TYPE_NAMES[argv[0]->type]);

	mgListClear(argv[0]);

	return mgCreateValueNull();
}


static MGValue* mg_list_slice(MGInstance *instance, size_t argc, MGValue **argv)
{
	if (argc < 1)
		MG_FAIL("Error: slice expected at least 1 argument, received %zu", argc);
	else if (argc > 3)
		MG_FAIL("Error: slice expected at most 3 arguments, received %zu", argc);
	else if (argv[0]->type != MG_VALUE_LIST)
		MG_FAIL("Error: slice expected argument %zu as \"%s\", received \"%s\"",
		        1, _MG_VALUE_TYPE_NAMES[MG_VALUE_LIST], _MG_VALUE_TYPE_NAMES[argv[0]->type]);

	intmax_t begin = 0;
	intmax_t end = (intmax_t) mgListLength(argv[0]);

	if (argc > 1)
	{
		if (argv[1]->type != MG_VALUE_INTEGER)
			MG_FAIL("Error: slice expected argument %zu as \"%s\", received \"%s\"",
			        1, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[argv[1]->type]);

		begin = _mgListIndexRelativeToAbsolute(argv[0]->data.a, argv[1]->data.i);
		begin = (begin > 0) ? begin : 0;
	}

	if (argc > 2)
	{
		if (argv[2]->type != MG_VALUE_INTEGER)
			MG_FAIL("Error: slice expected argument %zu as \"%s\", received \"%s\"",
			        2, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[argv[2]->type]);

		end = _mgListIndexRelativeToAbsolute(argv[0]->data.a, argv[2]->data.i);
		end = (end > (intmax_t) mgListLength(argv[0])) ? (intmax_t) mgListLength(argv[0]) : end;
	}

	if (begin >= end)
		return mgCreateValueList(0);

	MGValue *slice = mgCreateValueList((size_t) (end - begin));

	for (intmax_t i = begin; i < end; ++i)
		mgListAdd(slice, mgReferenceValue(_mgListGet(argv[0]->data.a, i)));

	return slice;
}


MGValue* mgCreateListLib(void)
{
	MGValue *module = mgCreateValueModule();

	mgModuleSetCFunction(module, "add", mg_list_add); // list.add(list, item [, item...])
	mgModuleSetCFunction(module, "add_from", mg_list_add_from); // list.add_from(list, iterable)
	mgModuleSetCFunction(module, "insert", mg_list_insert); // list.insert(list, index, item)

	mgModuleSetCFunction(module, "clear", mg_list_clear); // list.clear(list)
	mgModuleSetCFunction(module, "size", mg_len); // list.size(list): int

	mgModuleSetCFunction(module, "slice", mg_list_slice); // list.size(list, begin = 0, end = len(list)): list

	return module;
}
