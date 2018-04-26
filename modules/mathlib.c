
#include <stdarg.h>
#include <math.h>
#include <limits.h>

#include "modelgen.h"
#include "module.h"
#include "error.h"
#include "utilities.h"


#define _MG_PI  3.141592653589793238462643383279502884f
#define _MG_TAU 6.283185307179586476925286766559005768f

#define _MG_DEG2RAD (_MG_PI / 180.0f)
#define _MG_RAD2DEG (180.0f / _MG_PI)


static MGValue* mg_abs(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	if (argc != 1)
		mgFatalError("Error: abs expects exactly 1 argument, received %zu", argc);

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		return mgCreateValueInteger((argv[0]->data.i < 0) ? -argv[0]->data.i : argv[0]->data.i);
	case MG_VALUE_FLOAT:
		return mgCreateValueFloat(fabsf(argv[0]->data.f));
	default:
		mgFatalError("Error: abs expected argument as \"%s\" or \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
		        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
		return mgCreateValueNull();
	}
}


static MGValue* mg_deg(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	if (argc != 1)
		mgFatalError("Error: deg expects exactly 1 argument, received %zu", argc);

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		return mgCreateValueFloat(argv[0]->data.i * _MG_RAD2DEG);
	case MG_VALUE_FLOAT:
		return mgCreateValueFloat(argv[0]->data.f * _MG_RAD2DEG);
	default:
		mgFatalError("Error: deg expected argument as \"%s\" or \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
		        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
		return mgCreateValueNull();
	}
}


static MGValue* mg_rad(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	if (argc != 1)
		mgFatalError("Error: rad expects exactly 1 argument, received %zu", argc);

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		return mgCreateValueFloat(argv[0]->data.i * _MG_DEG2RAD);
	case MG_VALUE_FLOAT:
		return mgCreateValueFloat(argv[0]->data.f * _MG_DEG2RAD);
	default:
		mgFatalError("Error: rad expected argument as \"%s\" or \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
		        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
		return mgCreateValueNull();
	}
}


static MGValue* mg_sign(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	if (argc != 1)
		mgFatalError("Error: sign expects exactly 1 argument, received %zu", argc);

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		return mgCreateValueInteger((0 < argv[0]->data.i) - (argv[0]->data.i < 0));
	case MG_VALUE_FLOAT:
		return mgCreateValueInteger((0.0f < argv[0]->data.f) - (argv[0]->data.f < 0.0f));
	default:
		mgFatalError("Error: sign expected argument as \"%s\" or \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
		        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
		return mgCreateValueNull();
	}
}


static MGValue* mg_even(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	if (argc != 1)
		mgFatalError("Error: even expects exactly 1 argument, received %zu", argc);

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		return mgCreateValueInteger((argv[0]->data.i & 1) == 0);
	case MG_VALUE_FLOAT:
		return mgCreateValueInteger(_MG_FEQUAL(fmodf(argv[0]->data.f, 2.0f), 0.0f));
	default:
		mgFatalError("Error: even expected argument as \"%s\" or \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
		        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
		return mgCreateValueNull();
	}
}


static MGValue* mg_odd(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	if (argc != 1)
		mgFatalError("Error: odd expects exactly 1 argument, received %zu", argc);

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		return mgCreateValueInteger((argv[0]->data.i & 1) == 1);
	case MG_VALUE_FLOAT:
		return mgCreateValueInteger(_MG_FEQUAL(fmodf(argv[0]->data.f, 2.0f), 1.0f));
	default:
		mgFatalError("Error: odd expected argument as \"%s\" or \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
		        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
		return mgCreateValueNull();
	}
}


