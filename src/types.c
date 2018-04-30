
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
			mgCreateMapIterator(&iterator, (MGValue*) value);

			MGValue *k, *v;
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
		NULL,
		NULL,
		mgAnyInverse
	}
};
