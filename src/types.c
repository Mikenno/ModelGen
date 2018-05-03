
#include <stdio.h>

#include "modelgen.h"
#include "value.h"
#include "module.h"
#include "types.h"
#include "error.h"
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
		return value->data.i != 0;
	case MG_TYPE_FLOAT:
		return !_MG_FEQUAL(value->data.f, 0.0f);
	case MG_TYPE_STRING:
		return mgStringLength(value) != 0;
	case MG_TYPE_TUPLE:
		return mgTupleLength(value) > 0;
	case MG_TYPE_LIST:
		return mgListLength(value) > 0;
	case MG_TYPE_MAP:
		return mgMapSize(value) > 0;
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


MGtribool mgAnyEqual(const MGValue *lhs, const MGValue *rhs)
{
	if (lhs == rhs)
		return MG_TRUE;
	else if ((lhs->type == MG_TYPE_NULL) || (rhs->type == MG_TYPE_NULL))
		return lhs->type == rhs->type;
	else if ((lhs->type == MG_TYPE_INTEGER) && (rhs->type == MG_TYPE_INTEGER))
		return lhs->data.i == rhs->data.i;
	else if ((lhs->type == MG_TYPE_FLOAT) && (rhs->type == MG_TYPE_INTEGER))
		return lhs->data.f == rhs->data.i;
	else if ((lhs->type == MG_TYPE_INTEGER) && (rhs->type == MG_TYPE_FLOAT))
		return lhs->data.i == rhs->data.f;
	else if ((lhs->type == MG_TYPE_FLOAT) && (rhs->type == MG_TYPE_FLOAT))
		return _MG_FEQUAL(lhs->data.f, rhs->data.f);
	else if ((lhs->type == MG_TYPE_STRING) && (rhs->type == MG_TYPE_STRING))
		return (lhs->data.str.s == rhs->data.str.s) || ((lhs->data.str.length == rhs->data.str.length) && !strcmp(lhs->data.str.s, rhs->data.str.s));
	else if (((lhs->type == MG_TYPE_TUPLE) || (lhs->type == MG_TYPE_LIST)) && (lhs->type == rhs->type))
	{
		if (mgListLength(lhs) != mgListLength(rhs))
			return MG_FALSE;

		for (size_t i = 0; i < mgListLength(lhs); ++i)
			if (!mgAnyEqual(_mgListGet(lhs->data.a, i), _mgListGet(rhs->data.a, i)))
				return MG_FALSE;

		return MG_TRUE;
	}
	else if ((lhs->type == MG_TYPE_MAP) && (rhs->type == MG_TYPE_MAP))
	{
		if (mgMapSize(lhs) != mgMapSize(rhs))
			return MG_FALSE;

		MGtribool result = MG_TRUE;

		MGMapIterator iterator;
		mgCreateMapIterator(&iterator, lhs);

		const MGValue *k, *v, *v2;
		while (mgMapNext(&iterator, &k, &v))
		{
			v2 = mgMapGet(rhs, k->data.str.s);

			if (!v2 || !mgAnyEqual(v, v2))
			{
				result = MG_FALSE;
				break;
			}
		}

		mgDestroyMapIterator(&iterator);

		return result;
	}

	return MG_INDETERMINATE;
}


MGtribool mgAnyLess(const MGValue *lhs, const MGValue *rhs)
{
	if (lhs == rhs)
		return MG_FALSE;
	else if ((lhs->type == MG_TYPE_INTEGER) && (rhs->type == MG_TYPE_INTEGER))
		return lhs->data.i < rhs->data.i;
	else if ((lhs->type == MG_TYPE_FLOAT) && (rhs->type == MG_TYPE_INTEGER))
		return lhs->data.f < rhs->data.i;
	else if ((lhs->type == MG_TYPE_INTEGER) && (rhs->type == MG_TYPE_FLOAT))
		return lhs->data.i < rhs->data.f;
	else if ((lhs->type == MG_TYPE_FLOAT) && (rhs->type == MG_TYPE_FLOAT))
		return lhs->data.f < rhs->data.f;
	return MG_INDETERMINATE;
}


