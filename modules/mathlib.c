
#include <stdarg.h>
#include <math.h>
#include <limits.h>

#include "value.h"
#include "module.h"
#include "callable.h"
#include "error.h"
#include "utilities.h"


#define _MG_PI  3.141592653589793238462643383279502884f
#define _MG_TAU 6.283185307179586476925286766559005768f

#define _MG_DEG2RAD (_MG_PI / 180.0f)
#define _MG_RAD2DEG (180.0f / _MG_PI)


static MGValue* mg_abs(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, 1);
	mgCheckArgumentTypes(instance, argc, argv, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT);

	switch (argv[0]->type)
	{
	case MG_TYPE_INTEGER:
		return mgCreateValueInteger((argv[0]->data.i < 0) ? -argv[0]->data.i : argv[0]->data.i);
	case MG_TYPE_FLOAT:
		return mgCreateValueFloat(fabsf(argv[0]->data.f));
	default:
		return MG_NULL_VALUE;
	}
}


static MGValue* mg_deg(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, 1);
	mgCheckArgumentTypes(instance, argc, argv, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT);

	switch (argv[0]->type)
	{
	case MG_TYPE_INTEGER:
		return mgCreateValueFloat(argv[0]->data.i * _MG_RAD2DEG);
	case MG_TYPE_FLOAT:
		return mgCreateValueFloat(argv[0]->data.f * _MG_RAD2DEG);
	default:
		return MG_NULL_VALUE;
	}
}


static MGValue* mg_rad(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, 1);
	mgCheckArgumentTypes(instance, argc, argv, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT);

	switch (argv[0]->type)
	{
	case MG_TYPE_INTEGER:
		return mgCreateValueFloat(argv[0]->data.i * _MG_DEG2RAD);
	case MG_TYPE_FLOAT:
		return mgCreateValueFloat(argv[0]->data.f * _MG_DEG2RAD);
	default:
		return MG_NULL_VALUE;
	}
}


static MGValue* mg_approximately(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 2, 3);
	mgCheckArgumentTypes(instance, argc, argv, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT);

	return mgCreateValueBoolean(MG_APPROXIMATELY(
			(argv[0]->type == MG_TYPE_INTEGER) ? argv[0]->data.i : argv[0]->data.f,
			(argv[1]->type == MG_TYPE_INTEGER) ? argv[1]->data.i : argv[1]->data.f,
			(argc > 2) ? ((argv[2]->type == MG_TYPE_INTEGER) ? argv[2]->data.i : argv[2]->data.f) : MG_EPSILON));
}


static MGValue* mg_sign(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, 1);
	mgCheckArgumentTypes(instance, argc, argv, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT);

	switch (argv[0]->type)
	{
	case MG_TYPE_INTEGER:
		return mgCreateValueInteger((0 < argv[0]->data.i) - (argv[0]->data.i < 0));
	case MG_TYPE_FLOAT:
		return mgCreateValueInteger((0.0f < argv[0]->data.f) - (argv[0]->data.f < 0.0f));
	default:
		return MG_NULL_VALUE;
	}
}


static MGValue* mg_even(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, 1);
	mgCheckArgumentTypes(instance, argc, argv, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT);

	switch (argv[0]->type)
	{
	case MG_TYPE_INTEGER:
		return mgCreateValueBoolean((argv[0]->data.i & 1) == 0);
	case MG_TYPE_FLOAT:
		return mgCreateValueBoolean(MG_FEQUAL(fmodf(argv[0]->data.f, 2.0f), 0.0f));
	default:
		return MG_NULL_VALUE;
	}
}


static MGValue* mg_odd(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, 1);
	mgCheckArgumentTypes(instance, argc, argv, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT);

	switch (argv[0]->type)
	{
	case MG_TYPE_INTEGER:
		return mgCreateValueBoolean((argv[0]->data.i & 1) == 1);
	case MG_TYPE_FLOAT:
		return mgCreateValueBoolean(MG_FEQUAL(fmodf(argv[0]->data.f, 2.0f), 1.0f));
	default:
		return MG_NULL_VALUE;
	}
}