// Check if b is a multiple of a
static MGValue* mg_multiple(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	if (argc != 2)
		mgFatalError("Error: multiple expects exactly 2 arguments, received %zu", argc);

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		switch (argv[1]->type)
		{
		case MG_VALUE_INTEGER:
			return mgCreateValueInteger((argv[1]->data.i % argv[0]->data.i) == 0);
		case MG_VALUE_FLOAT:
			return mgCreateValueInteger(_MG_FEQUAL(fmodf(argv[1]->data.f, (float) argv[0]->data.i), 0.0f));
		default:
			mgFatalError("Error: multiple expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
			        2, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
			        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
			return mgCreateValueNull();
		}
	case MG_VALUE_FLOAT:
		switch (argv[1]->type)
		{
		case MG_VALUE_INTEGER:
			return mgCreateValueInteger(_MG_FEQUAL(fmodf((float) argv[1]->data.i, argv[0]->data.f), 0.0f));
		case MG_VALUE_FLOAT:
			return mgCreateValueInteger(_MG_FEQUAL(fmodf(argv[1]->data.f, argv[0]->data.f), 0.0f));
		default:
			mgFatalError("Error: multiple expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
			        2, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
			        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
			return mgCreateValueNull();
		}
	default:
		mgFatalError("Error: multiple expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
		        1, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
		        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
		return mgCreateValueNull();
	}
}


static MGValue* mg_ceil(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	if (argc != 1)
		mgFatalError("Error: ceil expects exactly 1 argument, received %zu", argc);

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		return mgCreateValueInteger(argv[0]->data.i);
	case MG_VALUE_FLOAT:
		return mgCreateValueFloat(ceilf(argv[0]->data.f));
	default:
		mgFatalError("Error: ceil expected argument as \"%s\" or \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
		        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
		return mgCreateValueNull();
	}
}


static MGValue* mg_floor(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	if (argc != 1)
		mgFatalError("Error: floor expects exactly 1 argument, received %zu", argc);

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		return mgCreateValueInteger(argv[0]->data.i);
	case MG_VALUE_FLOAT:
		return mgCreateValueFloat(floorf(argv[0]->data.f));
	default:
		mgFatalError("Error: floor expected argument as \"%s\" or \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
		        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
		return mgCreateValueNull();
	}
}


static MGValue* mg_round(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	if (argc != 1)
		mgFatalError("Error: round expects exactly 1 argument, received %zu", argc);

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		return mgCreateValueInteger(argv[0]->data.i);
	case MG_VALUE_FLOAT:
		return mgCreateValueFloat(roundf(argv[0]->data.f));
	default:
		mgFatalError("Error: round expected argument as \"%s\" or \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
		        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
		return mgCreateValueNull();
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


static MGValue* mg_pow(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	if (argc != 2)
		mgFatalError("Error: pow expects exactly 2 arguments, received %zu", argc);

	for (size_t i = 0; i < 2; ++i)
		if ((argv[i]->type != MG_VALUE_INTEGER) && (argv[i]->type != MG_VALUE_FLOAT))
			mgFatalError("Error: pow expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
			        i + 1, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
			        _MG_VALUE_TYPE_NAMES[argv[i]->type]);

	if ((argv[0]->type == MG_VALUE_INTEGER) && (argv[1]->type == MG_VALUE_INTEGER) && (argv[1]->data.i >= 0))
		return mgCreateValueInteger(_mg_powi(argv[0]->data.i, (unsigned int) argv[1]->data.i));
	else
		return mgCreateValueFloat(powf(
				(argv[0]->type == MG_VALUE_INTEGER) ? (float) argv[0]->data.i : argv[0]->data.f,
				(argv[1]->type == MG_VALUE_INTEGER) ? (float) argv[1]->data.i : argv[1]->data.f));
}


static MGValue* mg_sqrt(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	if (argc != 1)
		mgFatalError("Error: sqrt expects exactly 1 argument, received %zu", argc);

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		return mgCreateValueFloat(sqrtf((float) argv[0]->data.i));
	case MG_VALUE_FLOAT:
		return mgCreateValueFloat(sqrtf(argv[0]->data.f));
	default:
		mgFatalError("Error: sqrt expected argument as \"%s\" or \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
		        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
		return mgCreateValueNull();
	}
}


static MGValue* mg_cos(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	if (argc != 1)
		mgFatalError("Error: cos expects exactly 1 argument, received %zu", argc);

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		return mgCreateValueFloat(cosf((float) argv[0]->data.i));
	case MG_VALUE_FLOAT:
		return mgCreateValueFloat(cosf(argv[0]->data.f));
	default:
		mgFatalError("Error: cos expected argument as \"%s\" or \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
		        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
		return mgCreateValueNull();
	}
}


static MGValue* mg_sin(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	if (argc != 1)
		mgFatalError("Error: sin expects exactly 1 argument, received %zu", argc);

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		return mgCreateValueFloat(sinf((float) argv[0]->data.i));
	case MG_VALUE_FLOAT:
		return mgCreateValueFloat(sinf(argv[0]->data.f));
	default:
		mgFatalError("Error: sin expected argument as \"%s\" or \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
		        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
		return mgCreateValueNull();
	}
}


static MGValue* mg_tan(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	if (argc != 1)
		mgFatalError("Error: tan expects exactly 1 argument, received %zu", argc);

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		return mgCreateValueFloat(tanf((float) argv[0]->data.i));
	case MG_VALUE_FLOAT:
		return mgCreateValueFloat(tanf(argv[0]->data.f));
	default:
		mgFatalError("Error: tan expected argument as \"%s\" or \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
		        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
		return mgCreateValueNull();
	}
}


static MGValue* mg_acos(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	if (argc != 1)
		mgFatalError("Error: acos expects exactly 1 argument, received %zu", argc);

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		return mgCreateValueFloat(acosf((float) argv[0]->data.i));
	case MG_VALUE_FLOAT:
		return mgCreateValueFloat(acosf(argv[0]->data.f));
	default:
		mgFatalError("Error: acos expected argument as \"%s\" or \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
		        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
		return mgCreateValueNull();
	}
}


static MGValue* mg_asin(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	if (argc != 1)
		mgFatalError("Error: asin expects exactly 1 argument, received %zu", argc);

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		return mgCreateValueFloat(asinf((float) argv[0]->data.i));
	case MG_VALUE_FLOAT:
		return mgCreateValueFloat(asinf(argv[0]->data.f));
	default:
		mgFatalError("Error: asin expected argument as \"%s\" or \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
		        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
		return mgCreateValueNull();
	}
}


static MGValue* mg_atan(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	if (argc != 1)
		mgFatalError("Error: atan expects exactly 1 argument, received %zu", argc);

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		return mgCreateValueFloat(atanf((float) argv[0]->data.i));
	case MG_VALUE_FLOAT:
		return mgCreateValueFloat(atanf(argv[0]->data.f));
	default:
		mgFatalError("Error: atan expected argument as \"%s\" or \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
		        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
		return mgCreateValueNull();
	}
}


static MGValue* mg_atan2(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	if (argc != 2)
		mgFatalError("Error: atan2 expects exactly 2 arguments, received %zu", argc);

	for (size_t i = 0; i < 2; ++i)
		if ((argv[i]->type != MG_VALUE_INTEGER) && (argv[i]->type != MG_VALUE_FLOAT))
			mgFatalError("Error: atan2 expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
			        i + 1, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
			        _MG_VALUE_TYPE_NAMES[argv[i]->type]);

	return mgCreateValueFloat(atan2f(
			(argv[0]->type == MG_VALUE_INTEGER) ? (float) argv[0]->data.i : argv[0]->data.f,
			(argv[1]->type == MG_VALUE_INTEGER) ? (float) argv[1]->data.i : argv[1]->data.f));
}


static MGValue* mg_exp(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	if (argc != 1)
		mgFatalError("Error: exp expects exactly 1 argument, received %zu", argc);

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		return mgCreateValueFloat(expf((float) argv[0]->data.i));
	case MG_VALUE_FLOAT:
		return mgCreateValueFloat(expf(argv[0]->data.f));
	default:
		mgFatalError("Error: exp expected argument as \"%s\" or \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
		        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
		return mgCreateValueNull();
	}
}


static MGValue* mg_log(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	if (argc != 1)
		mgFatalError("Error: log expects exactly 1 argument, received %zu", argc);

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		return mgCreateValueFloat(logf((float) argv[0]->data.i));
	case MG_VALUE_FLOAT:
		return mgCreateValueFloat(logf(argv[0]->data.f));
	default:
		mgFatalError("Error: log expected argument as \"%s\" or \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
		        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
		return mgCreateValueNull();
	}
}


static MGValue* mg_log2(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	if (argc != 1)
		mgFatalError("Error: log2 expects exactly 1 argument, received %zu", argc);

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		return mgCreateValueFloat(log2f((float) argv[0]->data.i));
	case MG_VALUE_FLOAT:
		return mgCreateValueFloat(log2f(argv[0]->data.f));
	default:
		mgFatalError("Error: log2 expected argument as \"%s\" or \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
		        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
		return mgCreateValueNull();
	}
}


MGValue* _mg_max(size_t argc, const MGValue* const* argv)
{
	MG_ASSERT(argc > 0);

	MGbool isInt;

	union {
		int i;
		float f;
	} result;

	memset(&result, 0, sizeof(result));

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		isInt = MG_TRUE;
		result.i = argv[0]->data.i;
		break;
	case MG_VALUE_FLOAT:
		isInt = MG_FALSE;
		result.f = argv[0]->data.f;
		break;
	default:
		mgFatalError("Error: max expected argument as \"%s\" or \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
		        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
		return mgCreateValueNull();
	}

	for (size_t i = 1; i < argc; ++i)
	{
		if (isInt && (argv[i]->type == MG_VALUE_FLOAT))
		{
			isInt = MG_FALSE;
			result.f = (float) result.i;
		}

		switch (argv[i]->type)
		{
		case MG_VALUE_INTEGER:
			if (isInt)
				result.i = (argv[i]->data.i > result.i) ? argv[i]->data.i : result.i;
			else
				result.f = ((float) argv[i]->data.i > result.f) ? (float) argv[i]->data.i : result.f;
			break;
		case MG_VALUE_FLOAT:
			MG_ASSERT(!isInt);
			result.f = (argv[i]->data.f > result.f) ? argv[i]->data.f : result.f;
			break;
		default:
			mgFatalError("Error: max expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
			        i + 1, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
			        _MG_VALUE_TYPE_NAMES[argv[i]->type]);
		}
	}

	return isInt ? mgCreateValueInteger(result.i) : mgCreateValueFloat(result.f);
}


MGValue* _mg_min(size_t argc, const MGValue* const* argv)
{
	MG_ASSERT(argc > 0);

	MGbool isInt;

	union {
		int i;
		float f;
	} result;

	memset(&result, 0, sizeof(result));

	switch (argv[0]->type)
	{
	case MG_VALUE_INTEGER:
		isInt = MG_TRUE;
		result.i = argv[0]->data.i;
		break;
	case MG_VALUE_FLOAT:
		isInt = MG_FALSE;
		result.f = argv[0]->data.f;
		break;
	default:
		mgFatalError("Error: min expected argument as \"%s\" or \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
		        _MG_VALUE_TYPE_NAMES[argv[0]->type]);
		return mgCreateValueNull();
	}

	for (size_t i = 1; i < argc; ++i)
	{
		if (isInt && (argv[i]->type == MG_VALUE_FLOAT))
		{
			isInt = MG_FALSE;
			result.f = (float) result.i;
		}

		switch (argv[i]->type)
		{
		case MG_VALUE_INTEGER:
			if (isInt)
				result.i = (argv[i]->data.i < result.i) ? argv[i]->data.i : result.i;
			else
				result.f = ((float) argv[i]->data.i < result.f) ? (float) argv[i]->data.i : result.f;
			break;
		case MG_VALUE_FLOAT:
			MG_ASSERT(!isInt);
			result.f = (argv[i]->data.f < result.f) ? argv[i]->data.f : result.f;
			break;
		default:
			mgFatalError("Error: min expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
			        i + 1, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
			        _MG_VALUE_TYPE_NAMES[argv[i]->type]);
		}
	}

	return isInt ? mgCreateValueInteger(result.i) : mgCreateValueFloat(result.f);
}


static MGValue* mg_max(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	if (argc < 1)
		mgFatalError("Error: max expected at least 1 argument, received %zu", argc);

	if (argc == 1)
	{
		if ((argv[0]->type != MG_VALUE_TUPLE) && (argv[0]->type != MG_VALUE_LIST))
			mgFatalError("Error: max expected argument %zu as \"%s\", received \"%s\"",
			        1, _MG_VALUE_TYPE_NAMES[MG_VALUE_LIST], _MG_VALUE_TYPE_NAMES[argv[0]->type]);

		return _mg_max(mgListLength(argv[0]), (const MGValue* const*) mgListItems(argv[0]));
	}
	else
		return _mg_max(argc, argv);
}


static MGValue* mg_min(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	if (argc < 1)
		mgFatalError("Error: min expected at least 1 argument, received %zu", argc);

	if (argc == 1)
	{
		if ((argv[0]->type != MG_VALUE_TUPLE) && (argv[0]->type != MG_VALUE_LIST))
			mgFatalError("Error: min expected argument %zu as \"%s\", received \"%s\"",
			        1, _MG_VALUE_TYPE_NAMES[MG_VALUE_LIST], _MG_VALUE_TYPE_NAMES[argv[0]->type]);

		return _mg_min(mgListLength(argv[0]), (const MGValue* const*) mgListItems(argv[0]));
	}
	else
		return _mg_min(argc, argv);
}


static MGValue* mg_clamp(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	const MGValue *value = argv[0];
	const MGValue *min = argv[1];
	const MGValue *max = argv[2];

	if (argc != 3)
		mgFatalError("Error: clamp expects exactly 3 arguments, received %zu", argc);
	else if ((value->type != MG_VALUE_INTEGER) && (value->type != MG_VALUE_FLOAT))
		mgFatalError("Error: clamp expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
		        1, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT], _MG_VALUE_TYPE_NAMES[value->type]);
	else if ((min->type != MG_VALUE_INTEGER) && (min->type != MG_VALUE_FLOAT))
		mgFatalError("Error: clamp expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
		        2, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT], _MG_VALUE_TYPE_NAMES[min->type]);
	else if ((max->type != MG_VALUE_INTEGER) && (max->type != MG_VALUE_FLOAT))
		mgFatalError("Error: clamp expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
		        3, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT], _MG_VALUE_TYPE_NAMES[max->type]);

	if ((value->type == MG_VALUE_FLOAT) || (min->type == MG_VALUE_FLOAT) || (max->type == MG_VALUE_FLOAT))
	{
		float result = (value->type == MG_VALUE_INTEGER) ? (float) value->data.i : value->data.f;
		result = (min->type == MG_VALUE_INTEGER) ? ((result < min->data.i) ? (float) min->data.i : result) : ((result < min->data.f) ? min->data.f : result);
		result = (max->type == MG_VALUE_INTEGER) ? ((result > max->data.i) ? (float) max->data.i : result) : ((result > max->data.f) ? max->data.f : result);

		return mgCreateValueFloat(result);
	}
	else
	{
		int result = value->data.i;
		result = (result < min->data.i) ? min->data.i : result;
		result = (result > max->data.i) ? max->data.i : result;

		return mgCreateValueInteger(result);
	}
}


static MGValue* mg_sum(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	const MGValue *list = argv[0];

	if (argc != 1)
		mgFatalError("Error: sum expects exactly 1 argument, received %zu", argc);
	else if ((list->type != MG_VALUE_TUPLE) && (list->type != MG_VALUE_LIST))
		mgFatalError("Error: sum expected argument as \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_LIST], _MG_VALUE_TYPE_NAMES[list->type]);

	MGbool isInt = MG_TRUE;

	union {
		int i;
		float f;
	} result;

	memset(&result, 0, sizeof(result));

	const size_t length = mgListLength(list);

	for (size_t i = 0; i < length; ++i)
	{
		MGValue *item = _mgListGet(list->data.a, i);

		if (isInt && (item->type == MG_VALUE_FLOAT))
		{
			isInt = MG_FALSE;
			result.f = (float) result.i;
		}

		switch (item->type)
		{
		case MG_VALUE_INTEGER:
			if (isInt)
				result.i += item->data.i;
			else
				result.f += (float) item->data.i;
			break;
		case MG_VALUE_FLOAT:
			MG_ASSERT(!isInt);
			result.f += item->data.f;
			break;
		default:
			mgFatalError("Error: sum expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
			        i + 1, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
			        _MG_VALUE_TYPE_NAMES[item->type]);
		}
	}

	return isInt ? mgCreateValueInteger(result.i) : mgCreateValueFloat(result.f);
}


static MGValue* mg_random(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	if (argc != 0)
		mgFatalError("Error: random expects exactly 0 arguments, received %zu", argc);

	return mgCreateValueFloat((float) rand() / (float) RAND_MAX);
}


static MGValue* mg_seed(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	if (argc != 1)
		mgFatalError("Error: seed expects exactly 1 argument, received %zu", argc);
	else if (argv[0]->type != MG_VALUE_INTEGER)
		mgFatalError("Error: seed expected argument %zu as \"%s\", received \"%s\"",
		        1, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[argv[0]->type]);

	srand((unsigned int) argv[0]->data.i);

	return mgReferenceValue(argv[0]);
}


static MGValue* mg_normalize(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	const MGValue *value = argv[0];
	const MGValue *min = (argc > 2) ? argv[1] : NULL;
	const MGValue *max = (argc > 2) ? argv[2] : argv[1];

	if (argc < 2)
		mgFatalError("Error: normalize expected at least 2 arguments, received %zu", argc);
	else if (argc > 3)
		mgFatalError("Error: normalize expected at most 3 arguments, received %zu", argc);
	else if ((value->type != MG_VALUE_INTEGER) && (value->type != MG_VALUE_FLOAT))
		mgFatalError("Error: normalize expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
		        1, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT], _MG_VALUE_TYPE_NAMES[value->type]);
	else if (min && (min->type != MG_VALUE_INTEGER) && (min->type != MG_VALUE_FLOAT))
		mgFatalError("Error: normalize expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
		        2, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT], _MG_VALUE_TYPE_NAMES[min->type]);
	else if ((max->type != MG_VALUE_INTEGER) && (max->type != MG_VALUE_FLOAT))
		mgFatalError("Error: normalize expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
		        min ? 3 : 2, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT], _MG_VALUE_TYPE_NAMES[max->type]);

	float _value = (value->type == MG_VALUE_INTEGER) ? (float) value->data.i : value->data.f;
	float _min = min ? ((min->type == MG_VALUE_INTEGER) ? (float) min->data.i : min->data.f) : 0.0f;
	float _max = (max->type == MG_VALUE_INTEGER) ? (float) max->data.i : max->data.f;

	return mgCreateValueFloat((_value - _min) / (_max - _min));
}