MGtribool mgAnyLessEqual(const MGValue *lhs, const MGValue *rhs)
{
	if (lhs == rhs)
		return MG_TRUE;
	else if ((lhs->type == MG_TYPE_INTEGER) && (rhs->type == MG_TYPE_INTEGER))
		return lhs->data.i <= rhs->data.i;
	else if ((lhs->type == MG_TYPE_FLOAT) && (rhs->type == MG_TYPE_INTEGER))
		return lhs->data.f <= rhs->data.i;
	else if ((lhs->type == MG_TYPE_INTEGER) && (rhs->type == MG_TYPE_FLOAT))
		return lhs->data.i <= rhs->data.f;
	else if ((lhs->type == MG_TYPE_FLOAT) && (rhs->type == MG_TYPE_FLOAT))
		return lhs->data.f <= rhs->data.f;
	return MG_INDETERMINATE;
}


MGValue* mgIntAdd(const MGValue *lhs, const MGValue *rhs)
{
	if ((lhs->type == MG_TYPE_INTEGER) && (rhs->type == MG_TYPE_INTEGER))
		return mgCreateValueInteger(lhs->data.i + rhs->data.i);
	return NULL;
}


MGValue* mgIntSub(const MGValue *lhs, const MGValue *rhs)
{
	if ((lhs->type == MG_TYPE_INTEGER) && (rhs->type == MG_TYPE_INTEGER))
		return mgCreateValueInteger(lhs->data.i - rhs->data.i);
	return NULL;
}


MGValue* mgIntMul(const MGValue *lhs, const MGValue *rhs)
{
	if ((lhs->type == MG_TYPE_INTEGER) && (rhs->type == MG_TYPE_INTEGER))
		return mgCreateValueInteger(lhs->data.i * rhs->data.i);
	return NULL;
}


MGValue* mgIntDiv(const MGValue *lhs, const MGValue *rhs)
{
	if ((lhs->type != MG_TYPE_INTEGER) || (rhs->type != MG_TYPE_INTEGER))
		return NULL;
	return mgCreateValueFloat(lhs->data.i / (float) rhs->data.i);
}


MGValue* mgIntIntDiv(const MGValue *lhs, const MGValue *rhs)
{
	if ((lhs->type != MG_TYPE_INTEGER) || (rhs->type != MG_TYPE_INTEGER))
		return NULL;
	if (rhs->data.i == 0)
		mgFatalError("Error: Division by zero");
	return mgCreateValueInteger(lhs->data.i / rhs->data.i);
}


MGValue* mgIntMod(const MGValue *lhs, const MGValue *rhs)
{
	if ((lhs->type == MG_TYPE_INTEGER) && (rhs->type == MG_TYPE_INTEGER))
		return mgCreateValueInteger(lhs->data.i % rhs->data.i);
	return NULL;
}


MGValue* mgFloatAdd(const MGValue *lhs, const MGValue *rhs)
{
	if ((lhs->type == MG_TYPE_FLOAT) && (rhs->type == MG_TYPE_INTEGER))
		return mgCreateValueFloat(lhs->data.f + rhs->data.i);
	else if ((lhs->type == MG_TYPE_INTEGER) && (rhs->type == MG_TYPE_FLOAT))
		return mgCreateValueFloat(lhs->data.i + rhs->data.f);
	else if ((lhs->type == MG_TYPE_FLOAT) && (rhs->type == MG_TYPE_FLOAT))
		return mgCreateValueFloat(lhs->data.f + rhs->data.f);
	return NULL;
}


MGValue* mgFloatSub(const MGValue *lhs, const MGValue *rhs)
{
	if ((lhs->type == MG_TYPE_FLOAT) && (rhs->type == MG_TYPE_INTEGER))
		return mgCreateValueFloat(lhs->data.f - rhs->data.i);
	else if ((lhs->type == MG_TYPE_INTEGER) && (rhs->type == MG_TYPE_FLOAT))
		return mgCreateValueFloat(lhs->data.i - rhs->data.f);
	else if ((lhs->type == MG_TYPE_FLOAT) && (rhs->type == MG_TYPE_FLOAT))
		return mgCreateValueFloat(lhs->data.f - rhs->data.f);
	return NULL;
}


MGValue* mgFloatMul(const MGValue *lhs, const MGValue *rhs)
{
	if ((lhs->type == MG_TYPE_FLOAT) && (rhs->type == MG_TYPE_INTEGER))
		return mgCreateValueFloat(lhs->data.f * rhs->data.i);
	else if ((lhs->type == MG_TYPE_INTEGER) && (rhs->type == MG_TYPE_FLOAT))
		return mgCreateValueFloat(lhs->data.i * rhs->data.f);
	else if ((lhs->type == MG_TYPE_FLOAT) && (rhs->type == MG_TYPE_FLOAT))
		return mgCreateValueFloat(lhs->data.f * rhs->data.f);
	return NULL;
}