// Check if b is a multiple of a
static MGValue* mg_multiple(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 2, 2);
	mgCheckArgumentTypes(instance, argc, argv, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT);

	switch (argv[0]->type)
	{
	case MG_TYPE_INTEGER:
		switch (argv[1]->type)
		{
		case MG_TYPE_INTEGER:
			return mgCreateValueBoolean((argv[1]->data.i % argv[0]->data.i) == 0);
		case MG_TYPE_FLOAT:
			return mgCreateValueBoolean(MG_FEQUAL(fmodf(argv[1]->data.f, (float) argv[0]->data.i), 0.0f));
		default:
			return MG_NULL_VALUE;
		}
	case MG_TYPE_FLOAT:
		switch (argv[1]->type)
		{
		case MG_TYPE_INTEGER:
			return mgCreateValueBoolean(MG_FEQUAL(fmodf((float) argv[1]->data.i, argv[0]->data.f), 0.0f));
		case MG_TYPE_FLOAT:
			return mgCreateValueBoolean(MG_FEQUAL(fmodf(argv[1]->data.f, argv[0]->data.f), 0.0f));
		default:
			return MG_NULL_VALUE;
		}
	default:
		return MG_NULL_VALUE;
	}
}


static MGValue* mg_ceil(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, 1);
	mgCheckArgumentTypes(instance, argc, argv, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT);

	switch (argv[0]->type)
	{
	case MG_TYPE_INTEGER:
		return mgCreateValueInteger(argv[0]->data.i);
	case MG_TYPE_FLOAT:
		return mgCreateValueFloat(ceilf(argv[0]->data.f));
	default:
		return MG_NULL_VALUE;
	}
}


static MGValue* mg_floor(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, 1);
	mgCheckArgumentTypes(instance, argc, argv, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT);

	switch (argv[0]->type)
	{
	case MG_TYPE_INTEGER:
		return mgCreateValueInteger(argv[0]->data.i);
	case MG_TYPE_FLOAT:
		return mgCreateValueFloat(floorf(argv[0]->data.f));
	default:
		return MG_NULL_VALUE;
	}
}


static MGValue* mg_round(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, 1);
	mgCheckArgumentTypes(instance, argc, argv, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT);

	switch (argv[0]->type)
	{
	case MG_TYPE_INTEGER:
		return mgCreateValueInteger(argv[0]->data.i);
	case MG_TYPE_FLOAT:
		return mgCreateValueFloat(roundf(argv[0]->data.f));
	default:
		return MG_NULL_VALUE;
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
	mgCheckArgumentCount(instance, argc, 2, 2);
	mgCheckArgumentTypes(instance, argc, argv, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT);

	if ((argv[0]->type == MG_TYPE_INTEGER) && (argv[1]->type == MG_TYPE_INTEGER) && (argv[1]->data.i >= 0))
		return mgCreateValueInteger(_mg_powi(argv[0]->data.i, (unsigned int) argv[1]->data.i));
	else
		return mgCreateValueFloat(powf(
				(argv[0]->type == MG_TYPE_INTEGER) ? (float) argv[0]->data.i : argv[0]->data.f,
				(argv[1]->type == MG_TYPE_INTEGER) ? (float) argv[1]->data.i : argv[1]->data.f));
}


static MGValue* mg_sqrt(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, 1);
	mgCheckArgumentTypes(instance, argc, argv, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT);

	switch (argv[0]->type)
	{
	case MG_TYPE_INTEGER:
		return mgCreateValueFloat(sqrtf((float) argv[0]->data.i));
	case MG_TYPE_FLOAT:
		return mgCreateValueFloat(sqrtf(argv[0]->data.f));
	default:
		return MG_NULL_VALUE;
	}
}