static MGValue* mg_lerp(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	const MGValue *a = argv[0];
	const MGValue *b = argv[1];
	const MGValue *t = argv[2];

	if (argc != 3)
		mgFatalError("Error: lerp expects exactly 3 arguments, received %zu", argc);
	else if ((a->type != MG_VALUE_INTEGER) && (a->type != MG_VALUE_FLOAT))
		mgFatalError("Error: lerp expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
		        1, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT], _MG_VALUE_TYPE_NAMES[a->type]);
	else if ((b->type != MG_VALUE_INTEGER) && (b->type != MG_VALUE_FLOAT))
		mgFatalError("Error: lerp expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
		        2, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT], _MG_VALUE_TYPE_NAMES[b->type]);
	else if ((t->type != MG_VALUE_INTEGER) && (t->type != MG_VALUE_FLOAT))
		mgFatalError("Error: lerp expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
		        3, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT], _MG_VALUE_TYPE_NAMES[t->type]);

	float _a = (a->type == MG_VALUE_INTEGER) ? (float) a->data.i : a->data.f;
	float _b = (b->type == MG_VALUE_INTEGER) ? (float) b->data.i : b->data.f;
	float _t = (t->type == MG_VALUE_INTEGER) ? (float) t->data.i : t->data.f;

	return mgCreateValueFloat((1.0f - _t) * _a + _t * _b);
}


