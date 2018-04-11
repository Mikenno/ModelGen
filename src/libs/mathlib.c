
#include <stdarg.h>
#include <math.h>
#include <limits.h>

#include "mathlib.h"

#include "../modelgen.h"
#include "../module.h"


#define _MG_PI 3.141592653589793238462643383279502884f
#define _MG_DEG2RAD (_MG_PI / 180.0f)
#define _MG_RAD2DEG (180.0f / _MG_PI)


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


static MGValue* mg_abs(MGInstance *instance, size_t argc, MGValue **argv)
{
	if (argc != 1)
		MG_FAIL("Error: abs expects exactly 1 argument, received %zu", argc);

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		return mgCreateValueInteger((argv[0]->data.i < 0) ? -argv[0]->data.i : argv[0]->data.i);
	case MG_VALUE_FLOAT:
		return mgCreateValueFloat(fabsf(argv[0]->data.f));
	default:
		MG_FAIL("Error: abs expected argument as \"%s\" or \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
		        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
		return mgCreateValueVoid();
	}
}


static MGValue* mg_deg(MGInstance *instance, size_t argc, MGValue **argv)
{
	if (argc != 1)
		MG_FAIL("Error: deg expects exactly 1 argument, received %zu", argc);

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		return mgCreateValueFloat(argv[0]->data.i * _MG_RAD2DEG);
	case MG_VALUE_FLOAT:
		return mgCreateValueFloat(argv[0]->data.f * _MG_RAD2DEG);
	default:
		MG_FAIL("Error: deg expected argument as \"%s\" or \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
		        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
		return mgCreateValueVoid();
	}
}


static MGValue* mg_rad(MGInstance *instance, size_t argc, MGValue **argv)
{
	if (argc != 1)
		MG_FAIL("Error: rad expects exactly 1 argument, received %zu", argc);

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		return mgCreateValueFloat(argv[0]->data.i * _MG_DEG2RAD);
	case MG_VALUE_FLOAT:
		return mgCreateValueFloat(argv[0]->data.f * _MG_DEG2RAD);
	default:
		MG_FAIL("Error: rad expected argument as \"%s\" or \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
		        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
		return mgCreateValueVoid();
	}
}


static MGValue* mg_ceil(MGInstance *instance, size_t argc, MGValue **argv)
{
	if (argc != 1)
		MG_FAIL("Error: ceil expects exactly 1 argument, received %zu", argc);

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		return mgCreateValueInteger(argv[0]->data.i);
	case MG_VALUE_FLOAT:
		return mgCreateValueFloat(ceilf(argv[0]->data.f));
	default:
		MG_FAIL("Error: ceil expected argument as \"%s\" or \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
		        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
		return mgCreateValueVoid();
	}
}


static MGValue* mg_floor(MGInstance *instance, size_t argc, MGValue **argv)
{
	if (argc != 1)
		MG_FAIL("Error: floor expects exactly 1 argument, received %zu", argc);

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		return mgCreateValueInteger(argv[0]->data.i);
	case MG_VALUE_FLOAT:
		return mgCreateValueFloat(floorf(argv[0]->data.f));
	default:
		MG_FAIL("Error: floor expected argument as \"%s\" or \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
		        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
		return mgCreateValueVoid();
	}
}


static MGValue* mg_round(MGInstance *instance, size_t argc, MGValue **argv)
{
	if (argc != 1)
		MG_FAIL("Error: round expects exactly 1 argument, received %zu", argc);

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		return mgCreateValueInteger(argv[0]->data.i);
	case MG_VALUE_FLOAT:
		return mgCreateValueFloat(roundf(argv[0]->data.f));
	default:
		MG_FAIL("Error: round expected argument as \"%s\" or \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
		        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
		return mgCreateValueVoid();
	}
}


static MGValue* mg_sign(MGInstance *instance, size_t argc, MGValue **argv)
{
	if (argc != 1)
		MG_FAIL("Error: sign expects exactly 1 argument, received %zu", argc);

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		return mgCreateValueInteger((0 < argv[0]->data.i) - (argv[0]->data.i < 0));
	case MG_VALUE_FLOAT:
		return mgCreateValueInteger((0.0f < argv[0]->data.f) - (argv[0]->data.f < 0.0f));
	default:
		MG_FAIL("Error: sign expected argument as \"%s\" or \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
		        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
		return mgCreateValueVoid();
	}
}


