
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>

#include "modelgen.h"
#include "module.h"
#include "inspect.h"


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


static MGValue* mg_print(size_t argc, MGValue **argv)
{
	for (size_t i = 0; i < argc; ++i)
	{
		if (i > 0)
			putchar(' ');

		MGValue *value = argv[i];

		if (value->type != MG_VALUE_STRING)
			_mgInspectValue(argv[i]);
		else if (value->data.s)
			fputs(value->data.s, stdout);
	}

	putchar('\n');

	return mgCreateValueInteger(0);
}


static MGValue* mg_range(size_t argc, MGValue **argv)
{
	if (argc < 1)
		MG_FAIL("Error: range expected at least 1 argument, received %zu", argc);
	else if (argc > 3)
		MG_FAIL("Error: range expected at most 3 arguments, received %zu", argc);

	for (size_t i = 0; i < argc; ++i)
		if (argv[i]->type != MG_VALUE_INTEGER)
			MG_FAIL("Error: range expected argument %zu as \"%s\", received \"%s\"",
			        i + 1, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[argv[i]->type]);

	int range[3] = { 0, 0, 1 };

	if (argc > 1)
		for (size_t i = 0; i < argc; ++i)
			range[i] = argv[i]->data.i;
	else
		range[1] = argv[0]->data.i;

	if (range[2] == 0)
		MG_FAIL("Error: step cannot be 0", argc);

	int length = (range[1] - range[0]) / range[2] + (((range[1] - range[0]) % range[2]) != 0);
	MG_ASSERT(length >= 0);

	MGValue *value = mgCreateValueTuple((size_t) length);
	for (int i = 0; i < length; ++i)
		mgTupleAdd(value, mgCreateValueInteger(range[0] + range[2] * i));

	return value;
}


static MGValue* mg_type(size_t argc, MGValue **argv)
{
	if (argc != 1)
		MG_FAIL("Error: type expects exactly 1 argument, received %zu", argc);

	return mgCreateValueString(_MG_VALUE_TYPE_NAMES[argv[0]->type]);
}


void mgLoadBaseLib(MGModule *module)
{
	mgModuleSetInteger(module, "false", 0);
	mgModuleSetInteger(module, "true", 1);

	MGValue *version = mgCreateValueTuple(3);
	mgTupleAdd(version, mgCreateValueInteger(MG_MAJOR_VERSION));
	mgTupleAdd(version, mgCreateValueInteger(MG_MINOR_VERSION));
	mgTupleAdd(version, mgCreateValueInteger(MG_PATCH_VERSION));
	mgModuleSet(module, "version", version);

	mgModuleSetCFunction(module, "print", mg_print);
	mgModuleSetCFunction(module, "range", mg_range);
	mgModuleSetCFunction(module, "type", mg_type);
}