static MGValue* mg_map(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	const MGValue *value = argv[0];
	const MGValue *min1 = argv[1];
	const MGValue *max1 = argv[2];
	const MGValue *min2 = argv[3];
	const MGValue *max2 = argv[4];

	if (argc != 5)
		mgFatalError("Error: map expects exactly 5 arguments, received %zu", argc);
	else if ((value->type != MG_VALUE_INTEGER) && (value->type != MG_VALUE_FLOAT))
		mgFatalError("Error: map expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
		        1, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT], _MG_VALUE_TYPE_NAMES[value->type]);
	else if ((min1->type != MG_VALUE_INTEGER) && (min1->type != MG_VALUE_FLOAT))
		mgFatalError("Error: map expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
		        2, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT], _MG_VALUE_TYPE_NAMES[min1->type]);
	else if ((max1->type != MG_VALUE_INTEGER) && (max1->type != MG_VALUE_FLOAT))
		mgFatalError("Error: map expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
		        3, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT], _MG_VALUE_TYPE_NAMES[max1->type]);
	else if ((min2->type != MG_VALUE_INTEGER) && (min2->type != MG_VALUE_FLOAT))
		mgFatalError("Error: map expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
		        4, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT], _MG_VALUE_TYPE_NAMES[min2->type]);
	else if ((max2->type != MG_VALUE_INTEGER) && (max2->type != MG_VALUE_FLOAT))
		mgFatalError("Error: map expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
		        5, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT], _MG_VALUE_TYPE_NAMES[max2->type]);

	float _value = (value->type == MG_VALUE_INTEGER) ? (float) value->data.i : value->data.f;
	float _min1 = (min1->type == MG_VALUE_INTEGER) ? (float) min1->data.i : min1->data.f;
	float _max1 = (max1->type == MG_VALUE_INTEGER) ? (float) max1->data.i : max1->data.f;
	float _min2 = (min2->type == MG_VALUE_INTEGER) ? (float) min2->data.i : min2->data.f;
	float _max2 = (max2->type == MG_VALUE_INTEGER) ? (float) max2->data.i : max2->data.f;

	return mgCreateValueFloat(_min2 + (_max2 - _min2) * ((_value - _min1) / (_max1 - _min1)));
}