static inline int _mg_powi(int base, unsigned int exp)
{
	int result = 1;

	for (;;)
	{
		if (exp & 1)
			result *= base;

		exp >>= 1;

		if (!exp)
			break;

		base *= base;
	}

	return result;
}


static MGValue* mg_pow(MGInstance *instance, size_t argc, MGValue **argv)
{
	if (argc != 2)
		MG_FAIL("Error: pow expects exactly 2 arguments, received %zu", argc);

	for (size_t i = 0; i < 2; ++i)
		if ((argv[i]->type != MG_VALUE_INTEGER) && (argv[i]->type != MG_VALUE_FLOAT))
			MG_FAIL("Error: pow expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
			        i + 1, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
			        _MG_VALUE_TYPE_NAMES[argv[i]->type]);

	if ((argv[0]->type == MG_VALUE_INTEGER) && (argv[1]->type == MG_VALUE_INTEGER) && (argv[1]->data.i >= 0))
		return mgCreateValueInteger(_mg_powi(argv[0]->data.i, (unsigned int) argv[1]->data.i));
	else
		return mgCreateValueFloat(powf(
				(argv[0]->type == MG_VALUE_INTEGER) ? (float) argv[0]->data.i : argv[0]->data.f,
				(argv[1]->type == MG_VALUE_INTEGER) ? (float) argv[1]->data.i : argv[1]->data.f));
}


static MGValue* mg_sqrt(MGInstance *instance, size_t argc, MGValue **argv)
{
	if (argc != 1)
		MG_FAIL("Error: sqrt expects exactly 1 argument, received %zu", argc);

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		return mgCreateValueFloat(sqrtf((float) argv[0]->data.i));
	case MG_VALUE_FLOAT:
		return mgCreateValueFloat(sqrtf(argv[0]->data.f));
	default:
		MG_FAIL("Error: sqrt expected argument as \"%s\" or \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
		        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
		return mgCreateValueVoid();
	}
}


static MGValue* mg_cos(MGInstance *instance, size_t argc, MGValue **argv)
{
	if (argc != 1)
		MG_FAIL("Error: cos expects exactly 1 argument, received %zu", argc);

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		return mgCreateValueFloat(cosf((float) argv[0]->data.i));
	case MG_VALUE_FLOAT:
		return mgCreateValueFloat(cosf(argv[0]->data.f));
	default:
		MG_FAIL("Error: cos expected argument as \"%s\" or \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
		        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
		return mgCreateValueVoid();
	}
}


static MGValue* mg_sin(MGInstance *instance, size_t argc, MGValue **argv)
{
	if (argc != 1)
		MG_FAIL("Error: sin expects exactly 1 argument, received %zu", argc);

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		return mgCreateValueFloat(sinf((float) argv[0]->data.i));
	case MG_VALUE_FLOAT:
		return mgCreateValueFloat(sinf(argv[0]->data.f));
	default:
		MG_FAIL("Error: sin expected argument as \"%s\" or \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
		        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
		return mgCreateValueVoid();
	}
}


static MGValue* mg_tan(MGInstance *instance, size_t argc, MGValue **argv)
{
	if (argc != 1)
		MG_FAIL("Error: tan expects exactly 1 argument, received %zu", argc);

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		return mgCreateValueFloat(tanf((float) argv[0]->data.i));
	case MG_VALUE_FLOAT:
		return mgCreateValueFloat(tanf(argv[0]->data.f));
	default:
		MG_FAIL("Error: tan expected argument as \"%s\" or \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
		        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
		return mgCreateValueVoid();
	}
}


static MGValue* mg_acos(MGInstance *instance, size_t argc, MGValue **argv)
{
	if (argc != 1)
		MG_FAIL("Error: acos expects exactly 1 argument, received %zu", argc);

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		return mgCreateValueFloat(acosf((float) argv[0]->data.i));
	case MG_VALUE_FLOAT:
		return mgCreateValueFloat(acosf(argv[0]->data.f));
	default:
		MG_FAIL("Error: acos expected argument as \"%s\" or \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
		        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
		return mgCreateValueVoid();
	}
}


