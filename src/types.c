
#include <stdio.h>

#include "modelgen.h"
#include "value.h"
#include "module.h"
#include "types.h"
#include "utilities.h"


extern MGNode* mgReferenceNode(const MGNode *node);


void mgAnyCopy(MGValue *copy, const MGValue *value)
{
	switch (copy->type)
	{
	case MG_TYPE_STRING:
		if (value->data.str.usage != MG_STRING_USAGE_STATIC)
			copy->data.str.s = mgStringDuplicate(value->data.str.s);
		break;
	case MG_TYPE_TUPLE:
	case MG_TYPE_LIST:
		if (mgListLength(value))
		{
			_mgListCreate(MGValue*, copy->data.a, mgListCapacity(value));
			for (size_t i = 0; i < mgListLength(value); ++i)
				_mgListAdd(MGValue*, copy->data.a, mgDeepCopyValue(_mgListGet(value->data.a, i)));
		}
		else if (mgListCapacity(value))
			_mgListInitialize(copy->data.a);
		break;
	case MG_TYPE_MAP:
		if (_mgMapSize(value->data.m))
		{
			MGMapIterator iterator;
			mgCreateMapIterator(&iterator, value);

			const MGValue *k, *v;
			while (mgMapNext(&iterator, &k, &v))
				mgMapSet(copy, k->data.str.s, mgDeepCopyValue(v));

			mgDestroyMapIterator(&iterator);
		}
		else
			_mgCreateMap(&copy->data.m, 0);
		break;
	case MG_TYPE_PROCEDURE:
	case MG_TYPE_FUNCTION:
		copy->data.func.module = mgReferenceValue(value->data.func.module);
		copy->data.func.node = mgReferenceNode(value->data.func.node);
		if (value->data.func.locals)
			copy->data.func.locals = mgDeepCopyValue(value->data.func.locals);
		break;
	default:
		break;
	}
}


void mgAnyDestroy(MGValue *value)
{
	switch (value->type)
	{
	case MG_TYPE_STRING:
		if (value->data.str.usage != MG_STRING_USAGE_STATIC)
			free(value->data.str.s);
		break;
	case MG_TYPE_TUPLE:
	case MG_TYPE_LIST:
		for (size_t i = 0; i < _mgListLength(value->data.a); ++i)
			mgDestroyValue(_mgListGet(value->data.a, i));
		_mgListDestroy(value->data.a);
		break;
	case MG_TYPE_MAP:
		_mgDestroyMap(&value->data.m);
		break;
	case MG_TYPE_MODULE:
		MG_ASSERT(value->data.module.globals);
		mgDestroyParser(&value->data.module.parser);
		free(value->data.module.filename);
		mgDestroyValue(value->data.module.globals);
		break;
	case MG_TYPE_PROCEDURE:
	case MG_TYPE_FUNCTION:
		mgDestroyValue(value->data.func.module);
		mgDestroyNode(value->data.func.node);
		if (value->data.func.locals)
			mgDestroyValue(value->data.func.locals);
		break;
	default:
		break;
	}
}


MGbool mgAnyTruthValue(const MGValue *value)
{
	switch (value->type)
	{
	case MG_TYPE_NULL:
		return MG_FALSE;
	case MG_TYPE_INTEGER:
		return (MGbool) (value->data.i != 0);
	case MG_TYPE_FLOAT:
		return (MGbool) !_MG_FEQUAL(value->data.f, 0.0f);
	case MG_TYPE_STRING:
		return (MGbool) (mgStringLength(value) != 0);
	case MG_TYPE_TUPLE:
		return (MGbool) (mgTupleLength(value) > 0);
	case MG_TYPE_LIST:
		return (MGbool) (mgListLength(value) > 0);
	case MG_TYPE_MAP:
		return (MGbool) (mgMapSize(value) > 0);
	default:
		return MG_TRUE;
	}
}