static MGValue* mg_nearest(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	const MGValue *value = argv[0];
	const MGValue *a = argv[1];
	const MGValue *b = argv[2];

	if (argc != 3)
		mgFatalError("Error: nearest expects exactly 3 arguments, received %zu", argc);
	else if ((value->type != MG_VALUE_INTEGER) && (value->type != MG_VALUE_FLOAT))
		mgFatalError("Error: nearest expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
		        1, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT], _MG_VALUE_TYPE_NAMES[value->type]);
	else if ((a->type != MG_VALUE_INTEGER) && (a->type != MG_VALUE_FLOAT))
		mgFatalError("Error: nearest expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
		        2, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT], _MG_VALUE_TYPE_NAMES[a->type]);
	else if ((b->type != MG_VALUE_INTEGER) && (b->type != MG_VALUE_FLOAT))
		mgFatalError("Error: nearest expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
		        3, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT], _MG_VALUE_TYPE_NAMES[b->type]);

	if ((value->type == MG_VALUE_FLOAT) || (a->type == MG_VALUE_FLOAT) || (b->type == MG_VALUE_FLOAT))
	{
		float _value = (value->type == MG_VALUE_INTEGER) ? (float) value->data.i : value->data.f;
		float _a = (a->type == MG_VALUE_INTEGER) ? (float) a->data.i : a->data.f;
		float _b = (b->type == MG_VALUE_INTEGER) ? (float) b->data.i : b->data.f;

		return mgCreateValueFloat((fabsf(_a - _value) > fabsf(_b - _value)) ? _b : _a);
	}
	else
	{
		int _value = value->data.i;
		int _a = a->data.i;
		int _b = b->data.i;

		return mgCreateValueInteger((abs(_a - _value) > abs(_b - _value)) ? _b : _a);
	}
}