static MGValue* mg_cos(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, 1);
	mgCheckArgumentTypes(instance, argc, argv, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT);

	switch (argv[0]->type)
	{
	case MG_TYPE_INTEGER:
		return mgCreateValueFloat(cosf((float) argv[0]->data.i));
	case MG_TYPE_FLOAT:
		return mgCreateValueFloat(cosf(argv[0]->data.f));
	default:
		return MG_NULL_VALUE;
	}
}


static MGValue* mg_sin(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, 1);
	mgCheckArgumentTypes(instance, argc, argv, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT);

	switch (argv[0]->type)
	{
	case MG_TYPE_INTEGER:
		return mgCreateValueFloat(sinf((float) argv[0]->data.i));
	case MG_TYPE_FLOAT:
		return mgCreateValueFloat(sinf(argv[0]->data.f));
	default:
		return MG_NULL_VALUE;
	}
}


static MGValue* mg_tan(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, 1);
	mgCheckArgumentTypes(instance, argc, argv, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT);

	switch (argv[0]->type)
	{
	case MG_TYPE_INTEGER:
		return mgCreateValueFloat(tanf((float) argv[0]->data.i));
	case MG_TYPE_FLOAT:
		return mgCreateValueFloat(tanf(argv[0]->data.f));
	default:
		return MG_NULL_VALUE;
	}
}


static MGValue* mg_acos(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, 1);
	mgCheckArgumentTypes(instance, argc, argv, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT);

	switch (argv[0]->type)
	{
	case MG_TYPE_INTEGER:
		return mgCreateValueFloat(acosf((float) argv[0]->data.i));
	case MG_TYPE_FLOAT:
		return mgCreateValueFloat(acosf(argv[0]->data.f));
	default:
		return MG_NULL_VALUE;
	}
}


static MGValue* mg_asin(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, 1);
	mgCheckArgumentTypes(instance, argc, argv, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT);

	switch (argv[0]->type)
	{
	case MG_TYPE_INTEGER:
		return mgCreateValueFloat(asinf((float) argv[0]->data.i));
	case MG_TYPE_FLOAT:
		return mgCreateValueFloat(asinf(argv[0]->data.f));
	default:
		return MG_NULL_VALUE;
	}
}


static MGValue* mg_atan(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, 1);
	mgCheckArgumentTypes(instance, argc, argv, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT);

	switch (argv[0]->type)
	{
	case MG_TYPE_INTEGER:
		return mgCreateValueFloat(atanf((float) argv[0]->data.i));
	case MG_TYPE_FLOAT:
		return mgCreateValueFloat(atanf(argv[0]->data.f));
	default:
		return MG_NULL_VALUE;
	}
}


static MGValue* mg_atan2(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 2, 2);
	mgCheckArgumentTypes(instance, argc, argv, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT);

	return mgCreateValueFloat(atan2f(
			(argv[0]->type == MG_TYPE_INTEGER) ? (float) argv[0]->data.i : argv[0]->data.f,
			(argv[1]->type == MG_TYPE_INTEGER) ? (float) argv[1]->data.i : argv[1]->data.f));
}


static MGValue* mg_exp(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, 1);
	mgCheckArgumentTypes(instance, argc, argv, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT);

	switch (argv[0]->type)
	{
	case MG_TYPE_INTEGER:
		return mgCreateValueFloat(expf((float) argv[0]->data.i));
	case MG_TYPE_FLOAT:
		return mgCreateValueFloat(expf(argv[0]->data.f));
	default:
		return MG_NULL_VALUE;
	}
}


static MGValue* mg_log(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, 1);
	mgCheckArgumentTypes(instance, argc, argv, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT);

	switch (argv[0]->type)
	{
	case MG_TYPE_INTEGER:
		return mgCreateValueFloat(logf((float) argv[0]->data.i));
	case MG_TYPE_FLOAT:
		return mgCreateValueFloat(logf(argv[0]->data.f));
	default:
		return MG_NULL_VALUE;
	}
}


