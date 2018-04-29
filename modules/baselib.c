
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>

#include "modelgen.h"
#include "module.h"
#include "callable.h"
#include "eval.h"
#include "inspect.h"
#include "error.h"
#include "utilities.h"


static MGValue* mg_print(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	for (size_t i = 0; i < argc; ++i)
	{
		if (i > 0)
			putchar(' ');

		const MGValue *value = argv[i];

		if (value->type != MG_VALUE_STRING)
			mgInspectValueEx(argv[i], MG_FALSE);
		else if (value->data.str.s)
			fputs(value->data.str.s, stdout);
	}

	putchar('\n');

	return mgCreateValueNull();
}


MGValue* _mg_rangei(int start, int stop, int step)
{
	int difference = stop - start;

	if (difference == 0)
		return mgCreateValueList(0);

	if (step == 0)
		step = (difference > 0) - (difference < 0);

	if ((difference ^ step) < 0)
		return mgCreateValueList(0);

	int length = difference / step + ((difference % step) != 0);

	MGValue *range = mgCreateValueList((size_t) length);
	for (int i = 0; i < length; ++i)
		mgListAdd(range, mgCreateValueInteger(start + step * i));

	return range;
}


static MGValue* _mg_rangef(float start, float stop, float step)
{
	float difference = stop - start;

	if (_MG_FEQUAL(difference, 0.0f))
		return mgCreateValueList(0);

	if (_MG_FEQUAL(step, 0.0f))
		step = (float) ((difference > 0) - (difference < 0));

	int length = (int) ceilf((stop - start) / step);

	if (length < 0)
		return mgCreateValueList(0);

	MGValue *range = mgCreateValueList((size_t) length);
	for (int i = 0; i < length; ++i)
		mgListAdd(range, mgCreateValueFloat(start + step * i));

	return range;
}


// Returns a tuple containing values within the half-closed interval [start, stop)
static MGValue* mg_range(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, 3);
	mgCheckArgumentTypes(instance, argc, argv, 2, MG_VALUE_INTEGER, MG_VALUE_FLOAT, 2, MG_VALUE_INTEGER, MG_VALUE_FLOAT, 2, MG_VALUE_INTEGER, MG_VALUE_FLOAT);

	if (argc < 1)
		mgFatalError("Error: range expected at least 1 argument, received %zu", argc);
	else if (argc > 3)
		mgFatalError("Error: range expected at most 3 arguments, received %zu", argc);

	MGbool isInt = MG_TRUE;

	for (size_t i = 0; i < argc; ++i)
		if (argv[i]->type == MG_VALUE_FLOAT)
			isInt = MG_FALSE;

	union {
		int i[3];
		float f[3];
	} range;

	memset(&range, 0, sizeof(range));

	if (isInt)
	{
		if (argc > 1)
			for (size_t i = 0; i < argc; ++i)
				range.i[i] = argv[i]->data.i;
		else
			range.i[1] = argv[0]->data.i;
	}
	else
	{
		if (argc > 1)
			for (size_t i = 0; i < argc; ++i)
				range.f[i] = (argv[i]->type == MG_VALUE_INTEGER) ? (float) argv[i]->data.i : argv[i]->data.f;
		else
			range.f[1] = (argv[0]->type == MG_VALUE_INTEGER) ? (float) argv[0]->data.i : argv[0]->data.f;
	}

	if (argc > 2)
		if ((isInt && (range.i[2] == 0)) || (!isInt && _MG_FEQUAL(range.f[2], 0.0f)))
			mgFatalError("Error: step cannot be 0");

	return isInt ?
	       _mg_rangei(range.i[0], range.i[1], range.i[2]) :
	       _mg_rangef(range.f[0], range.f[1], range.f[2]);
}