static MGValue* mg_snap(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	const MGValue *value = argv[0];
	const MGValue *n = argv[1];
	const MGValue *offset = (argc > 2) ? argv[2] : NULL;

	if (argc < 2)
		mgFatalError("Error: snap expected at least 2 arguments, received %zu", argc);
	else if (argc > 3)
		mgFatalError("Error: snap expected at most 3 arguments, received %zu", argc);
	else if ((value->type != MG_VALUE_INTEGER) && (value->type != MG_VALUE_FLOAT))
		mgFatalError("Error: snap expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
		        1, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT], _MG_VALUE_TYPE_NAMES[value->type]);
	else if ((n->type != MG_VALUE_INTEGER) && (n->type != MG_VALUE_FLOAT))
		mgFatalError("Error: snap expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
		        2, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT], _MG_VALUE_TYPE_NAMES[n->type]);
	else if (offset && (offset->type != MG_VALUE_INTEGER) && (offset->type != MG_VALUE_FLOAT))
		mgFatalError("Error: snap expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
		        3, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT], _MG_VALUE_TYPE_NAMES[offset->type]);

	float _value = (value->type == MG_VALUE_INTEGER) ? (float) value->data.i : value->data.f;
	float _n = (n->type == MG_VALUE_INTEGER) ? (float) n->data.i : n->data.f;
	float _offset = offset ? ((offset->type == MG_VALUE_INTEGER) ? (float) offset->data.i : offset->data.f) : 0.0f;

	return mgCreateValueFloat(roundf((_value - _offset) / _n) * _n + _offset);
}


static MGValue* mg_snap_ceil(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	const MGValue *value = argv[0];
	const MGValue *n = argv[1];
	const MGValue *offset = (argc > 2) ? argv[2] : NULL;

	if (argc < 2)
		mgFatalError("Error: snap_ceil expected at least 2 arguments, received %zu", argc);
	else if (argc > 3)
		mgFatalError("Error: snap_ceil expected at most 3 arguments, received %zu", argc);
	else if ((value->type != MG_VALUE_INTEGER) && (value->type != MG_VALUE_FLOAT))
		mgFatalError("Error: snap_ceil expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
		        1, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT], _MG_VALUE_TYPE_NAMES[value->type]);
	else if ((n->type != MG_VALUE_INTEGER) && (n->type != MG_VALUE_FLOAT))
		mgFatalError("Error: snap_ceil expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
		        2, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT], _MG_VALUE_TYPE_NAMES[n->type]);
	else if (offset && (offset->type != MG_VALUE_INTEGER) && (offset->type != MG_VALUE_FLOAT))
		mgFatalError("Error: snap_ceil expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
		        3, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT], _MG_VALUE_TYPE_NAMES[offset->type]);

	float _value = (value->type == MG_VALUE_INTEGER) ? (float) value->data.i : value->data.f;
	float _n = (n->type == MG_VALUE_INTEGER) ? (float) n->data.i : n->data.f;
	float _offset = offset ? ((offset->type == MG_VALUE_INTEGER) ? (float) offset->data.i : offset->data.f) : 0.0f;

	return mgCreateValueFloat(ceilf((_value - _offset) / _n) * _n + _offset);
}