char* mgAnyToString(const MGValue *value)
{
	char *s, *end, *s2;
	size_t len, len2;

	switch (value->type)
	{
	case MG_TYPE_NULL:
		return mgStringDuplicateEx("null", 4);
	case MG_TYPE_INTEGER:
		return mgIntToString(value->data.i);
	case MG_TYPE_FLOAT:
		return mgFloatToString(value->data.f);
	case MG_TYPE_STRING:
		return mgStringDuplicateEx(value->data.str.s, value->data.str.length);
	case MG_TYPE_TUPLE:
	case MG_TYPE_LIST:
		len = 2;
		s = (char*) malloc((len + 1) * sizeof(char));
		end = s;

		*end++ = (char) ((value->type == MG_TYPE_TUPLE) ? '(' : '[');

		for (size_t i = 0; i < _mgListLength(value->data.a); ++i)
		{
			s2 = mgAnyToString(_mgListGet(value->data.a, i));
			MG_ASSERT(s2);

			len2 = strlen(s2);
			len += len2 + ((i > 0) ? 2 : 0);
			len += ((_mgListGet(value->data.a, i)->type == MG_TYPE_STRING) ? 2 : 0);

			end = end - (size_t) s;
			s = realloc(s, (len + 1) * sizeof(char));
			end = s + (size_t) end;

			if (i > 0)
				*end++ = ',', *end++ = ' ';

			if (_mgListGet(value->data.a, i)->type == MG_TYPE_STRING)
				*end++ = '"';

			strcpy(end, s2);
			end += len2;

			if (_mgListGet(value->data.a, i)->type == MG_TYPE_STRING)
				*end++ = '"';

			free(s2);
		}

		*end++ = (char) ((value->type == MG_TYPE_TUPLE) ? ')' : ']');
		*end = '\0';

		return s;
	case MG_TYPE_MAP:
		len = 2;
		s = (char*) malloc((len + 1) * sizeof(char));
		end = s;

		*end++ = '{';

		MGMapIterator iterator;
		mgCreateMapIterator(&iterator, value);

		const MGValue *k, *v;
		while (mgMapNext(&iterator, &k, &v))
		{
			const MGValue *values[2] = { k, v };

			for (int i = 0; i < 2; ++i)
			{
				const MGValue *value2 = values[i];

				s2 = mgAnyToString(value2);
				MG_ASSERT(s2);
				len2 = strlen(s2);
				len += len2 + (((i == 0) && ((end - s) > 1)) ? 2 : 0);
				len += 2 + ((value2->type == MG_TYPE_STRING) ? 2 : 0);

				end = end - (size_t) s;
				s = realloc(s, (len + 1) * sizeof(char));
				end = s + (size_t) end;

				if ((i == 0) && ((end - s) > 1))
					*end++ = ',', *end++ = ' ';

				if (value2->type == MG_TYPE_STRING)
					*end++ = '"';

				strcpy(end, s2);
				end += len2;

				if (value2->type == MG_TYPE_STRING)
					*end++ = '"';

				if (i == 0)
					*end++ = ':', *end++ = ' ';

				free(s2);
			}
		}

		mgDestroyMapIterator(&iterator);

		*end++ = '}';
		*end = '\0';

		return s;
	default:
		return NULL;
	}
}


MGValue* mgAnyPositive(const MGValue *operand)
{
	switch (operand->type)
	{
	case MG_TYPE_INTEGER:
		return mgCreateValueInteger(+operand->data.i);
	case MG_TYPE_FLOAT:
		return mgCreateValueFloat(+operand->data.f);
	default:
		return NULL;
	}
}


MGValue* mgAnyNegative(const MGValue *operand)
{
	switch (operand->type)
	{
	case MG_TYPE_INTEGER:
		return mgCreateValueInteger(-operand->data.i);
	case MG_TYPE_FLOAT:
		return mgCreateValueFloat(-operand->data.f);
	default:
		return NULL;
	}
}


MGValue* mgAnyInverse(const MGValue *operand)
{
	return mgCreateValueInteger(!mgAnyTruthValue(operand));
}


const MGTypeData _mgTypes[] = {
	{
		"null",
		NULL,
		mgAnyCopy,
		mgAnyDestroy,
		mgAnyTruthValue,
		mgAnyToString,
		NULL,
		NULL,
		mgAnyInverse
	},
	{
		"int",
		NULL,
		mgAnyCopy,
		mgAnyDestroy,
		mgAnyTruthValue,
		mgAnyToString,
		mgAnyPositive,
		mgAnyNegative,
		mgAnyInverse
	},
	{
		"float",
		NULL,
		mgAnyCopy,
		mgAnyDestroy,
		mgAnyTruthValue,
		mgAnyToString,
		mgAnyPositive,
		mgAnyNegative,
		mgAnyInverse
	},
	{
		"string",
		NULL,
		mgAnyCopy,
		mgAnyDestroy,
		mgAnyTruthValue,
		mgAnyToString,
		NULL,
		NULL,
		mgAnyInverse
	},
	{
		"tuple",
		NULL,
		mgAnyCopy,
		mgAnyDestroy,
		mgAnyTruthValue,
		mgAnyToString,
		NULL,
		NULL,
		mgAnyInverse
	},
	{
		"list",
		NULL,
		mgAnyCopy,
		mgAnyDestroy,
		mgAnyTruthValue,
		mgAnyToString,
		NULL,
		NULL,
		mgAnyInverse
	},
	{
		"map",
		NULL,
		mgAnyCopy,
		mgAnyDestroy,
		mgAnyTruthValue,
		mgAnyToString,
		NULL,
		NULL,
		mgAnyInverse
	},
	{
		"cfunc",
		NULL,
		mgAnyCopy,
		mgAnyDestroy,
		mgAnyTruthValue,
		mgAnyToString,
		NULL,
		NULL,
		mgAnyInverse
	},
	{
		"func",
		NULL,
		mgAnyCopy,
		mgAnyDestroy,
		mgAnyTruthValue,
		mgAnyToString,
		NULL,
		NULL,
		mgAnyInverse
	},
	{
		"proc",
		NULL,
		mgAnyCopy,
		mgAnyDestroy,
		mgAnyTruthValue,
		mgAnyToString,
		NULL,
		NULL,
		mgAnyInverse
	},
	{
		"module",
		NULL,
		mgAnyCopy,
		mgAnyDestroy,
		mgAnyTruthValue,
		mgAnyToString,
		NULL,
		NULL,
		mgAnyInverse
	}
};