static MGValue* mg_enumerate(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, 2);
	mgCheckArgumentTypes(instance, argc, argv, 2, MG_VALUE_TUPLE, MG_VALUE_LIST, 1, MG_VALUE_INTEGER);

	int start = (argc > 1) ? argv[1]->data.i : 0;

	size_t length = mgListLength(argv[0]);

	MGValue *enumerated = mgCreateValueList(length);
	for (size_t i = 0; i < length; ++i)
		mgListAdd(enumerated, mgCreateValueTupleEx(2, mgCreateValueInteger(start++), mgReferenceValue(_mgListGet(argv[0]->data.a, i))));

	return enumerated;
}


static MGValue* mg_consecutive(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, 2);
	mgCheckArgumentTypes(instance, argc, argv, 2, MG_VALUE_TUPLE, MG_VALUE_LIST, 1, MG_VALUE_INTEGER);

	intmax_t n = (argc > 1) ? (intmax_t) argv[1]->data.i : 2;

	const MGValue *list = argv[0];
	const intmax_t length = (intmax_t) mgListLength(list) - n + 1;

	if ((n < 1) || (length < 2))
		return mgCreateValueList(0);

	MGValue *result = mgCreateValueList((size_t) length);

	for (intmax_t i = 0; i < length; ++i)
	{
		MGValue *tuple = mgCreateValueTuple((size_t) n);

		for (intmax_t j = 0; j < n; ++j)
			mgListAdd(tuple, mgReferenceValue(_mgListGet(argv[0]->data.a, i + j)));

		mgListAdd(result, tuple);
	}

	return result;
}


static MGValue* mg_zip(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 2, 8);
	mgCheckArgumentTypes(instance, argc, argv,
	                     2, MG_VALUE_TUPLE, MG_VALUE_LIST,
	                     2, MG_VALUE_TUPLE, MG_VALUE_LIST,
	                     2, MG_VALUE_TUPLE, MG_VALUE_LIST,
	                     2, MG_VALUE_TUPLE, MG_VALUE_LIST,
	                     2, MG_VALUE_TUPLE, MG_VALUE_LIST,
	                     2, MG_VALUE_TUPLE, MG_VALUE_LIST,
	                     2, MG_VALUE_TUPLE, MG_VALUE_LIST,
	                     2, MG_VALUE_TUPLE, MG_VALUE_LIST);

	size_t minLength = SIZE_MAX;

	for (size_t i = 0; i < argc; ++i)
		minLength = (minLength > mgListLength(argv[i])) ? mgListLength(argv[i]) : minLength;

	MGValue *zipped = mgCreateValueList(minLength);

	for (size_t i = 0; i < minLength; ++i)
	{
		MGValue *tuple = mgCreateValueTuple(argc);

		for (size_t j = 0; j < argc; ++j)
			mgListAdd(tuple, mgReferenceValue(_mgListGet(argv[j]->data.a, i)));

		mgListAdd(zipped, tuple);
	}

	return zipped;
}


static MGValue* mg_map(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 2, 9);
	mgCheckArgumentTypes(instance, argc, argv,
	                     2, MG_VALUE_FUNCTION, MG_VALUE_CFUNCTION,
	                     2, MG_VALUE_TUPLE, MG_VALUE_LIST,
	                     2, MG_VALUE_TUPLE, MG_VALUE_LIST,
	                     2, MG_VALUE_TUPLE, MG_VALUE_LIST,
	                     2, MG_VALUE_TUPLE, MG_VALUE_LIST,
	                     2, MG_VALUE_TUPLE, MG_VALUE_LIST,
	                     2, MG_VALUE_TUPLE, MG_VALUE_LIST,
	                     2, MG_VALUE_TUPLE, MG_VALUE_LIST,
	                     2, MG_VALUE_TUPLE, MG_VALUE_LIST);

	size_t minLength = SIZE_MAX;

	for (size_t i = 1; i < argc; ++i)
		minLength = (minLength > mgListLength(argv[i])) ? mgListLength(argv[i]) : minLength;

	MGValue *mapped = mgCreateValueList(minLength);
	MGValue **argv2 = malloc((argc - 1) * sizeof(MGValue*));

	for (size_t i = 0; i < minLength; ++i)
	{
		for (size_t j = 1; j < argc; ++j)
			argv2[j - 1] = _mgListGet(argv[j]->data.a, i); // Purposely not referenced

		mgListAdd(mapped, mgCall(instance, argv[0], argc - 1, (const MGValue* const*) argv2));
	}

	free(argv2);

	return mapped;
}