static MGValue* mg_log2(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, 1);
	mgCheckArgumentTypes(instance, argc, argv, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT);

	switch (argv[0]->type)
	{
	case MG_TYPE_INTEGER:
		return mgCreateValueFloat(log2f((float) argv[0]->data.i));
	case MG_TYPE_FLOAT:
		return mgCreateValueFloat(log2f(argv[0]->data.f));
	default:
		return MG_NULL_VALUE;
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
	case MG_TYPE_INTEGER:
		isInt = MG_TRUE;
		result.i = argv[0]->data.i;
		break;
	case MG_TYPE_FLOAT:
		isInt = MG_FALSE;
		result.f = argv[0]->data.f;
		break;
	default:
		mgFatalError("Error: max expected argument as \"%s\" or \"%s\", received \"%s\"",
		        mgGetTypeName(MG_TYPE_INTEGER), mgGetTypeName(MG_TYPE_FLOAT),
		        mgGetTypeName(argv[0]->type));
		return MG_NULL_VALUE;
	}

	for (size_t i = 1; i < argc; ++i)
	{
		if (isInt && (argv[i]->type == MG_TYPE_FLOAT))
		{
			isInt = MG_FALSE;
			result.f = (float) result.i;
		}

		switch (argv[i]->type)
		{
		case MG_TYPE_INTEGER:
			if (isInt)
				result.i = (argv[i]->data.i > result.i) ? argv[i]->data.i : result.i;
			else
				result.f = ((float) argv[i]->data.i > result.f) ? (float) argv[i]->data.i : result.f;
			break;
		case MG_TYPE_FLOAT:
			MG_ASSERT(!isInt);
			result.f = (argv[i]->data.f > result.f) ? argv[i]->data.f : result.f;
			break;
		default:
			mgFatalError("Error: max expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
			        i + 1, mgGetTypeName(MG_TYPE_INTEGER), mgGetTypeName(MG_TYPE_FLOAT),
			        mgGetTypeName(argv[i]->type));
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
	case MG_TYPE_INTEGER:
		isInt = MG_TRUE;
		result.i = argv[0]->data.i;
		break;
	case MG_TYPE_FLOAT:
		isInt = MG_FALSE;
		result.f = argv[0]->data.f;
		break;
	default:
		mgFatalError("Error: min expected argument as \"%s\" or \"%s\", received \"%s\"",
		        mgGetTypeName(MG_TYPE_INTEGER), mgGetTypeName(MG_TYPE_FLOAT),
		        mgGetTypeName(argv[0]->type));
		return MG_NULL_VALUE;
	}

	for (size_t i = 1; i < argc; ++i)
	{
		if (isInt && (argv[i]->type == MG_TYPE_FLOAT))
		{
			isInt = MG_FALSE;
			result.f = (float) result.i;
		}

		switch (argv[i]->type)
		{
		case MG_TYPE_INTEGER:
			if (isInt)
				result.i = (argv[i]->data.i < result.i) ? argv[i]->data.i : result.i;
			else
				result.f = ((float) argv[i]->data.i < result.f) ? (float) argv[i]->data.i : result.f;
			break;
		case MG_TYPE_FLOAT:
			MG_ASSERT(!isInt);
			result.f = (argv[i]->data.f < result.f) ? argv[i]->data.f : result.f;
			break;
		default:
			mgFatalError("Error: min expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
			        i + 1, mgGetTypeName(MG_TYPE_INTEGER), mgGetTypeName(MG_TYPE_FLOAT),
			        mgGetTypeName(argv[i]->type));
		}
	}

	return isInt ? mgCreateValueInteger(result.i) : mgCreateValueFloat(result.f);
}


static MGValue* mg_max(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, SIZE_MAX);

	if (argc < 1)
		mgFatalError("Error: max expected at least 1 argument, received %zu", argc);

	if (argc == 1)
	{
		if ((argv[0]->type != MG_TYPE_TUPLE) && (argv[0]->type != MG_TYPE_LIST))
			mgFatalError("Error: max expected argument %zu as \"%s\", received \"%s\"",
			        1, mgGetTypeName(MG_TYPE_LIST), mgGetTypeName(argv[0]->type));

		return _mg_max(mgListLength(argv[0]), (const MGValue* const*) mgListItems(argv[0]));
	}
	else
		return _mg_max(argc, argv);
}