static MGValue* mg_snap_floor(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	const MGValue *value = argv[0];
	const MGValue *n = argv[1];
	const MGValue *offset = (argc > 2) ? argv[2] : NULL;

	if (argc < 2)
		mgFatalError("Error: snap_floor expected at least 2 arguments, received %zu", argc);
	else if (argc > 3)
		mgFatalError("Error: snap_floor expected at most 3 arguments, received %zu", argc);
	else if ((value->type != MG_VALUE_INTEGER) && (value->type != MG_VALUE_FLOAT))
		mgFatalError("Error: snap_floor expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
		        1, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT], _MG_VALUE_TYPE_NAMES[value->type]);
	else if ((n->type != MG_VALUE_INTEGER) && (n->type != MG_VALUE_FLOAT))
		mgFatalError("Error: snap_floor expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
		        2, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT], _MG_VALUE_TYPE_NAMES[n->type]);
	else if (offset && (offset->type != MG_VALUE_INTEGER) && (offset->type != MG_VALUE_FLOAT))
		mgFatalError("Error: snap_floor expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
		        3, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT], _MG_VALUE_TYPE_NAMES[offset->type]);

	float _value = (value->type == MG_VALUE_INTEGER) ? (float) value->data.i : value->data.f;
	float _n = (n->type == MG_VALUE_INTEGER) ? (float) n->data.i : n->data.f;
	float _offset = offset ? ((offset->type == MG_VALUE_INTEGER) ? (float) offset->data.i : offset->data.f) : 0.0f;

	return mgCreateValueFloat(floorf((_value - _offset) / _n) * _n + _offset);
}


static MGValue* mg_snap_within(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	const MGValue *value = argv[0];
	const MGValue *n = argv[1];
	const MGValue *within = argv[2];
	const MGValue *offset = (argc > 3) ? argv[3] : NULL;

	if (argc < 3)
		mgFatalError("Error: snap_within expected at least 3 arguments, received %zu", argc);
	else if (argc > 4)
		mgFatalError("Error: snap_within expected at most 4 arguments, received %zu", argc);
	else if ((value->type != MG_VALUE_INTEGER) && (value->type != MG_VALUE_FLOAT))
		mgFatalError("Error: snap_within expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
		        1, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT], _MG_VALUE_TYPE_NAMES[value->type]);
	else if ((n->type != MG_VALUE_INTEGER) && (n->type != MG_VALUE_FLOAT))
		mgFatalError("Error: snap_within expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
		        2, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT], _MG_VALUE_TYPE_NAMES[n->type]);
	else if ((within->type != MG_VALUE_INTEGER) && (within->type != MG_VALUE_FLOAT))
		mgFatalError("Error: snap_within expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
		        3, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT], _MG_VALUE_TYPE_NAMES[within->type]);
	else if (offset && (offset->type != MG_VALUE_INTEGER) && (offset->type != MG_VALUE_FLOAT))
		mgFatalError("Error: snap_within expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
		        4, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT], _MG_VALUE_TYPE_NAMES[offset->type]);

	float _value = (value->type == MG_VALUE_INTEGER) ? (float) value->data.i : value->data.f;
	float _n = (n->type == MG_VALUE_INTEGER) ? (float) n->data.i : n->data.f;
	float _within = (within->type == MG_VALUE_INTEGER) ? (float) within->data.i : within->data.f;
	float _offset = offset ? ((offset->type == MG_VALUE_INTEGER) ? (float) offset->data.i : offset->data.f) : 0.0f;
	float snapped = roundf((_value - _offset) / _n) * _n + _offset;

	return mgCreateValueFloat((fabsf(_value - snapped) > _within) ? _value : snapped);
}


// Wrap between min <= value < max
static MGValue* mg_wrap(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	const MGValue *value = argv[0];
	const MGValue *min = (argc > 2) ? argv[1] : NULL;
	const MGValue *max = (argc > 2) ? argv[2] : argv[1];

	if (argc < 2)
		mgFatalError("Error: wrap expected at least 2 arguments, received %zu", argc);
	else if (argc > 3)
		mgFatalError("Error: wrap expected at most 3 arguments, received %zu", argc);
	else if ((value->type != MG_VALUE_INTEGER) && (value->type != MG_VALUE_FLOAT))
		mgFatalError("Error: wrap expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
		        1, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT], _MG_VALUE_TYPE_NAMES[value->type]);
	else if (min && (min->type != MG_VALUE_INTEGER) && (min->type != MG_VALUE_FLOAT))
		mgFatalError("Error: wrap expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
		        2, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT], _MG_VALUE_TYPE_NAMES[min->type]);
	else if ((max->type != MG_VALUE_INTEGER) && (max->type != MG_VALUE_FLOAT))
		mgFatalError("Error: wrap expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
		        min ? 3 : 2, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT], _MG_VALUE_TYPE_NAMES[max->type]);

	if ((value->type == MG_VALUE_FLOAT) || (min && (min->type == MG_VALUE_FLOAT)) || (max->type == MG_VALUE_FLOAT))
	{
		float _value = (value->type == MG_VALUE_INTEGER) ? (float) value->data.i : value->data.f;
		float _min = min ? ((min->type == MG_VALUE_INTEGER) ? (float) min->data.i : min->data.f) : 0.0f;
		float _max = (max->type == MG_VALUE_INTEGER) ? (float) max->data.i : max->data.f;

		float length = _max - _min;
		float wrapped = fmodf(_value, length);
		wrapped = _min + ((wrapped < 0.0f) ? (length + wrapped) : wrapped);

		return mgCreateValueFloat(wrapped);
	}
	else
	{
		int _value = value->data.i;
		int _min = min ? min->data.i : 0;
		int _max = max->data.i;

		int length = _max - _min;
		int wrapped = _value % length;
		wrapped = _min + ((wrapped < 0) ? (length + wrapped) : wrapped);

		return mgCreateValueInteger(wrapped);
	}
}