static MGValue* mg_filter(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 2, 2);
	mgCheckArgumentTypes(instance, argc, argv, 2, MG_VALUE_FUNCTION, MG_VALUE_CFUNCTION, 2, MG_VALUE_TUPLE, MG_VALUE_LIST);

	const MGValue *callable = argv[0];
	const MGValue *list = argv[1];
	const size_t length = mgListLength(list);

	MGValue *result = mgCreateValueList(length / 2);

	for (size_t i = 0; i < length; ++i)
	{
		MGValue *item = _mgListGet(list->data.a, i);
		const MGValue* const argv2[1] = { item }; // Purposely not referenced

		MGValue *filtered = mgCall(instance, callable, 1, argv2);
		MG_ASSERT(filtered);
		MG_ASSERT(filtered->type == MG_VALUE_INTEGER);

		if (filtered->data.i)
			mgListAdd(result, mgReferenceValue(item));

		mgDestroyValue(filtered);
	}

	return result;
}


static MGValue* mg_reduce(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 2, 2);
	mgCheckArgumentTypes(instance, argc, argv, 2, MG_VALUE_FUNCTION, MG_VALUE_CFUNCTION, 2, MG_VALUE_TUPLE, MG_VALUE_LIST);

	const MGValue *callable = argv[0];
	const MGValue *list = argv[1];
	const size_t length = mgListLength(list);

	if (length < 1)
		mgFatalError("Error: reduce expected argument %zu as a non-empty \"%s\"",
		        2, _MG_VALUE_TYPE_NAMES[list->type]);

	MGValue *result = mgReferenceValue(_mgListGet(list->data.a, 0));

	for (size_t i = 1; i < length; ++i)
	{
		MGValue *item = _mgListGet(list->data.a, i);
		const MGValue* const argv2[2] = { result, item }; // Purposely not referenced

		MGValue *_result = mgCall(instance, callable, 2, argv2);
		MG_ASSERT(_result);

		if (result)
			mgDestroyValue(result);

		result = _result;
	}

	return result;
}


static MGValue* mg_type(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, 1);
	mgCheckArgumentTypes(instance, argc, argv, 0);

	return mgCreateValueStringEx(_MG_VALUE_TYPE_NAMES[argv[0]->type], MG_STRING_USAGE_STATIC);
}


MGValue* mg_len(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, 1);

	switch (argv[0]->type)
	{
	case MG_VALUE_TUPLE:
	case MG_VALUE_LIST:
		return mgCreateValueInteger((int) mgListLength(argv[0]));
	case MG_VALUE_MAP:
		return mgCreateValueInteger((int) mgMapSize(argv[0]));
	case MG_VALUE_STRING:
		return mgCreateValueInteger((int) mgStringLength(argv[0]));
	default:
		mgFatalError("Error: \"%s\" has no length", _MG_VALUE_TYPE_NAMES[argv[0]->type]);
		return mgCreateValueNull();
	}
}


static MGValue* mg_int(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, 2);

	if (argc == 2)
		mgCheckArgumentTypes(instance, argc, argv, 1, MG_VALUE_STRING, 1, MG_VALUE_INTEGER);
	else
		mgCheckArgumentTypes(instance, argc, argv, 3, MG_VALUE_INTEGER, MG_VALUE_FLOAT, MG_VALUE_STRING);

	int base = (argc == 2) ? argv[1]->data.i : 10;

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		return mgCreateValueInteger(argv[0]->data.i);
	case MG_VALUE_FLOAT:
		return mgCreateValueInteger((int) argv[0]->data.f);
	case MG_VALUE_STRING:
		return mgCreateValueInteger(strtol(argv[0]->data.str.s, NULL, base));
	default:
		return mgCreateValueNull();
	}
}