static MGValue* mg_asin(MGInstance *instance, size_t argc, MGValue **argv)
{
	if (argc != 1)
		MG_FAIL("Error: asin expects exactly 1 argument, received %zu", argc);

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		return mgCreateValueFloat(asinf((float) argv[0]->data.i));
	case MG_VALUE_FLOAT:
		return mgCreateValueFloat(asinf(argv[0]->data.f));
	default:
		MG_FAIL("Error: asin expected argument as \"%s\" or \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
		        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
		return mgCreateValueVoid();
	}
}


static MGValue* mg_atan(MGInstance *instance, size_t argc, MGValue **argv)
{
	if (argc != 1)
		MG_FAIL("Error: atan expects exactly 1 argument, received %zu", argc);

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		return mgCreateValueFloat(atanf((float) argv[0]->data.i));
	case MG_VALUE_FLOAT:
		return mgCreateValueFloat(atanf(argv[0]->data.f));
	default:
		MG_FAIL("Error: atan expected argument as \"%s\" or \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
		        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
		return mgCreateValueVoid();
	}
}


static MGValue* mg_atan2(MGInstance *instance, size_t argc, MGValue **argv)
{
	if (argc != 2)
		MG_FAIL("Error: atan2 expects exactly 2 arguments, received %zu", argc);

	for (size_t i = 0; i < 2; ++i)
		if ((argv[i]->type != MG_VALUE_INTEGER) && (argv[i]->type != MG_VALUE_FLOAT))
			MG_FAIL("Error: atan2 expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
			        i + 1, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
			        _MG_VALUE_TYPE_NAMES[argv[i]->type]);

	return mgCreateValueFloat(atan2f(
			(argv[0]->type == MG_VALUE_INTEGER) ? (float) argv[0]->data.i : argv[0]->data.f,
			(argv[1]->type == MG_VALUE_INTEGER) ? (float) argv[1]->data.i : argv[1]->data.f));
}


static MGValue* mg_exp(MGInstance *instance, size_t argc, MGValue **argv)
{
	if (argc != 1)
		MG_FAIL("Error: exp expects exactly 1 argument, received %zu", argc);

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		return mgCreateValueFloat(expf((float) argv[0]->data.i));
	case MG_VALUE_FLOAT:
		return mgCreateValueFloat(expf(argv[0]->data.f));
	default:
		MG_FAIL("Error: exp expected argument as \"%s\" or \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
		        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
		return mgCreateValueVoid();
	}
}


static MGValue* mg_log(MGInstance *instance, size_t argc, MGValue **argv)
{
	if (argc != 1)
		MG_FAIL("Error: log expects exactly 1 argument, received %zu", argc);

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		return mgCreateValueFloat(logf((float) argv[0]->data.i));
	case MG_VALUE_FLOAT:
		return mgCreateValueFloat(logf(argv[0]->data.f));
	default:
		MG_FAIL("Error: log expected argument as \"%s\" or \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
		        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
		return mgCreateValueVoid();
	}
}


static MGValue* mg_log2(MGInstance *instance, size_t argc, MGValue **argv)
{
	if (argc != 1)
		MG_FAIL("Error: log2 expects exactly 1 argument, received %zu", argc);

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		return mgCreateValueFloat(log2f((float) argv[0]->data.i));
	case MG_VALUE_FLOAT:
		return mgCreateValueFloat(log2f(argv[0]->data.f));
	default:
		MG_FAIL("Error: log2 expected argument as \"%s\" or \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
		        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
		return mgCreateValueVoid();
	}
}


MGValue* _mg_maxi(size_t argc, MGValue **argv)
{
	int result = INT_MIN;

	for (size_t i = 0; i < argc; ++i)
	{
		switch (argv[i]->type)
		{
		case MG_VALUE_INTEGER:
			result = (argv[i]->data.i > result) ? argv[i]->data.i : result;
			break;
		case MG_VALUE_FLOAT:
			result = ((int) argv[i]->data.f > result) ? (int) argv[i]->data.f : result;
			break;
		default:
			MG_FAIL("Error: max<%s> expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
			        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER],
			        i + 1,
			        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
			        _MG_VALUE_TYPE_NAMES[argv[i]->type]);
		}
	}

	return mgCreateValueInteger(result);
}