static MGValue* mg_min(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, SIZE_MAX);

	if (argc == 1)
	{
		if ((argv[0]->type != MG_TYPE_TUPLE) && (argv[0]->type != MG_TYPE_LIST))
			mgFatalError("Error: min expected argument %zu as \"%s\", received \"%s\"",
			        1, mgGetTypeName(MG_TYPE_LIST), mgGetTypeName(argv[0]->type));

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

	mgCheckArgumentCount(instance, argc, 3, 3);
	mgCheckArgumentTypes(instance, argc, argv, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT);

	if ((value->type == MG_TYPE_FLOAT) || (min->type == MG_TYPE_FLOAT) || (max->type == MG_TYPE_FLOAT))
	{
		float result = (value->type == MG_TYPE_INTEGER) ? (float) value->data.i : value->data.f;
		result = (min->type == MG_TYPE_INTEGER) ? ((result < min->data.i) ? (float) min->data.i : result) : ((result < min->data.f) ? min->data.f : result);
		result = (max->type == MG_TYPE_INTEGER) ? ((result > max->data.i) ? (float) max->data.i : result) : ((result > max->data.f) ? max->data.f : result);

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

	mgCheckArgumentCount(instance, argc, 1, 1);
	mgCheckArgumentTypes(instance, argc, argv, 2, MG_TYPE_TUPLE, MG_TYPE_LIST);

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

		if (isInt && (item->type == MG_TYPE_FLOAT))
		{
			isInt = MG_FALSE;
			result.f = (float) result.i;
		}

		switch (item->type)
		{
		case MG_TYPE_INTEGER:
			if (isInt)
				result.i += item->data.i;
			else
				result.f += (float) item->data.i;
			break;
		case MG_TYPE_FLOAT:
			MG_ASSERT(!isInt);
			result.f += item->data.f;
			break;
		default:
			mgFatalError("Error: sum expected argument %zu as \"%s\" or \"%s\", received \"%s\"",
			        i + 1, mgGetTypeName(MG_TYPE_INTEGER), mgGetTypeName(MG_TYPE_FLOAT),
			        mgGetTypeName(item->type));
		}
	}

	return isInt ? mgCreateValueInteger(result.i) : mgCreateValueFloat(result.f);
}


static MGValue* mg_random(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 0, 0);

	return mgCreateValueFloat((float) rand() / (float) RAND_MAX);
}


static MGValue* mg_seed(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, 1);
	mgCheckArgumentTypes(instance, argc, argv, 1, MG_TYPE_INTEGER);

	srand((unsigned int) argv[0]->data.i);

	return mgReferenceValue(argv[0]);
}


static MGValue* mg_normalize(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	const MGValue *value = argv[0];
	const MGValue *min = (argc > 2) ? argv[1] : NULL;
	const MGValue *max = (argc > 2) ? argv[2] : argv[1];

	mgCheckArgumentCount(instance, argc, 2, 3);
	mgCheckArgumentTypes(instance, argc, argv, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT);

	float _value = (value->type == MG_TYPE_INTEGER) ? (float) value->data.i : value->data.f;
	float _min = min ? ((min->type == MG_TYPE_INTEGER) ? (float) min->data.i : min->data.f) : 0.0f;
	float _max = (max->type == MG_TYPE_INTEGER) ? (float) max->data.i : max->data.f;

	return mgCreateValueFloat((_value - _min) / (_max - _min));
}