MGValue* mgFloatDiv(const MGValue *lhs, const MGValue *rhs)
{
	if ((lhs->type == MG_TYPE_FLOAT) && (rhs->type == MG_TYPE_INTEGER))
		return mgCreateValueFloat(lhs->data.f / rhs->data.i);
	else if ((lhs->type == MG_TYPE_INTEGER) && (rhs->type == MG_TYPE_FLOAT))
		return mgCreateValueFloat(lhs->data.i / rhs->data.f);
	else if ((lhs->type == MG_TYPE_FLOAT) && (rhs->type == MG_TYPE_FLOAT))
		return mgCreateValueFloat(lhs->data.f / rhs->data.f);
	return NULL;
}


MGValue* mgFloatIntDiv(const MGValue *lhs, const MGValue *rhs)
{
	if ((lhs->type == MG_TYPE_FLOAT) && (rhs->type == MG_TYPE_INTEGER))
		return mgCreateValueInteger((int) (lhs->data.f / rhs->data.i));
	else if ((lhs->type == MG_TYPE_INTEGER) && (rhs->type == MG_TYPE_FLOAT))
		return mgCreateValueInteger((int) (lhs->data.i / rhs->data.f));
	else if ((lhs->type == MG_TYPE_FLOAT) && (rhs->type == MG_TYPE_FLOAT))
		return mgCreateValueInteger((int) (lhs->data.f / rhs->data.f));
	return NULL;
}


MGValue* mgFloatMod(const MGValue *lhs, const MGValue *rhs)
{
	if ((lhs->type == MG_TYPE_FLOAT) && (rhs->type == MG_TYPE_INTEGER))
		return mgCreateValueFloat(fmodf(lhs->data.f, (float) rhs->data.i));
	else if ((lhs->type == MG_TYPE_INTEGER) && (rhs->type == MG_TYPE_FLOAT))
		return mgCreateValueFloat(fmodf((float) lhs->data.i, rhs->data.f));
	else if ((lhs->type == MG_TYPE_FLOAT) && (rhs->type == MG_TYPE_FLOAT))
		return mgCreateValueFloat(fmodf(lhs->data.f, rhs->data.f));
	return NULL;
}


MGValue* mgStringAdd(const MGValue *lhs, const MGValue *rhs)
{
	if ((lhs->type == MG_TYPE_STRING) && (rhs->type == MG_TYPE_STRING))
	{
		size_t len = lhs->data.str.length + rhs->data.str.length;
		char *s = (char*) malloc((len + 1) * sizeof(char));
		MG_ASSERT(s);
		strcpy(s, lhs->data.str.s);
		strcpy(s + lhs->data.str.length, rhs->data.str.s);
		s[len] = '\0';
		return mgCreateValueStringEx(s, MG_STRING_USAGE_KEEP);
	}
	else if (lhs->type == MG_TYPE_STRING)
	{
		char *s2 = mgValueToString(rhs);
		MG_ASSERT(s2);

		size_t len = lhs->data.str.length + strlen(s2);
		char *s = (char*) malloc((len + 1) * sizeof(char));
		MG_ASSERT(s);

		strcpy(s, lhs->data.str.s);
		strcpy(s + lhs->data.str.length, s2);
		s[len] = '\0';

		free(s2);

		return mgCreateValueStringEx(s, MG_STRING_USAGE_KEEP);
	}
	else if (rhs->type == MG_TYPE_STRING)
	{
		char *s2 = mgValueToString(lhs);
		MG_ASSERT(s2);

		size_t len = rhs->data.str.length + strlen(s2);
		char *s = (char*) malloc((rhs->data.str.length + strlen(s2) + 1) * sizeof(char));
		MG_ASSERT(s);

		strcpy(s, s2);
		strcpy(s + (len - rhs->data.str.length), rhs->data.str.s);
		s[len] = '\0';

		free(s2);

		return mgCreateValueStringEx(s, MG_STRING_USAGE_KEEP);
	}

	return NULL;
}


