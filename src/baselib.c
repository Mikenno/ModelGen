
#include <stdlib.h>
#include <stdarg.h>

#include "modelgen.h"
#include "inspect.h"
#include "utilities.h"


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

	MGValue *value = mgCreateValue(MG_VALUE_INTEGER);
	value->data.i = 0;

	return value;
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

	MGValue *value = mgCreateValue(MG_VALUE_TUPLE);

	if (length > 0)
	{
		value->data.a.items = (MGValue**) malloc(length * sizeof(MGValue*));
		value->data.a.length = (size_t) length;
		value->data.a.capacity = (size_t) length;

		for (int i = 0; i < length; ++i)
		{
			MGValue *item = mgCreateValue(MG_VALUE_INTEGER);
			item->data.i = range[0] + range[2] * i;

			value->data.a.items[i] = item;
		}
	}
	else
	{
		value->data.a.items = NULL;
		value->data.a.length = 0;
		value->data.a.capacity = 0;
	}

	return value;
}


static MGValue* mg_type(size_t argc, MGValue **argv)
{
	if (argc != 1)
		MG_FAIL("Error: type expects exactly 1 argument, received %zu", argc);

	MGValue *value = mgCreateValue(MG_VALUE_STRING);
	value->data.s = mgDuplicateString(_MG_VALUE_TYPE_NAMES[argv[0]->type]);

	return value;
}


void mgLoadBaseLib(MGModule *module)
{
	mgSetValueInteger(module, "false", 0);
	mgSetValueInteger(module, "true", 1);

	mgSetValueCFunction(module, "print", mg_print);
	mgSetValueCFunction(module, "range", mg_range);
	mgSetValueCFunction(module, "type", mg_type);
}