static MGValue* mg_lerp(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	const MGValue *a = argv[0];
	const MGValue *b = argv[1];
	const MGValue *t = argv[2];

	mgCheckArgumentCount(instance, argc, 3, 3);
	mgCheckArgumentTypes(instance, argc, argv, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT);

	float _a = (a->type == MG_TYPE_INTEGER) ? (float) a->data.i : a->data.f;
	float _b = (b->type == MG_TYPE_INTEGER) ? (float) b->data.i : b->data.f;
	float _t = (t->type == MG_TYPE_INTEGER) ? (float) t->data.i : t->data.f;

	return mgCreateValueFloat((1.0f - _t) * _a + _t * _b);
}


static MGValue* mg_map(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	const MGValue *value = argv[0];
	const MGValue *min1 = argv[1];
	const MGValue *max1 = argv[2];
	const MGValue *min2 = argv[3];
	const MGValue *max2 = argv[4];

	mgCheckArgumentCount(instance, argc, 5, 5);
	mgCheckArgumentTypes(instance, argc, argv, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT);

	float _value = (value->type == MG_TYPE_INTEGER) ? (float) value->data.i : value->data.f;
	float _min1 = (min1->type == MG_TYPE_INTEGER) ? (float) min1->data.i : min1->data.f;
	float _max1 = (max1->type == MG_TYPE_INTEGER) ? (float) max1->data.i : max1->data.f;
	float _min2 = (min2->type == MG_TYPE_INTEGER) ? (float) min2->data.i : min2->data.f;
	float _max2 = (max2->type == MG_TYPE_INTEGER) ? (float) max2->data.i : max2->data.f;

	return mgCreateValueFloat(_min2 + (_max2 - _min2) * ((_value - _min1) / (_max1 - _min1)));
}