MGValue* mgStringMul(const MGValue *lhs, const MGValue *rhs)
{
	const char *str;
	size_t len;
	int times;

	if ((lhs->type == MG_TYPE_STRING) && (rhs->type == MG_TYPE_INTEGER))
	{
		str = lhs->data.str.s;
		len = lhs->data.str.length;
		times = rhs->data.i;
	}
	else if ((lhs->type == MG_TYPE_INTEGER) && (rhs->type == MG_TYPE_STRING))
	{
		str = rhs->data.str.s;
		len = rhs->data.str.length;
		times = lhs->data.i;
	}
	else
		return NULL;

	if ((len > 0) && (times > 0))
		return mgCreateValueStringEx(mgStringRepeatDuplicate(str, len, (size_t) times), MG_STRING_USAGE_KEEP);
	return mgCreateValueStringEx("", MG_STRING_USAGE_STATIC);
}


MGValue* mgTypeListAdd(const MGValue *lhs, const MGValue *rhs)
{
	if (((lhs->type == MG_TYPE_TUPLE) || (lhs->type == MG_TYPE_LIST)) && (lhs->type == rhs->type))
	{
		MGValue *list = mgCreateValueList(mgListLength(lhs) + mgListLength(rhs));
		list->type = (lhs->type == MG_TYPE_TUPLE) ? MG_TYPE_TUPLE : MG_TYPE_LIST;

		for (size_t i = 0; i < mgListLength(lhs); ++i)
			mgListAdd(list, mgReferenceValue(_mgListGet(lhs->data.a, i)));

		for (size_t i = 0; i < mgListLength(rhs); ++i)
			mgListAdd(list, mgReferenceValue(_mgListGet(rhs->data.a, i)));

		return list;
	}

	return NULL;
}


MGValue* mgListMul(const MGValue *lhs, const MGValue *rhs)
{
	const MGValue *list;
	int times;

	if (((lhs->type == MG_TYPE_TUPLE) || (lhs->type == MG_TYPE_LIST)) && (rhs->type == MG_TYPE_INTEGER))
	{
		list = lhs;
		times = rhs->data.i;
	}
	else if ((lhs->type == MG_TYPE_INTEGER) || ((rhs->type == MG_TYPE_TUPLE) || (rhs->type == MG_TYPE_LIST)))
	{
		list = rhs;
		times = lhs->data.i;
	}
	else
		return NULL;

	const size_t len = ((mgListLength(list) > 0) && (times > 0)) ? (mgListLength(list) * times) : 0;
	MGValue *repeated = (list->type == MG_TYPE_TUPLE) ? mgCreateValueTuple(len) : mgCreateValueList(len);

	for (size_t i = 0; i < len; ++i)
		mgListAdd(repeated, mgReferenceValue(_mgListGet(list->data.a, i % mgListLength(list))));

	return repeated;
}


static inline size_t _mgListRelativeIndex(const MGValue *list, const MGValue *index)
{
	size_t i = (size_t) _mgListIndexRelativeToAbsolute(list->data.a, mgIntegerGet(index));

	if ((i < 0) || (i >= mgListLength(list)))
	{
		if (mgIntegerGet(index) >= 0)
			mgFatalError("Error: %s index out of range (0 <= %d < %zu)",
			             mgGetTypeName(list->type), mgIntegerGet(index), mgListLength(list));
		else
			mgFatalError("Error: %s index out of range (-%zu <= %d < 0)",
			             mgGetTypeName(list->type), mgListLength(list), mgIntegerGet(index));
	}

	return i;
}


MGValue* mgListSubscriptGet(const MGValue *list, const MGValue *index)
{
	if (index->type != MG_TYPE_INTEGER)
		return NULL;

	MGValue *value = _mgListGet(list->data.a, _mgListRelativeIndex(list, index));
	return value ? mgReferenceValue(value) : MG_NULL_VALUE;
}


MGbool mgListSubscriptSet(const MGValue *list, const MGValue *index, MGValue *value)
{
	if (index->type == MG_TYPE_INTEGER)
	{
		const size_t i = _mgListRelativeIndex(list, index);

		mgDestroyValue(_mgListGet(list->data.a, i));

		if (value)
			_mgListSet(list->data.a, i, mgReferenceValue(value));
		else
			_mgListRemove(((MGValue*) list)->data.a, i);

		return MG_TRUE;
	}

	return MG_FALSE;
}


MGValue* mgMapAdd(const MGValue *lhs, const MGValue *rhs)
{
	if ((lhs->type == MG_TYPE_MAP) && (rhs->type == MG_TYPE_MAP))
	{
		MGValue *map = mgCreateValueMap(mgMapSize(lhs) + mgMapSize(rhs));

		mgMapMerge(map, lhs, MG_TRUE);
		mgMapMerge(map, rhs, MG_TRUE);

		return map;
	}

	return NULL;
}


