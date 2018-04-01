
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>

#include "modelgen.h"
#include "module.h"
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


static MGValue* mg_print(MGModule *module, size_t argc, MGValue **argv)
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

	return mgCreateValueVoid();
}


MGValue* _mg_rangei(int start, int stop, int step)
{
	int difference = stop - start;

	if (difference == 0)
		return mgCreateValueTuple(0);

	if (step == 0)
		step = (difference > 0) - (difference < 0);

	int length = difference / step + ((difference % step) != 0);

	if ((difference ^ step) < 0)
		return mgCreateValueTuple(0);

	MGValue *value = mgCreateValueTuple((size_t) length);
	for (int i = 0; i < length; ++i)
		mgTupleAdd(value, mgCreateValueInteger(start + step * i));

	return value;
}


static MGValue* mg_range(MGModule *module, size_t argc, MGValue **argv)
{
	if (argc < 1)
		MG_FAIL("Error: range expected at least 1 argument, received %zu", argc);
	else if (argc > 3)
		MG_FAIL("Error: range expected at most 3 arguments, received %zu", argc);

	for (size_t i = 0; i < argc; ++i)
		if (argv[i]->type != MG_VALUE_INTEGER)
			MG_FAIL("Error: rangef expected argument %zu as \"%s\", received \"%s\"",
			        i + 1, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[argv[i]->type]);

	int range[3] = { 0, 0, 0 };

	if (argc > 1)
		for (size_t i = 0; i < argc; ++i)
			range[i] = argv[i]->data.i;
	else
		range[1] = argv[0]->data.i;

	if ((argc > 2) && (range[2] == 0))
		MG_FAIL("Error: step cannot be 0");

	return _mg_rangei(range[0], range[1], range[2]);
}


static MGValue* mg_type(MGModule *module, size_t argc, MGValue **argv)
{
	if (argc != 1)
		MG_FAIL("Error: type expects exactly 1 argument, received %zu", argc);

	return mgCreateValueString(_MG_VALUE_TYPE_NAMES[argv[0]->type]);
}


static MGValue* mg_traceback(MGModule *module, size_t argc, MGValue **argv)
{
	MG_ASSERT(module);
	MG_ASSERT(module->instance);
	MG_ASSERT(module->instance->callStackTop);

	const MGStackFrame *frame = module->instance->callStackTop;

	while (frame->last)
		frame = frame->last;

	size_t depth = 0;

	while (frame)
	{
		printf("%zu: ", depth);
		mgInspectStackFrame(frame);
		putchar('\n');

		frame = frame->next;
		++depth;
	}

	return mgCreateValueVoid();
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
	mgModuleSetCFunction(module, "traceback", mg_traceback);
}