// Oscillates between min <= value <= max
static MGValue* mg_ping_pong(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	const MGValue *value = argv[0];
	const MGValue *min = (argc > 2) ? argv[1] : NULL;
	const MGValue *max = (argc > 2) ? argv[2] : argv[1];

	if (argc < 2)
		mgFatalError("Error: ping_pong expected at least 2 arguments, received %zu", argc);
	else if (argc > 3)
		mgFatalError("Error: ping_pong expected at most 3 arguments, received %zu", argc);
	else if ((value->type != MG_VALUE_INTEGER) && (value->type != MG_VALUE_FLOAT))
		mgFatalError("Error: ping_pong expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
		        1, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT], _MG_VALUE_TYPE_NAMES[value->type]);
	else if (min && (min->type != MG_VALUE_INTEGER) && (min->type != MG_VALUE_FLOAT))
		mgFatalError("Error: ping_pong expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
		        2, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT], _MG_VALUE_TYPE_NAMES[min->type]);
	else if ((max->type != MG_VALUE_INTEGER) && (max->type != MG_VALUE_FLOAT))
		mgFatalError("Error: ping_pong expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
		        min ? 3 : 2, _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT], _MG_VALUE_TYPE_NAMES[max->type]);

	if ((value->type == MG_VALUE_FLOAT) || (min && (min->type == MG_VALUE_FLOAT)) || (max->type == MG_VALUE_FLOAT))
	{
		float _value = (value->type == MG_VALUE_INTEGER) ? (float) value->data.i : value->data.f;
		float _min = min ? ((min->type == MG_VALUE_INTEGER) ? (float) min->data.i : min->data.f) : 0.0f;
		float _max = (max->type == MG_VALUE_INTEGER) ? (float) max->data.i : max->data.f;

		float length = _max - _min;
		float pingPonged = fmodf(_value, (length * 2.0f));
		pingPonged = (pingPonged < 0.0f) ? -pingPonged : pingPonged;
		pingPonged = _min + ((pingPonged >= length) ? ((length * 2.0f) - pingPonged) : pingPonged);

		return mgCreateValueFloat(pingPonged);
	}
	else
	{
		int _value = value->data.i;
		int _min = min ? min->data.i : 0;
		int _max = max->data.i;

		int length = _max - _min;
		int pingPonged = _value % (length * 2);
		pingPonged = (pingPonged < 0) ? -pingPonged : pingPonged;
		pingPonged = _min + ((pingPonged >= length) ? ((length * 2) - pingPonged) : pingPonged);

		return mgCreateValueInteger(pingPonged);
	}
}


MGValue* mgCreateMathLib(void)
{
	MGValue *module = mgCreateValueModule();

	MG_ASSERT(module);
	MG_ASSERT(module->type == MG_VALUE_MODULE);

	mgModuleSetFloat(module, "inf", INFINITY);
	mgModuleSetFloat(module, "nan", NAN);
	mgModuleSetFloat(module, "pi", _MG_PI);
	mgModuleSetFloat(module, "tau", _MG_TAU);

	mgModuleSetInteger(module, "int_max", INT_MAX);
	mgModuleSetInteger(module, "int_min", INT_MIN);

	mgModuleSetCFunction(module, "abs", mg_abs);

	mgModuleSetCFunction(module, "deg", mg_deg);
	mgModuleSetCFunction(module, "rad", mg_rad);

	mgModuleSetCFunction(module, "sign", mg_sign);

	mgModuleSetCFunction(module, "even", mg_even);
	mgModuleSetCFunction(module, "odd", mg_odd);

	mgModuleSetCFunction(module, "multiple", mg_multiple);

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
	mgModuleSetCFunction(module, "clamp", mg_clamp); // clamp(value, min, max)

	mgModuleSetCFunction(module, "sum", mg_sum); // sum(iterable)

	mgModuleSetCFunction(module, "random", mg_random); // random(): float
	mgModuleSetCFunction(module, "seed", mg_seed); // seed(seed: int)

	mgModuleSetCFunction(module, "normalize", mg_normalize); // normalize(value, [min = 0,] max)

	mgModuleSetCFunction(module, "lerp", mg_lerp); // lerp(a, b, t)
	mgModuleSetCFunction(module, "map", mg_map); // map(value, min1, max1, min2, max2)

	mgModuleSetCFunction(module, "nearest", mg_nearest); // nearest(value, a, b)

	mgModuleSetCFunction(module, "snap", mg_snap); // snap(value, n [, offset = 0])
	mgModuleSetCFunction(module, "snap_ceil", mg_snap_ceil); // snap_ceil(value, n [, offset = 0])
	mgModuleSetCFunction(module, "snap_floor", mg_snap_floor); // snap_floor(value, n [, offset = 0])
	mgModuleSetCFunction(module, "snap_within", mg_snap_within); // snap_within(value, n, within [, offset = 0])

	mgModuleSetCFunction(module, "wrap", mg_wrap); // wrap(value, [min = 0,] max)
	mgModuleSetCFunction(module, "ping_pong", mg_ping_pong); // ping_pong(value, [min = 0,] max)

	return module;
}