MGValue* mgMapSubscriptGet(const MGValue *map, const MGValue *key)
{
	if (key->type != MG_TYPE_STRING)
		return NULL;

	MGValue *value = mgMapGet(map, key->data.str.s);
	return value ? mgReferenceValue(value) : MG_NULL_VALUE;
}


MGbool mgMapSubscriptSet(const MGValue *map, const MGValue *key, MGValue *value)
{
	if (key->type != MG_TYPE_STRING)
		return MG_FALSE;

	mgMapSet((MGValue*) map, key->data.str.s, value);

	return MG_TRUE;
}


MGValue* mgMapAttributeGet(const MGValue *map, const char *key)
{
	MGValue *value = mgMapGet(map, key);
	return value ? mgReferenceValue(value) : MG_NULL_VALUE;
}


MGbool mgMapAttributeSet(const MGValue *map, const char *key, MGValue *value)
{
	mgMapSet((MGValue*) map, key, value);

	return MG_TRUE;
}


MGValue* mgFunctionAttributeGet(const MGValue *function, const char *key)
{
	const MGValue *value = function->data.func.locals ? mgMapGet(function->data.func.locals, key) : NULL;
	return value ? mgReferenceValue(value) : NULL;
}


MGbool mgFunctionAttributeSet(const MGValue *function, const char *key, MGValue *value)
{
	if (!function->data.func.locals)
		return MG_FALSE;

	mgMapSet(function->data.func.locals, key, value);

	return MG_TRUE;
}


MGValue* mgModuleAttributeGet(const MGValue *module, const char *key)
{
	const MGValue *value = mgMapGet(module->data.module.globals, key);
	return value ? mgReferenceValue(value) : NULL;
}


MGbool mgModuleAttributeSet(const MGValue *module, const char *key, MGValue *value)
{
	mgMapSet(module->data.module.globals, key, value);

	return MG_TRUE;
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
		mgAnyInverse,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		mgAnyEqual,
		mgAnyLess,
		mgAnyLessEqual,
		NULL,
		NULL,
		NULL,
		NULL
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
		mgAnyInverse,
		mgIntAdd,
		mgIntSub,
		mgIntMul,
		mgIntDiv,
		mgIntIntDiv,
		mgIntMod,
		mgAnyEqual,
		mgAnyLess,
		mgAnyLessEqual,
		NULL,
		NULL,
		NULL,
		NULL
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
		mgAnyInverse,
		mgFloatAdd,
		mgFloatSub,
		mgFloatMul,
		mgFloatDiv,
		mgFloatIntDiv,
		mgFloatMod,
		mgAnyEqual,
		mgAnyLess,
		mgAnyLessEqual,
		NULL,
		NULL,
		NULL,
		NULL
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
		mgAnyInverse,
		mgStringAdd,
		NULL,
		mgStringMul,
		NULL,
		NULL,
		NULL,
		mgAnyEqual,
		mgAnyLess,
		mgAnyLessEqual,
		NULL,
		NULL,
		NULL,
		NULL
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
		mgAnyInverse,
		mgTypeListAdd,
		NULL,
		mgListMul,
		NULL,
		NULL,
		NULL,
		mgAnyEqual,
		mgAnyLess,
		mgAnyLessEqual,
		mgListSubscriptGet,
		mgListSubscriptSet,
		NULL,
		NULL
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
		mgAnyInverse,
		mgTypeListAdd,
		NULL,
		mgListMul,
		NULL,
		NULL,
		NULL,
		mgAnyEqual,
		mgAnyLess,
		mgAnyLessEqual,
		mgListSubscriptGet,
		mgListSubscriptSet,
		NULL,
		NULL
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
		mgAnyInverse,
		mgMapAdd,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		mgAnyEqual,
		mgAnyLess,
		mgAnyLessEqual,
		mgMapSubscriptGet,
		mgMapSubscriptSet,
		mgMapAttributeGet,
		mgMapAttributeSet
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
		mgAnyInverse,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		mgAnyEqual,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
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
		mgAnyInverse,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		mgAnyEqual,
		NULL,
		NULL,
		NULL,
		NULL,
		mgFunctionAttributeGet,
		mgFunctionAttributeSet
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
		mgAnyInverse,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		mgAnyEqual,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
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
		mgAnyInverse,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		mgAnyEqual,
		NULL,
		NULL,
		NULL,
		NULL,
		mgModuleAttributeGet,
		mgModuleAttributeSet
	}
};