static MGValue* mg_nearest(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	const MGValue *value = argv[0];
	const MGValue *a = argv[1];
	const MGValue *b = argv[2];

	mgCheckArgumentCount(instance, argc, 3, 3);
	mgCheckArgumentTypes(instance, argc, argv, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT);

	if ((value->type == MG_TYPE_FLOAT) || (a->type == MG_TYPE_FLOAT) || (b->type == MG_TYPE_FLOAT))
	{
		float _value = (value->type == MG_TYPE_INTEGER) ? (float) value->data.i : value->data.f;
		float _a = (a->type == MG_TYPE_INTEGER) ? (float) a->data.i : a->data.f;
		float _b = (b->type == MG_TYPE_INTEGER) ? (float) b->data.i : b->data.f;

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

	mgCheckArgumentCount(instance, argc, 2, 3);
	mgCheckArgumentTypes(instance, argc, argv, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT);

	float _value = (value->type == MG_TYPE_INTEGER) ? (float) value->data.i : value->data.f;
	float _n = (n->type == MG_TYPE_INTEGER) ? (float) n->data.i : n->data.f;
	float _offset = offset ? ((offset->type == MG_TYPE_INTEGER) ? (float) offset->data.i : offset->data.f) : 0.0f;

	return mgCreateValueFloat(roundf((_value - _offset) / _n) * _n + _offset);
}


static MGValue* mg_snap_ceil(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	const MGValue *value = argv[0];
	const MGValue *n = argv[1];
	const MGValue *offset = (argc > 2) ? argv[2] : NULL;

	mgCheckArgumentCount(instance, argc, 2, 3);
	mgCheckArgumentTypes(instance, argc, argv, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT);

	float _value = (value->type == MG_TYPE_INTEGER) ? (float) value->data.i : value->data.f;
	float _n = (n->type == MG_TYPE_INTEGER) ? (float) n->data.i : n->data.f;
	float _offset = offset ? ((offset->type == MG_TYPE_INTEGER) ? (float) offset->data.i : offset->data.f) : 0.0f;

	return mgCreateValueFloat(ceilf((_value - _offset) / _n) * _n + _offset);
}


static MGValue* mg_snap_floor(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	const MGValue *value = argv[0];
	const MGValue *n = argv[1];
	const MGValue *offset = (argc > 2) ? argv[2] : NULL;

	mgCheckArgumentCount(instance, argc, 2, 3);
	mgCheckArgumentTypes(instance, argc, argv, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT);

	float _value = (value->type == MG_TYPE_INTEGER) ? (float) value->data.i : value->data.f;
	float _n = (n->type == MG_TYPE_INTEGER) ? (float) n->data.i : n->data.f;
	float _offset = offset ? ((offset->type == MG_TYPE_INTEGER) ? (float) offset->data.i : offset->data.f) : 0.0f;

	return mgCreateValueFloat(floorf((_value - _offset) / _n) * _n + _offset);
}


static MGValue* mg_snap_within(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	const MGValue *value = argv[0];
	const MGValue *n = argv[1];
	const MGValue *within = argv[2];
	const MGValue *offset = (argc > 3) ? argv[3] : NULL;

	mgCheckArgumentCount(instance, argc, 3, 4);
	mgCheckArgumentTypes(instance, argc, argv, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT);

	float _value = (value->type == MG_TYPE_INTEGER) ? (float) value->data.i : value->data.f;
	float _n = (n->type == MG_TYPE_INTEGER) ? (float) n->data.i : n->data.f;
	float _within = (within->type == MG_TYPE_INTEGER) ? (float) within->data.i : within->data.f;
	float _offset = offset ? ((offset->type == MG_TYPE_INTEGER) ? (float) offset->data.i : offset->data.f) : 0.0f;
	float snapped = roundf((_value - _offset) / _n) * _n + _offset;

	return mgCreateValueFloat((fabsf(_value - snapped) > _within) ? _value : snapped);
}


// Wrap between min <= value < max
static MGValue* mg_wrap(MGInstance *instance, size_t argc, const MGValue* const* argv)
{
	const MGValue *value = argv[0];
	const MGValue *min = (argc > 2) ? argv[1] : NULL;
	const MGValue *max = (argc > 2) ? argv[2] : argv[1];

	mgCheckArgumentCount(instance, argc, 2, 3);
	mgCheckArgumentTypes(instance, argc, argv, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT);

	if ((value->type == MG_TYPE_FLOAT) || (min && (min->type == MG_TYPE_FLOAT)) || (max->type == MG_TYPE_FLOAT))
	{
		float _value = (value->type == MG_TYPE_INTEGER) ? (float) value->data.i : value->data.f;
		float _min = min ? ((min->type == MG_TYPE_INTEGER) ? (float) min->data.i : min->data.f) : 0.0f;
		float _max = (max->type == MG_TYPE_INTEGER) ? (float) max->data.i : max->data.f;

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

	mgCheckArgumentCount(instance, argc, 2, 3);
	mgCheckArgumentTypes(instance, argc, argv, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT, 2, MG_TYPE_INTEGER, MG_TYPE_FLOAT);

	if ((value->type == MG_TYPE_FLOAT) || (min && (min->type == MG_TYPE_FLOAT)) || (max->type == MG_TYPE_FLOAT))
	{
		float _value = (value->type == MG_TYPE_INTEGER) ? (float) value->data.i : value->data.f;
		float _min = min ? ((min->type == MG_TYPE_INTEGER) ? (float) min->data.i : min->data.f) : 0.0f;
		float _max = (max->type == MG_TYPE_INTEGER) ? (float) max->data.i : max->data.f;

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
	MG_ASSERT(module->type == MG_TYPE_MODULE);

	mgModuleSetFloat(module, "epsilon", MG_EPSILON);
	mgModuleSetFloat(module, "inf", INFINITY);
	mgModuleSetFloat(module, "nan", NAN);
	mgModuleSetFloat(module, "pi", _MG_PI);
	mgModuleSetFloat(module, "tau", _MG_TAU);

	mgModuleSetInteger(module, "int_max", INT_MAX);
	mgModuleSetInteger(module, "int_min", INT_MIN);

	mgModuleSetCFunction(module, "abs", mg_abs);

	mgModuleSetCFunction(module, "deg", mg_deg);
	mgModuleSetCFunction(module, "rad", mg_rad);

	mgModuleSetCFunction(module, "approximately", mg_approximately);

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