MGValue* _mg_maxf(size_t argc, MGValue **argv)
{
	float result = -INFINITY;

	for (size_t i = 0; i < argc; ++i)
	{
		switch (argv[i]->type)
		{
		case MG_VALUE_INTEGER:
			result = ((float) argv[i]->data.i > result) ? (float) argv[i]->data.i : result;
			break;
		case MG_VALUE_FLOAT:
			result = (argv[i]->data.f > result) ? argv[i]->data.f : result;
			break;
		default:
			MG_FAIL("Error: max<%s> expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
			        _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
			        i + 1,
			        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
			        _MG_VALUE_TYPE_NAMES[argv[i]->type]);
		}
	}

	return mgCreateValueFloat(result);
}


MGValue* _mg_mini(size_t argc, MGValue **argv)
{
	int result = INT_MAX;

	for (size_t i = 0; i < argc; ++i)
	{
		switch (argv[i]->type)
		{
		case MG_VALUE_INTEGER:
			result = (argv[i]->data.i < result) ? argv[i]->data.i : result;
			break;
		case MG_VALUE_FLOAT:
			result = ((int) argv[i]->data.f < result) ? (int) argv[i]->data.f : result;
			break;
		default:
			MG_FAIL("Error: min<%s> expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
			        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER],
			        i + 1,
			        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
			        _MG_VALUE_TYPE_NAMES[argv[i]->type]);
		}
	}

	return mgCreateValueInteger(result);
}


MGValue* _mg_minf(size_t argc, MGValue **argv)
{
	float result = INFINITY;

	for (size_t i = 0; i < argc; ++i)
	{
		switch (argv[i]->type)
		{
		case MG_VALUE_INTEGER:
			result = ((float) argv[i]->data.i < result) ? (float) argv[i]->data.i : result;
			break;
		case MG_VALUE_FLOAT:
			result = (argv[i]->data.f < result) ? argv[i]->data.f : result;
			break;
		default:
			MG_FAIL("Error: min<%s> expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
			        _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
			        i + 1,
			        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
			        _MG_VALUE_TYPE_NAMES[argv[i]->type]);
		}
	}

	return mgCreateValueFloat(result);
}


static MGValue* mg_max(MGInstance *instance, size_t argc, MGValue **argv)
{
	if (argc < 1)
		MG_FAIL("Error: max expected at least 1 argument, received %zu", argc);

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		return _mg_maxi(argc, argv);
	case MG_VALUE_FLOAT:
		return _mg_maxf(argc, argv);
	default:
		MG_FAIL("Error: max expected argument as \"%s\" or \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
		        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
		return mgCreateValueVoid();
	}
}


static MGValue* mg_min(MGInstance *instance, size_t argc, MGValue **argv)
{
	if (argc < 1)
		MG_FAIL("Error: min expected at least 1 argument, received %zu", argc);

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		return _mg_mini(argc, argv);
	case MG_VALUE_FLOAT:
		return _mg_minf(argc, argv);
	default:
		MG_FAIL("Error: min expected argument as \"%s\" or \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
		        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
		return mgCreateValueVoid();
	}
}


void mgLoadMathLib(MGModule *module)
{
	mgModuleSetFloat(module, "inf", INFINITY);
	mgModuleSetFloat(module, "nan", NAN);
	mgModuleSetFloat(module, "pi", _MG_PI);

	mgModuleSetCFunction(module, "abs", mg_abs);

	mgModuleSetCFunction(module, "sign", mg_sign);

	mgModuleSetCFunction(module, "deg", mg_deg);
	mgModuleSetCFunction(module, "rad", mg_rad);

	mgModuleSetCFunction(module, "ceil", mg_ceil);
	mgModuleSetCFunction(module, "floor", mg_floor);
	mgModuleSetCFunction(module, "round", mg_round);

	mgModuleSetCFunction(module, "pow", mg_pow);
	mgModuleSetCFunction(module, "sqrt", mg_sqrt);

	mgModuleSetCFunction(module, "cos", mg_cos);
	mgModuleSetCFunction(module, "sin", mg_sin);
	mgModuleSetCFunction(module, "tan", mg_tan);

	mgModuleSetCFunction(module, "acos", mg_acos);
	mgModuleSetCFunction(module, "asin", mg_asin);
	mgModuleSetCFunction(module, "atan", mg_atan);
	mgModuleSetCFunction(module, "atan2", mg_atan2);

	mgModuleSetCFunction(module, "exp", mg_exp);
	mgModuleSetCFunction(module, "log", mg_log);
	mgModuleSetCFunction(module, "log2", mg_log2);

	mgModuleSetCFunction(module, "max", mg_max);
	mgModuleSetCFunction(module, "min", mg_min);
}