static MGValue* mg_float(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, 1);
	mgCheckArgumentTypes(instance, argc, argv, 3, MG_VALUE_INTEGER, MG_VALUE_FLOAT, MG_VALUE_STRING);

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		return mgCreateValueFloat((float) argv[0]->data.i);
	case MG_VALUE_FLOAT:
		return mgCreateValueFloat(argv[0]->data.f);
	case MG_VALUE_STRING:
		return mgCreateValueFloat(strtof(argv[0]->data.str.s, NULL));
	default:
		return mgCreateValueNull();
	}
}


static MGValue* mg_traceback(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	MG_ASSERT(instance);
	MG_ASSERT(instance->callStackTop);

	mgCheckArgumentCount(instance, argc, 1, 1);

	const MGStackFrame *frame = instance->callStackTop;
	MG_ASSERT(frame);

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

	return mgCreateValueNull();
}


static MGValue* mg_globals(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	MG_ASSERT(instance);
	MG_ASSERT(instance->callStackTop);
	MG_ASSERT(instance->callStackTop->module);
	MG_ASSERT(instance->callStackTop->module->type == MG_VALUE_MODULE);
	MG_ASSERT(instance->callStackTop->module->data.module.globals);

	mgCheckArgumentCount(instance, argc, 0, 0);

	return mgReferenceValue(instance->callStackTop->module->data.module.globals);
}


static MGValue* mg_locals(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	MG_ASSERT(instance);
	MG_ASSERT(instance->callStackTop);
	MG_ASSERT(instance->callStackTop->last);
	MG_ASSERT(instance->callStackTop->last->locals);

	mgCheckArgumentCount(instance, argc, 0, 0);

	return mgReferenceValue(instance->callStackTop->last->locals);
}


static MGValue* mg_import(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	MG_ASSERT(instance);

	mgCheckArgumentCount(instance, argc, 1, 1);
	mgCheckArgumentTypes(instance, argc, argv, 1, MG_VALUE_STRING);

	return mgImportModule(instance, argv[0]->data.str.s);
}


static MGValue* mg_eval(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	MG_ASSERT(instance);

	mgCheckArgumentCount(instance, argc, 1, 2);
	mgCheckArgumentTypes(instance, argc, argv, 1, MG_VALUE_STRING, 1, MG_VALUE_MAP);

	return mgEvalEx(instance, argv[0]->data.str.s, (argc > 1) ? argv[1] : NULL);
}


MGValue* mgCreateBaseLib(void)
{
	MGValue *module = mgCreateValueModule();

	MG_ASSERT(module);
	MG_ASSERT(module->type == MG_VALUE_MODULE);

	mgModuleSetInteger(module, "false", 0);
	mgModuleSetInteger(module, "true", 1);

	mgModuleSet(module, "version", mgCreateValueTupleEx(3, mgCreateValueInteger(MG_MAJOR_VERSION), mgCreateValueInteger(MG_MINOR_VERSION), mgCreateValueInteger(MG_PATCH_VERSION)));

	mgModuleSetCFunction(module, "print", mg_print);

	mgModuleSetCFunction(module, "range", mg_range);
	mgModuleSetCFunction(module, "enumerate", mg_enumerate);

	mgModuleSetCFunction(module, "consecutive", mg_consecutive);
	mgModuleSetCFunction(module, "zip", mg_zip);

	mgModuleSetCFunction(module, "map", mg_map);
	mgModuleSetCFunction(module, "filter", mg_filter);
	mgModuleSetCFunction(module, "reduce", mg_reduce);

	mgModuleSetCFunction(module, "type", mg_type);
	mgModuleSetCFunction(module, "len", mg_len);

	mgModuleSetCFunction(module, "int", mg_int);
	mgModuleSetCFunction(module, "float", mg_float);

	mgModuleSetCFunction(module, "traceback", mg_traceback);

	mgModuleSetCFunction(module, "globals", mg_globals);
	mgModuleSetCFunction(module, "locals", mg_locals);

	mgModuleSetCFunction(module, "__import", mg_import);

	mgModuleSetCFunction(module, "__eval", mg_eval);

	return module;
}
