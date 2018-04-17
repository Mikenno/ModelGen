
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "modelgen.h"
#include "module.h"
#include "inspect.h"
#include "utilities.h"


extern MGValue* _mg_rangei(int start, int stop, int step);


static inline void _mgFail(const char *file, int line, MGValue *module, MGNode *node, const char *format, ...)
{
	fflush(stdout);

	fprintf(stderr, "%s:%d: ", file, line);

	if (module && module->data.module.filename)
		fprintf(stderr, "%s:", module->data.module.filename);

	if (node && node->tokenBegin)
		fprintf(stderr, "%u:%u: ", node->tokenBegin->begin.line, node->tokenBegin->begin.character);
	else if (module && module->data.module.filename)
		putchar(' ');

	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);

	putc('\n', stderr);
	fflush(stderr);

	exit(1);
}

#define MG_FAIL(...) _mgFail(__FILE__, __LINE__, module, node, __VA_ARGS__)
#define _MG_FAIL(module, node, ...) _mgFail(__FILE__, __LINE__, module, node, __VA_ARGS__)


inline MGValue* mgDeepCopyValue(const MGValue *value)
{
	MG_ASSERT(value);
	MG_ASSERT((value->type != MG_VALUE_PROCEDURE) && (value->type != MG_VALUE_FUNCTION));

	MGValue *copy = (MGValue*) malloc(sizeof(MGValue));
	MG_ASSERT(copy);

	*copy = *value;
	copy->refCount = 1;

	switch (copy->type)
	{
	case MG_VALUE_STRING:
		copy->data.str.s = mgStringDuplicate(value->data.str.s);
		break;
	case MG_VALUE_TUPLE:
	case MG_VALUE_LIST:
		if (mgListLength(value))
		{
			_mgListCreate(MGValue*, copy->data.a, mgListCapacity(value));
			for (size_t i = 0; i < mgListLength(value); ++i)
				_mgListAdd(MGValue*, copy->data.a, mgDeepCopyValue(_mgListGet(value->data.a, i)));
		}
		else if (mgListCapacity(value))
			_mgListInitialize(copy->data.a);
		break;
	case MG_VALUE_MAP:
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
	default:
		break;
	}

	return copy;
}


inline MGValue* mgReferenceValue(MGValue *value)
{
	MG_ASSERT(value);

	++value->refCount;

	return value;
}


static inline void _mgSetLocalValue(MGValue *module, const char *name, MGValue *value)
{
	MG_ASSERT(module);
	MG_ASSERT(module->type == MG_VALUE_MODULE);
	MG_ASSERT(module->data.module.instance);
	MG_ASSERT(module->data.module.instance->callStackTop);
	MG_ASSERT(module->data.module.instance->callStackTop->locals);

	mgMapSet(module->data.module.instance->callStackTop->locals, name, value);
}


static inline void _mgSetValue(MGValue *module, const char *name, MGValue *value)
{
	MG_ASSERT(module);
	MG_ASSERT(module->type == MG_VALUE_MODULE);
	MG_ASSERT(module->data.module.instance);
	MG_ASSERT(module->data.module.instance->callStackTop);
	MG_ASSERT(module->data.module.instance->callStackTop->locals);

	if (mgMapGet(module->data.module.instance->callStackTop->locals, name) || !mgModuleGet(module, name))
		_mgSetLocalValue(module, name, value);
	else
		mgModuleSet(module, name, value);
}


static inline MGValue* _mgGetValue(MGValue *module, const char *name)
{
	MG_ASSERT(module);
	MG_ASSERT(module->type == MG_VALUE_MODULE);
	MG_ASSERT(module->data.module.instance);
	MG_ASSERT(module->data.module.instance->callStackTop);
	MG_ASSERT(module->data.module.instance->callStackTop->locals);

	MGValue *value = mgMapGet(module->data.module.instance->callStackTop->locals, name);

	if (!value)
		value = mgModuleGet(module, name);

	if (!value)
		_MG_FAIL(module, NULL, "Error: Undefined name \"%s\"", name);

	return value;
}


static inline size_t _mgResolveArrayIndex(MGValue *module, MGNode *node, MGValue *collection, MGValue *index)
{
	MG_ASSERT(collection);
	MG_ASSERT((collection->type == MG_VALUE_TUPLE) || (collection->type == MG_VALUE_LIST));
	MG_ASSERT(index);

	if (index->type != MG_VALUE_INTEGER)
		MG_FAIL("Error: \"%s\" index must be \"%s\" and not \"%s\"",
		        _MG_VALUE_TYPE_NAMES[collection->type], _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[index->type]);

	size_t i = (size_t) _mgListIndexRelativeToAbsolute(collection->data.a, mgIntegerGet(index));

	if ((i < 0) || (i >= mgListLength(collection)))
	{
		if (mgIntegerGet(index) >= 0)
		{
			MG_FAIL("Error: \"%s\" index out of range (0 <= %d < %zu)",
			        _MG_VALUE_TYPE_NAMES[collection->type],
			        mgIntegerGet(index), mgListLength(collection));
		}
		else
		{
			MG_FAIL("Error: \"%s\" index out of range (%zu <= %d < 0)",
			        _MG_VALUE_TYPE_NAMES[collection->type],
			        -mgListLength(collection), mgIntegerGet(index));
		}
	}

	return i;
}


static inline MGValue* _mgResolveArrayGet(MGValue *module, MGNode *node, MGValue *collection, MGValue *index)
{
	return mgReferenceValue(_mgListGet(collection->data.a, _mgResolveArrayIndex(module, node, collection, index)));
}


static inline void _mgResolveArraySet(MGValue *module, MGNode *node, MGValue *collection, MGValue *index, MGValue *value)
{
	const size_t i = _mgResolveArrayIndex(module, node, collection, index);

	mgDestroyValue(_mgListGet(collection->data.a, i));

	if (value)
		_mgListSet(collection->data.a, i, mgReferenceValue(value));
	else
		_mgListRemove(collection->data.a, i);
}


static inline MGValue* _mgResolveMapGet(MGValue *module, MGNode *node, MGValue *collection, MGValue *key)
{
	MG_ASSERT(collection);
	MG_ASSERT(collection->type == MG_VALUE_MAP);
	MG_ASSERT(key);
	MG_ASSERT(key->type == MG_VALUE_STRING);
	MG_ASSERT(key->data.str.s);

	MGValue *value = mgMapGet(collection, key->data.str.s);

	if (value == NULL)
		return mgCreateValueNull();

	return mgReferenceValue(value);
}


static inline void _mgResolveMapSet(MGValue *collection, MGValue *key, MGValue *value)
{
	MG_ASSERT(collection);
	MG_ASSERT(collection->type == MG_VALUE_MAP);
	MG_ASSERT(key);
	MG_ASSERT(key->type == MG_VALUE_STRING);
	MG_ASSERT(key->data.str.s);

	mgMapSet(collection, key->data.str.s, value);
}


static inline MGValue* _mgResolveSubscriptGet(MGValue *module, MGNode *node, MGValue *collection, MGValue *index)
{
	if ((collection->type == MG_VALUE_TUPLE) || (collection->type == MG_VALUE_LIST))
		return _mgResolveArrayGet(module, node, collection, index);
	else if (collection->type == MG_VALUE_MAP)
		return _mgResolveMapGet(module, node, collection, index);
	else if (collection->type == MG_VALUE_MODULE)
		return _mgResolveMapGet(module, node, collection->data.module.globals, index);
	else
		MG_FAIL("Error: \"%s\" is not subscriptable", _MG_VALUE_TYPE_NAMES[collection->type]);

	return NULL;
}


static inline void _mgResolveSubscriptSet(MGValue *module, MGNode *node, MGValue *collection, MGValue *index, MGValue *value)
{
	if ((collection->type == MG_VALUE_TUPLE) || (collection->type == MG_VALUE_LIST))
		_mgResolveArraySet(module, node, collection, index, value);
	else if (collection->type == MG_VALUE_MAP)
		_mgResolveMapSet(collection, index, value);
	else if (collection->type == MG_VALUE_MODULE)
		_mgResolveMapSet(collection->data.module.globals, index, value);
	else
		MG_FAIL("Error: \"%s\" is not subscriptable", _MG_VALUE_TYPE_NAMES[collection->type]);
}


static MGValue* _mgVisitNode(MGValue *module, MGNode *node);


static MGValue* _mgVisitChildren(MGValue *module, MGNode *node)
{
	MG_ASSERT(module);
	MG_ASSERT(module->type == MG_VALUE_MODULE);
	MG_ASSERT(module->data.module.instance);
	MG_ASSERT(module->data.module.instance->callStackTop);

	MGStackFrame *frame = module->data.module.instance->callStackTop;

	for (size_t i = 0; i < _mgListLength(node->children); ++i)
	{
		mgDestroyValue(_mgVisitNode(module, _mgListGet(node->children, i)));

		if (frame->state == MG_STACK_FRAME_STATE_UNWINDING)
		{
			if (frame->value)
				return mgReferenceValue(frame->value);

			break;
		}
	}

	return mgCreateValueNull();
}


#if defined(__GNUC__)
static inline __attribute__((always_inline)) MGValue* _mgVisitModule(MGValue *module, MGNode *node)
#elif defined(_MSC_VER)
static __forceinline MGValue* _mgVisitModule(MGValue *module, MGNode *node)
#else
static inline MGValue* _mgVisitModule(MGValue *module, MGNode *node)
#endif
{
	return _mgVisitChildren(module, node);
}


static MGValue* _mgVisitCall(MGValue *module, MGNode *node)
{
	MG_ASSERT(module);
	MG_ASSERT(module->data.module.instance);
	MG_ASSERT(_mgListLength(node->children) > 0);

	MGNode *nameNode = _mgListGet(node->children, 0);

	const MGValue *func = NULL;
	char *name = NULL;

	if (nameNode->type == MG_NODE_NAME)
	{
		MG_ASSERT(nameNode->token);

		const size_t nameLength = nameNode->token->end.string - nameNode->token->begin.string;
		MG_ASSERT(nameLength > 0);
		name = mgStringDuplicateFixed(nameNode->token->begin.string, nameLength);

		func = _mgGetValue(module, name);

		if (!func)
			MG_FAIL("Error: Undefined name \"%s\"", name);
	}
	else
		func = _mgVisitNode(module, nameNode);

	MG_ASSERT(func);

	if (name == NULL)
		name = mgStringDuplicate("<anonymous>");

	if ((func->type != MG_VALUE_CFUNCTION) && (func->type != MG_VALUE_PROCEDURE) && (func->type != MG_VALUE_FUNCTION))
		MG_FAIL("Error: \"%s\" is not callable", _MG_VALUE_TYPE_NAMES[func->type]);

	_MGList(MGValue*) args;
	_mgListCreate(MGValue*, args, _mgListLength(node->children) - 1);

	for (size_t i = 0; i < _mgListCapacity(args); ++i)
	{
		_mgListAdd(MGValue*, args, _mgVisitNode(module, _mgListGet(node->children, i + 1)));
		MG_ASSERT(_mgListGet(args, i));
	}

	MGStackFrame frame;

	if ((func->type != MG_VALUE_CFUNCTION) && func->data.func.locals)
		mgCreateStackFrameEx(&frame, mgReferenceValue(module), mgReferenceValue(func->data.func.locals));
	else
		mgCreateStackFrame(&frame, mgReferenceValue(module));

	frame.state = MG_STACK_FRAME_STATE_ACTIVE;
	frame.caller = node;
	frame.callerName = name;

	mgPushStackFrame(frame.module->data.module.instance, &frame);

	if (func->type == MG_VALUE_CFUNCTION)
		frame.value = func->data.cfunc(frame.module->data.module.instance, _mgListLength(args), _mgListItems(args));
	else
	{
		MG_ASSERT(func->data.func.module);
		MG_ASSERT(func->data.func.node);

		if (func->data.func.locals)
		{
			MGMapIterator iterator;
			mgCreateMapIterator(&iterator, func->data.func.locals);

			MGValue *k, *v;
			while (mgMapNext(&iterator, &k, &v))
				_mgSetValue(module, k->data.str.s, mgReferenceValue(v));

			mgDestroyMapIterator(&iterator);
		}

		MGNode *funcNode = func->data.func.node;

		if (funcNode)
		{
			MG_ASSERT((funcNode->type == MG_NODE_PROCEDURE) || funcNode->type == MG_NODE_FUNCTION);
			MG_ASSERT((_mgListLength(funcNode->children) == 2) || (_mgListLength(funcNode->children) == 3));

			MGNode *funcParametersNode = _mgListGet(funcNode->children, 1);
			MG_ASSERT(funcParametersNode->type == MG_NODE_TUPLE);

			if (_mgListLength(funcParametersNode->children) < _mgListLength(args))
			{
				MGNode *funcNameNode = _mgListGet(funcNode->children, 0);
				MG_ASSERT(funcNameNode->type == MG_NODE_NAME);
				MG_ASSERT(funcNameNode->token);

				const size_t funcNameLength = funcNameNode->token->end.string - funcNameNode->token->begin.string;
				MG_ASSERT(funcNameLength > 0);
				char *funcName = mgStringDuplicateFixed(funcNameNode->token->begin.string, funcNameLength);
				MG_ASSERT(funcName);

				MG_FAIL("Error: %s expected at most %zu arguments, received %zu", funcName, _mgListLength(funcParametersNode->children), _mgListLength(args));

				free(funcName);
			}

			for (size_t i = 0; i < _mgListLength(funcParametersNode->children); ++i)
			{
				MGNode *funcParameterNode = _mgListGet(funcParametersNode->children, i);
				MG_ASSERT((funcParameterNode->type == MG_NODE_NAME) || (funcParameterNode->type == MG_NODE_ASSIGN));

				char *funcParameterName = NULL;

				if (funcParameterNode->type == MG_NODE_NAME)
				{
					const size_t funcParameterNameLength = funcParameterNode->token->end.string - funcParameterNode->token->begin.string;
					MG_ASSERT(funcParameterNameLength > 0);
					funcParameterName = mgStringDuplicateFixed(funcParameterNode->token->begin.string, funcParameterNameLength);
				}
				else if (funcParameterNode->type == MG_NODE_ASSIGN)
				{
					MG_ASSERT(_mgListLength(funcParameterNode->children) == 2);

					MGNode *funcParameterNameNode = _mgListGet(funcParameterNode->children, 0);
					MG_ASSERT(funcParameterNameNode->type == MG_NODE_NAME);
					MG_ASSERT(funcParameterNameNode->token);

					const size_t funcParameterNameLength = funcParameterNameNode->token->end.string - funcParameterNameNode->token->begin.string;
					MG_ASSERT(funcParameterNameLength > 0);
					funcParameterName = mgStringDuplicateFixed(funcParameterNameNode->token->begin.string, funcParameterNameLength);
				}

				MG_ASSERT(funcParameterName);

				if (i < _mgListLength(args))
					_mgSetLocalValue(module, funcParameterName, mgReferenceValue(_mgListGet(args, i)));
				else
				{
					if (funcParameterNode->type != MG_NODE_ASSIGN)
						MG_FAIL("Error: Expected argument \"%s\"", funcParameterName);

					_mgSetLocalValue(module, funcParameterName, _mgVisitNode(module, _mgListGet(funcParameterNode->children, 1)));
				}

				free(funcParameterName);
			}

			if (_mgListLength(funcNode->children) == 3)
				_mgVisitNode(func->data.func.module, _mgListGet(funcNode->children, 2));
		}
	}

	MGValue *value = NULL;

	if (frame.value)
		value = mgReferenceValue(frame.value);
	else
		value = mgCreateValueNull();

	mgPopStackFrame(frame.module->data.module.instance, &frame);
	mgDestroyStackFrame(&frame);

	for (size_t i = 0; i < _mgListLength(args); ++i)
		mgDestroyValue(_mgListGet(args, i));
	_mgListDestroy(args);

	free(name);

	MG_ASSERT(value);

	return value;
}


static MGValue* _mgVisitEmit(MGValue *module, MGNode *node)
{
	MG_ASSERT(module);
	MG_ASSERT(module->data.module.instance);
	MG_ASSERT(node->type == MG_NODE_EMIT);
	MG_ASSERT(_mgListLength(node->children) == 1);

	MGValue *tuple = _mgVisitNode(module, _mgListGet(node->children, 0));
	MG_ASSERT(tuple);

	if (tuple->type != MG_VALUE_TUPLE)
		MG_FAIL("Error: Expected \"%s\", received \"%s\"",
		        _MG_VALUE_TYPE_NAMES[MG_VALUE_TUPLE], _MG_VALUE_TYPE_NAMES[tuple->type]);
	else if (mgTupleLength(tuple) != 3)
		MG_FAIL("Error: Expected tuple with a length of 3, received a tuple with a length of %zu",
		        mgTupleLength(tuple));

	const size_t vertexCount = _mgListLength(module->data.module.instance->vertices);

	_mgListAddUninitialized(MGVertex, module->data.module.instance->vertices);
	MGVertex *vertices = _mgListItems(module->data.module.instance->vertices);

	for (size_t i = 0; i < 3; ++i)
	{
		if (mgTupleGet(tuple, i)->type == MG_VALUE_INTEGER)
			vertices[vertexCount][i] = (float) mgTupleGet(tuple, i)->data.i;
		else if (mgTupleGet(tuple, i)->type == MG_VALUE_FLOAT)
			vertices[vertexCount][i] = mgTupleGet(tuple, i)->data.f;
		else
			MG_FAIL("Error: Expected \"%s\" or \"%s\", received \"%s\"",
			        _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[MG_VALUE_FLOAT],
			        _MG_VALUE_TYPE_NAMES[tuple->type]);
	}

	++_mgListLength(module->data.module.instance->vertices);

	mgDestroyValue(tuple);

	return mgCreateValueNull();
}


static MGValue* _mgVisitReturn(MGValue *module, MGNode *node)
{
	MG_ASSERT(module);
	MG_ASSERT(module->data.module.instance);
	MG_ASSERT(module->data.module.instance->callStackTop);

	MGStackFrame *frame = module->data.module.instance->callStackTop;

	if (_mgListLength(node->children) > 0)
		frame->value = _mgVisitNode(module, _mgListGet(node->children, 0));
	else
		frame->value = mgCreateValueTuple(0);

	frame->state = MG_STACK_FRAME_STATE_UNWINDING;

	return mgCreateValueInteger((_mgListLength(node->children) > 0) ? 1 : 0);
}


static void _mgDelete(MGValue *module, MGNode *node)
{
	if (node->type == MG_NODE_NAME)
	{
		const size_t nameLength = node->token->end.string - node->token->begin.string;
		MG_ASSERT(nameLength > 0);
		char *name = mgStringDuplicateFixed(node->token->begin.string, nameLength);
		MG_ASSERT(name);

#if MG_DEBUG
		// Check if the name is defined
		_mgGetValue(module, name);
#endif

		_mgSetValue(module, name, NULL);

		free(name);
	}
	else if (node->type == MG_NODE_TUPLE)
	{
		for (size_t i = 0; i < _mgListLength(node->children); ++i)
			_mgDelete(module, _mgListGet(node->children, i));
	}
	else if (node->type == MG_NODE_SUBSCRIPT)
	{
		MG_ASSERT(_mgListLength(node->children) == 2);

		MGValue *index = _mgVisitNode(module, _mgListGet(node->children, 1));
		MGValue *collection = _mgVisitNode(module, _mgListGet(node->children, 0));

		MG_ASSERT(collection);
		MG_ASSERT(index);

#if MG_DEBUG
		// Check if the name is defined
		mgDestroyValue(_mgResolveSubscriptGet(module, node, collection, index));
#endif

		_mgResolveSubscriptSet(module, node, collection, index, NULL);

		mgDestroyValue(collection);
		mgDestroyValue(index);
	}
	else if (node->type == MG_NODE_ATTRIBUTE)
	{
		MG_ASSERT(_mgListLength(node->children) == 2);

		MGValue *object = _mgVisitNode(module, _mgListGet(node->children, 0));
		MG_ASSERT(object);
		MG_ASSERT((object->type == MG_VALUE_MAP) || (object->type == MG_VALUE_MODULE));

		const MGNode *attributeNode = _mgListGet(node->children, 1);
		MG_ASSERT(attributeNode->type == MG_NODE_NAME);
		MG_ASSERT(attributeNode->token);

		const size_t nameLength = attributeNode->token->end.string - attributeNode->token->begin.string;
		MG_ASSERT(nameLength > 0);
		char *name = mgStringDuplicateFixed(attributeNode->token->begin.string, nameLength);
		MG_ASSERT(name);

		if (object->type == MG_VALUE_MAP)
		{
#if MG_DEBUG
			// Check if the name is defined
			mgMapGet(object, name);
#endif

			mgMapSet(object, name, NULL);
		}
		else if (object->type == MG_VALUE_MODULE)
		{
#if MG_DEBUG
			// Check if the name is defined
			mgMapGet(object->data.module.globals, name);
#endif

			mgMapSet(object->data.module.globals, name, NULL);
		}

		free(name);

		mgDestroyValue(object);
	}
	else
		MG_FAIL("Error: Cannot delete \"%s\"", _MG_NODE_NAMES[node->type]);
}


static inline MGValue* _mgVisitDelete(MGValue *module, MGNode *node)
{
	MG_ASSERT(_mgListLength(node->children) == 1);

	_mgDelete(module, _mgListGet(node->children, 0));

	return mgCreateValueNull();
}


static MGValue* _mgVisitFor(MGValue *module, MGNode *node)
{
	MG_ASSERT(_mgListLength(node->children) >= 2);

	MGNode *nameNode = _mgListGet(node->children, 0);
	MG_ASSERT(nameNode->type == MG_NODE_NAME);
	MG_ASSERT(nameNode->token);

	const size_t nameLength = nameNode->token->end.string - nameNode->token->begin.string;
	MG_ASSERT(nameLength > 0);
	char *name = mgStringDuplicateFixed(nameNode->token->begin.string, nameLength);
	MG_ASSERT(name);

	MGValue *test = _mgVisitNode(module, _mgListGet(node->children, 1));
	MG_ASSERT(test);
	MG_ASSERT((test->type == MG_VALUE_TUPLE) || (test->type == MG_VALUE_LIST));

	int iterations = 0;

	for (size_t i = 0; i < _mgListLength(test->data.a); ++i, ++iterations)
	{
		MGValue *value = _mgListGet(test->data.a, i);
		MG_ASSERT(value);

		_mgSetLocalValue(module, name, mgReferenceValue(value));

		for (size_t j = 2; j < _mgListLength(node->children); ++j)
		{
			MGValue *result = _mgVisitNode(module, _mgListGet(node->children, j));
			MG_ASSERT(result);
			mgDestroyValue(result);
		}
	}

	mgDestroyValue(test);

	free(name);

	return mgCreateValueInteger(iterations);
}


static MGValue* _mgVisitIf(MGValue *module, MGNode *node)
{
	MG_ASSERT(_mgListLength(node->children) >= 2);

	MGValue *condition = _mgVisitNode(module, _mgListGet(node->children, 0));
	MG_ASSERT(condition);

	int _condition = 0;

	switch (condition->type)
	{
	case MG_VALUE_INTEGER:
		_condition = condition->data.i != 0;
		break;
	case MG_VALUE_FLOAT:
		_condition = !_MG_FEQUAL(condition->data.f, 0.0f);
		break;
	case MG_VALUE_STRING:
		_condition = condition->data.str.length != 0;
		break;
	case MG_VALUE_TUPLE:
		_condition = condition->data.a.length > 0;
		break;
	default:
		_condition = 1;
		break;
	}

	if (_condition)
		return _mgVisitNode(module, _mgListGet(node->children, 1));
	else if (_mgListLength(node->children) > 2)
		return _mgVisitNode(module, _mgListGet(node->children, 2));
	return mgCreateValueInteger(_condition);
}


static MGValue* _mgVisitFunction(MGValue *module, MGNode *node)
{
	MG_ASSERT(module);
	MG_ASSERT(module->data.module.instance);
	MG_ASSERT((_mgListLength(node->children) == 2) || (_mgListLength(node->children) == 3));

	MGNode *nameNode = _mgListGet(node->children, 0);
	MG_ASSERT((nameNode->type == MG_NODE_INVALID) || (nameNode->type == MG_NODE_NAME) || (nameNode->type == MG_NODE_ATTRIBUTE));

	MGValue *func = mgCreateValue((node->type == MG_NODE_PROCEDURE) ? MG_VALUE_PROCEDURE : MG_VALUE_FUNCTION);

	func->data.func.module = module;
	func->data.func.node = node;

	MGbool isClosure = MG_FALSE;

	if (module->data.module.instance->callStackTop && module->data.module.instance->callStackTop->last)
	{
		for (MGNode *parent = node->parent; parent; parent = parent->parent)
		{
			if ((parent->type == MG_NODE_PROCEDURE) || (parent->type == MG_NODE_FUNCTION))
			{
				isClosure = MG_TRUE;
				break;
			}
		}
	}

	if (isClosure && module->data.module.instance->callStackTop && module->data.module.instance->callStackTop->locals)
		func->data.func.locals = mgReferenceValue(module->data.module.instance->callStackTop->locals);
	else
		func->data.func.locals = NULL;

	if (nameNode->type == MG_NODE_INVALID)
		return func;

	MG_ASSERT((nameNode->type == MG_NODE_NAME) || (nameNode->type == MG_NODE_ATTRIBUTE));

	if (nameNode->type == MG_NODE_NAME)
	{
		MG_ASSERT(nameNode->token);

		const size_t nameLength = nameNode->token->end.string - nameNode->token->begin.string;
		MG_ASSERT(nameLength > 0);
		char *name = mgStringDuplicateFixed(nameNode->token->begin.string, nameLength);
		MG_ASSERT(name);

		_mgSetValue(module, name, mgReferenceValue(func));

		free(name);
	}
	else if (nameNode->type == MG_NODE_ATTRIBUTE)
	{
		MG_ASSERT(_mgListLength(nameNode->children) == 2);

		MGValue *object = _mgVisitNode(module, _mgListGet(nameNode->children, 0));
		MG_ASSERT(object);
		MG_ASSERT((object->type == MG_VALUE_MAP) || ((object->type == MG_VALUE_FUNCTION) && object->data.func.locals) || (object->type == MG_VALUE_MODULE));

		MGNode *attributeNode = _mgListGet(nameNode->children, 1);
		MG_ASSERT(attributeNode->type == MG_NODE_NAME);
		MG_ASSERT(attributeNode->token);

		const size_t nameLength = attributeNode->token->end.string - attributeNode->token->begin.string;
		MG_ASSERT(nameLength > 0);
		char *name = mgStringDuplicateFixed(attributeNode->token->begin.string, nameLength);
		MG_ASSERT(name);

		if (object->type == MG_VALUE_MAP)
			mgMapSet(object, name, mgReferenceValue(func));
		else if ((object->type == MG_VALUE_FUNCTION) && object->data.func.locals)
			mgMapSet(object->data.func.locals, name, mgReferenceValue(func));
		else if (object->type == MG_VALUE_MODULE)
			mgMapSet(object->data.module.globals, name, mgReferenceValue(func));

		free(name);

		mgDestroyValue(object);
	}

	return func;
}


static MGValue* _mgVisitSubscript(MGValue *module, MGNode *node)
{
	MG_ASSERT(_mgListLength(node->children) == 2);

	MGValue *index = _mgVisitNode(module, _mgListGet(node->children, 1));
	MGValue *collection = _mgVisitNode(module, _mgListGet(node->children, 0));

	MG_ASSERT(collection);
	MG_ASSERT(index);

	MGValue *value = _mgResolveSubscriptGet(module, node, collection, index);

	mgDestroyValue(collection);
	mgDestroyValue(index);

	MG_ASSERT(value);

	return value;
}


static MGValue* _mgVisitAttribute(MGValue *module, MGNode *node)
{
	MG_ASSERT(_mgListLength(node->children) == 2);

	MGValue *object = _mgVisitNode(module, _mgListGet(node->children, 0));
	MG_ASSERT(object);
	MG_ASSERT((object->type == MG_VALUE_MAP) || ((object->type == MG_VALUE_FUNCTION) && object->data.func.locals) || (object->type == MG_VALUE_MODULE));

	const MGNode *attribute = _mgListGet(node->children, 1);
	MG_ASSERT(attribute);
	MG_ASSERT(attribute->type == MG_NODE_NAME);
	MG_ASSERT(attribute->token);

	const size_t nameLength = attribute->token->end.string - attribute->token->begin.string;
	MG_ASSERT(nameLength > 0);
	char *name = mgStringDuplicateFixed(attribute->token->begin.string, nameLength);
	MG_ASSERT(name);

	MGValue *value = NULL;

	if (object->type == MG_VALUE_MAP)
		value = mgMapGet(object, name);
	else if ((object->type == MG_VALUE_FUNCTION) && object->data.func.locals)
		value = mgMapGet(object->data.func.locals, name);
	else if (object->type == MG_VALUE_MODULE)
		value = mgMapGet(object->data.module.globals, name);

	if (value == NULL)
		MG_FAIL("Error: \"%s\" has no attribute \"%s\"",
		        _MG_VALUE_TYPE_NAMES[object->type], name);

	free(name);

	return mgReferenceValue(value);
}


static MGValue* _mgVisitName(MGValue *module, MGNode *node)
{
	MG_ASSERT(node->token);

	const size_t nameLength = node->token->end.string - node->token->begin.string;
	MG_ASSERT(nameLength > 0);
	char *name = mgStringDuplicateFixed(node->token->begin.string, nameLength);
	MG_ASSERT(name);

	MGValue *value = _mgGetValue(module, name);

	free(name);

	return mgReferenceValue(value);
}


#if defined(__GNUC__)
static inline __attribute__((always_inline)) MGValue* _mgVisitNull(MGValue *module, MGNode *node)
#elif defined(_MSC_VER)
static __forceinline MGValue* _mgVisitNull(MGValue *module, MGNode *node)
#else
static inline MGValue* _mgVisitNull(MGValue *module, MGNode *node)
#endif
{
	MG_ASSERT(node->type == MG_NODE_NULL);

	return mgCreateValueNull();
}


static MGValue* _mgVisitInteger(MGValue *module, MGNode *node)
{
	MG_ASSERT(node->token);

	const size_t _valueLength = node->token->end.string - node->token->begin.string;
	MG_ASSERT(_valueLength > 0);
	char *_value = mgStringDuplicateFixed(node->token->begin.string, _valueLength);
	MG_ASSERT(_value);

	MGValue *value = mgCreateValueInteger(strtol(_value, NULL, 10));

	free(_value);

	return value;
}


static MGValue* _mgVisitFloat(MGValue *module, MGNode *node)
{
	MG_ASSERT(node->token);

	const size_t _valueLength = node->token->end.string - node->token->begin.string;
	MG_ASSERT(_valueLength > 0);
	char *_value = mgStringDuplicateFixed(node->token->begin.string, _valueLength);
	MG_ASSERT(_value);

	MGValue *value = mgCreateValueFloat(strtof(_value, NULL));

	free(_value);

	return value;
}


#if defined(__GNUC__)
static inline __attribute__((always_inline)) MGValue* _mgVisitString(MGValue *module, MGNode *node)
#elif defined(_MSC_VER)
static __forceinline MGValue* _mgVisitString(MGValue *module, MGNode *node)
#else
static inline MGValue* _mgVisitString(MGValue *module, MGNode *node)
#endif
{
	MG_ASSERT(node->token);

	return mgCreateValueString(node->token->value.s);
}


static MGValue* _mgVisitTuple(MGValue *module, MGNode *node)
{
	MG_ASSERT((node->type == MG_NODE_TUPLE) || (node->type == MG_NODE_LIST));

	MGValue *value = mgCreateValueTuple(_mgListLength(node->children));
	value->type = (node->type == MG_NODE_TUPLE) ? MG_VALUE_TUPLE : MG_VALUE_LIST;

	for (size_t i = 0; i < _mgListLength(node->children); ++i)
		mgTupleAdd(value, _mgVisitNode(module, _mgListGet(node->children, i)));

	return value;
}


static MGValue* _mgVisitRange(MGValue *module, MGNode *node)
{
	MG_ASSERT((_mgListLength(node->children) == 2) || (_mgListLength(node->children) == 3));

	MGValue *start = NULL, *stop = NULL, *step = NULL;

	start = _mgVisitNode(module, _mgListGet(node->children, 0));
	MG_ASSERT(start);
	MG_ASSERT(start->type == MG_VALUE_INTEGER);

	stop = _mgVisitNode(module, _mgListGet(node->children, 1));
	MG_ASSERT(stop);
	MG_ASSERT(stop->type == MG_VALUE_INTEGER);

	if (_mgListLength(node->children) == 3)
	{
		step = _mgVisitNode(module, _mgListGet(node->children, 2));
		MG_ASSERT(step);
		MG_ASSERT(step->type == MG_VALUE_INTEGER);
	}

	return _mg_rangei(start->data.i, stop->data.i, (_mgListLength(node->children) == 3) ? step->data.i : 0);
}


static MGValue* _mgVisitMap(MGValue *module, MGNode *node)
{
	MG_ASSERT(node->type == MG_NODE_MAP);
	MG_ASSERT((_mgListLength(node->children) % 2) == 0);

	MGValue *map = mgCreateValueMap(_mgListLength(node->children) / 2);

	for (size_t i = 0; i < _mgListLength(node->children); i += 2)
	{
		MGValue *value = _mgVisitNode(module, _mgListGet(node->children, i + 1));
		MGValue *key = _mgVisitNode(module, _mgListGet(node->children, i));

		MG_ASSERT(key);
		MG_ASSERT(key->type == MG_VALUE_STRING);
		MG_ASSERT(key->data.str.s);
		MG_ASSERT(value);

		mgMapSet(map, key->data.str.s, value);

		mgDestroyValue(key);
	}

	return map;
}


static inline MGValue* _mgVisitAssignment(MGValue *module, MGNode *node)
{
	MG_ASSERT(_mgListLength(node->children) == 2);

	MGValue *value = _mgVisitNode(module, _mgListGet(node->children, 1));
	MG_ASSERT(value);

	MGNode *lhs = _mgListGet(node->children, 0);
	MG_ASSERT(lhs);
	MG_ASSERT((lhs->type == MG_NODE_NAME) || (lhs->type == MG_NODE_SUBSCRIPT) || (lhs->type == MG_NODE_ATTRIBUTE) || (lhs->type == MG_NODE_TUPLE));

	if (lhs->type == MG_NODE_NAME)
	{
		MG_ASSERT(lhs->token);

		const size_t nameLength = lhs->token->end.string - lhs->token->begin.string;
		MG_ASSERT(nameLength > 0);
		char *name = mgStringDuplicateFixed(lhs->token->begin.string, nameLength);
		MG_ASSERT(name);

		_mgSetValue(module, name, mgReferenceValue(value));

		free(name);
	}
	else if (lhs->type == MG_NODE_SUBSCRIPT)
	{
		MG_ASSERT(_mgListLength(lhs->children) == 2);

		MGValue *index = _mgVisitNode(module, _mgListGet(lhs->children, 1));
		MGValue *collection = _mgVisitNode(module, _mgListGet(lhs->children, 0));

		MG_ASSERT(collection);
		MG_ASSERT(index);

		_mgResolveSubscriptSet(module, lhs, collection, index, mgReferenceValue(value));

		mgDestroyValue(collection);
		mgDestroyValue(index);
	}
	else if (lhs->type == MG_NODE_ATTRIBUTE)
	{
		MG_ASSERT(_mgListLength(lhs->children) == 2);

		MGValue *object = _mgVisitNode(module, _mgListGet(lhs->children, 0));
		MG_ASSERT(object);
		MG_ASSERT((object->type == MG_VALUE_MAP) || ((object->type == MG_VALUE_FUNCTION) && object->data.func.locals) || (object->type == MG_VALUE_MODULE));

		MGNode *attributeNode = _mgListGet(lhs->children, 1);
		MG_ASSERT(attributeNode->type == MG_NODE_NAME);
		MG_ASSERT(attributeNode->token);

		const size_t nameLength = attributeNode->token->end.string - attributeNode->token->begin.string;
		MG_ASSERT(nameLength > 0);
		char *name = mgStringDuplicateFixed(attributeNode->token->begin.string, nameLength);
		MG_ASSERT(name);

		if (object->type == MG_VALUE_MAP)
			mgMapSet(object, name, mgReferenceValue(value));
		else if ((object->type == MG_VALUE_FUNCTION) && object->data.func.locals)
			mgMapSet(object->data.func.locals, name, mgReferenceValue(value));
		else if (object->type == MG_VALUE_MODULE)
			mgMapSet(object->data.module.globals, name, mgReferenceValue(value));

		free(name);

		mgDestroyValue(object);
	}
	else if (lhs->type == MG_NODE_TUPLE)
	{
		if ((value->type != MG_VALUE_TUPLE) && (value->type != MG_VALUE_LIST))
			MG_FAIL("Error: \"%s\" is not iterable", _MG_VALUE_TYPE_NAMES[value->type]);

		if (_mgListLength(lhs->children) != mgListLength(value))
			MG_FAIL("Error: Mismatched lengths for parallel assignment (%zu != %zu)", _mgListLength(lhs->children), mgListLength(value));

		char *name = NULL;
		size_t nameCapacity = 0;

		for (size_t i = 0; i < _mgListLength(lhs->children); ++i)
		{
			if (_mgListGet(lhs->children, i)->type != MG_NODE_NAME)
				MG_FAIL("Error: Cannot assign to \"%s\"", _MG_NODE_NAMES[_mgListGet(lhs->children, i)->type]);

			const MGNode *const nameNode = _mgListGet(lhs->children, i);
			MG_ASSERT(nameNode->token);

			const size_t nameLength = nameNode->token->end.string - nameNode->token->begin.string;
			MG_ASSERT(nameLength > 0);

			if (nameLength >= nameCapacity)
			{
				nameCapacity = nameLength + 1;
				name = (char*) realloc(name, nameCapacity * sizeof(char));
				MG_ASSERT(name);
			}

			strncpy(name, nameNode->token->begin.string, nameLength);
			name[nameLength] = '\0';

			_mgSetValue(module, name, mgReferenceValue(_mgListGet(value->data.a, i)));
		}

		free(name);
	}

	return value;
}


static MGValue* _mgVisitBinOp(MGValue *module, MGNode *node)
{
	MG_ASSERT(_mgListLength(node->children) == 2);
	MG_ASSERT(node->type != MG_NODE_INVALID);

	MGValue *lhs = _mgVisitNode(module, _mgListGet(node->children, 0));
	MG_ASSERT(lhs);
	MGValue *rhs = NULL;

	if ((node->type != MG_NODE_BIN_OP_AND) && (node->type != MG_NODE_BIN_OP_OR) && (node->type != MG_NODE_BIN_OP_COALESCE))
	{
		rhs = _mgVisitNode(module, _mgListGet(node->children, 1));
		MG_ASSERT(rhs);
	}

	MGValue *value = NULL;

	switch (node->type)
	{
	case MG_NODE_BIN_OP_ADD:
		switch (lhs->type)
		{
		case MG_VALUE_INTEGER:
			switch (rhs->type)
			{
			case MG_VALUE_INTEGER:
				value = mgCreateValueInteger(lhs->data.i + rhs->data.i);
				break;
			case MG_VALUE_FLOAT:
				value = mgCreateValueFloat(lhs->data.i + rhs->data.f);
				break;
			case MG_VALUE_STRING:
				MG_ASSERT(rhs->data.str.s);
				size_t len = (size_t) snprintf(NULL, 0, "%d", lhs->data.i);
				char *s = (char*) malloc((len + rhs->data.str.length + 1) * sizeof(char));
				snprintf(s, len + 1, "%d", lhs->data.i);
				strcpy(s + len, rhs->data.str.s);
				s[len + rhs->data.str.length] = '\0';
				value = mgCreateValueString(s);
				free(s);
				break;
			default:
				break;
			}
			break;
		case MG_VALUE_FLOAT:
			switch (rhs->type)
			{
			case MG_VALUE_INTEGER:
				value = mgCreateValueFloat(lhs->data.f + rhs->data.i);
				break;
			case MG_VALUE_FLOAT:
				value = mgCreateValueFloat(lhs->data.f + rhs->data.f);
				break;
			case MG_VALUE_STRING:
				MG_ASSERT(rhs->data.str.s);
				size_t len = (size_t) snprintf(NULL, 0, "%f", lhs->data.f);
				char *s = (char*) malloc((len + rhs->data.str.length + 1) * sizeof(char));
				snprintf(s, len + 1, "%f", lhs->data.f);
				strcpy(s + len, rhs->data.str.s);
				s[len + rhs->data.str.length] = '\0';
				value = mgCreateValueString(s);
				free(s);
				break;
			default:
				break;
			}
			break;
		case MG_VALUE_STRING:
			MG_ASSERT(lhs->data.str.s);
			char *s = NULL;
			size_t len = 0;
			switch (rhs->type)
			{
			case MG_VALUE_INTEGER:
				len = (size_t) snprintf(NULL, 0, "%d", rhs->data.i);
				s = (char*) malloc((lhs->data.str.length + len + 1) * sizeof(char));
				strcpy(s, lhs->data.str.s);
				snprintf(s + lhs->data.str.length, len + 1, "%d", rhs->data.i);
				s[lhs->data.str.length + len] = '\0';
				break;
			case MG_VALUE_FLOAT:
				len = (size_t) snprintf(NULL, 0, "%f", rhs->data.f);
				s = (char*) malloc((lhs->data.str.length + len + 1) * sizeof(char));
				strcpy(s, lhs->data.str.s);
				snprintf(s + lhs->data.str.length, len + 1, "%f", rhs->data.f);
				s[lhs->data.str.length + len] = '\0';
				break;
			case MG_VALUE_STRING:
				MG_ASSERT(rhs->data.str.s);
				s = (char*) malloc((lhs->data.str.length + rhs->data.str.length + 1) * sizeof(char));
				MG_ASSERT(s);
				strcpy(s, lhs->data.str.s);
				strcpy(s + lhs->data.str.length, rhs->data.str.s);
				s[lhs->data.str.length + rhs->data.str.length] = '\0';
			default:
				break;
			}
			if (s)
			{
				value = mgCreateValueString(s);
				free(s);
			}
			break;
		case MG_VALUE_TUPLE:
		case MG_VALUE_LIST:
			if (lhs->type == rhs->type)
			{
				value = mgCreateValueTuple(mgListLength(lhs) + mgListLength(rhs));
				value->type = (lhs->type == MG_VALUE_TUPLE) ? MG_VALUE_TUPLE : MG_VALUE_LIST;

				for (size_t i = 0; i < mgListLength(lhs); ++i)
					mgTupleAdd(value, mgReferenceValue(_mgListGet(lhs->data.a, i)));

				for (size_t i = 0; i < mgListLength(rhs); ++i)
					mgTupleAdd(value, mgReferenceValue(_mgListGet(rhs->data.a, i)));
			}
			break;
		default:
			break;
		}
		break;
	case MG_NODE_BIN_OP_SUB:
		switch (lhs->type)
		{
		case MG_VALUE_INTEGER:
			switch (rhs->type)
			{
			case MG_VALUE_INTEGER:
				value = mgCreateValueInteger(lhs->data.i - rhs->data.i);
				break;
			case MG_VALUE_FLOAT:
				value = mgCreateValueFloat(lhs->data.i - rhs->data.f);
				break;
			default:
				break;
			}
			break;
		case MG_VALUE_FLOAT:
			switch (rhs->type)
			{
			case MG_VALUE_INTEGER:
				value = mgCreateValueFloat(lhs->data.f - rhs->data.i);
				break;
			case MG_VALUE_FLOAT:
				value = mgCreateValueFloat(lhs->data.f - rhs->data.f);
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
		break;
	case MG_NODE_BIN_OP_MUL:
		switch (lhs->type)
		{
		case MG_VALUE_INTEGER:
			switch (rhs->type)
			{
			case MG_VALUE_INTEGER:
				value = mgCreateValueInteger(lhs->data.i * rhs->data.i);
				break;
			case MG_VALUE_FLOAT:
				value = mgCreateValueFloat(lhs->data.i * rhs->data.f);
				break;
			case MG_VALUE_STRING:
			{
				if ((rhs->data.str.length > 0) && (lhs->data.i > 0))
				{
					char *str = mgStringRepeatDuplicate(rhs->data.str.s, rhs->data.str.length, (size_t) lhs->data.i);
					value = mgCreateValueString(str);
					free(str);
				}
				else
					value = mgCreateValueString("");

				break;
			}
			case MG_VALUE_TUPLE:
			{
				const size_t len = ((mgListLength(rhs) > 0) && (lhs->data.i > 0)) ? (mgListLength(rhs) * lhs->data.i) : 0;
				value = mgCreateValueTuple(len);

				for (size_t i = 0; i < len; ++i)
					mgTupleAdd(value, mgReferenceValue(_mgListGet(rhs->data.a, i % mgListLength(rhs))));

				break;
			}
			default:
				break;
			}
			break;
		case MG_VALUE_FLOAT:
			switch (rhs->type)
			{
			case MG_VALUE_INTEGER:
				value = mgCreateValueFloat(lhs->data.f * rhs->data.i);
				break;
			case MG_VALUE_FLOAT:
				value = mgCreateValueFloat(lhs->data.f * rhs->data.f);
				break;
			default:
				break;
			}
			break;
		case MG_VALUE_STRING:
			if (rhs->type == MG_VALUE_INTEGER)
			{
				if ((lhs->data.str.length > 0) && (rhs->data.i > 0))
				{
					char *str = mgStringRepeatDuplicate(lhs->data.str.s, lhs->data.str.length, (size_t) rhs->data.i);
					value = mgCreateValueString(str);
					free(str);
				}
				else
					value = mgCreateValueString("");
			}
			break;
		case MG_VALUE_TUPLE:
			if (rhs->type == MG_VALUE_INTEGER)
			{
				const size_t len = ((mgListLength(lhs) > 0) && (rhs->data.i > 0)) ? (mgListLength(lhs) * rhs->data.i) : 0;
				value = mgCreateValueTuple(len);

				for (size_t i = 0; i < len; ++i)
					mgTupleAdd(value, mgReferenceValue(_mgListGet(lhs->data.a, i % mgListLength(lhs))));
			}
			break;
		default:
			break;
		}
		break;
	case MG_NODE_BIN_OP_DIV:
		switch (lhs->type)
		{
		case MG_VALUE_INTEGER:
			switch (rhs->type)
			{
			case MG_VALUE_INTEGER:
				if (rhs->data.i == 0)
					MG_FAIL("Error: Division by zero");
				value = mgCreateValueFloat(lhs->data.i / (float) rhs->data.i);
				break;
			case MG_VALUE_FLOAT:
				value = mgCreateValueFloat(lhs->data.i / rhs->data.f);
				break;
			default:
				break;
			}
			break;
		case MG_VALUE_FLOAT:
			switch (rhs->type)
			{
			case MG_VALUE_INTEGER:
				value = mgCreateValueFloat(lhs->data.f / rhs->data.i);
				break;
			case MG_VALUE_FLOAT:
				value = mgCreateValueFloat(lhs->data.f / rhs->data.f);
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
		break;
	case MG_NODE_BIN_OP_INT_DIV:
		switch (lhs->type)
		{
		case MG_VALUE_INTEGER:
			switch (rhs->type)
			{
			case MG_VALUE_INTEGER:
				if (rhs->data.i == 0)
					MG_FAIL("Error: Division by zero");
				value = mgCreateValueInteger(lhs->data.i / rhs->data.i);
				break;
			case MG_VALUE_FLOAT:
				value = mgCreateValueInteger((int) (lhs->data.i / rhs->data.f));
				break;
			default:
				break;
			}
			break;
		case MG_VALUE_FLOAT:
			switch (rhs->type)
			{
			case MG_VALUE_INTEGER:
				value = mgCreateValueInteger((int) (lhs->data.f / rhs->data.i));
				break;
			case MG_VALUE_FLOAT:
				value = mgCreateValueInteger((int) (lhs->data.f / rhs->data.f));
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
		break;
	case MG_NODE_BIN_OP_MOD:
		switch (lhs->type)
		{
		case MG_VALUE_INTEGER:
			switch (rhs->type)
			{
			case MG_VALUE_INTEGER:
				value = mgCreateValueInteger(lhs->data.i % rhs->data.i);
				break;
			case MG_VALUE_FLOAT:
				value = mgCreateValueFloat(fmodf((float) lhs->data.i, rhs->data.f));
				break;
			default:
				break;
			}
			break;
		case MG_VALUE_FLOAT:
			switch (rhs->type)
			{
			case MG_VALUE_INTEGER:
				value = mgCreateValueFloat(fmodf(lhs->data.f, (float) rhs->data.i));
				break;
			case MG_VALUE_FLOAT:
				value = mgCreateValueFloat(fmodf(lhs->data.f, rhs->data.f));
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
		break;
	case MG_NODE_BIN_OP_COALESCE:
		if (lhs->type != MG_VALUE_NULL)
			value = mgReferenceValue(lhs);
		else
		{
			rhs = _mgVisitNode(module, _mgListGet(node->children, 1));
			MG_ASSERT(rhs);
			value = mgReferenceValue(rhs);
		}
		break;
	case MG_NODE_BIN_OP_EQ:
		if ((lhs->type == MG_VALUE_NULL) || (rhs->type == MG_VALUE_NULL))
			value = mgCreateValueInteger(lhs->type == rhs->type);
		else
			switch (lhs->type)
			{
			case MG_VALUE_INTEGER:
				switch (rhs->type)
				{
				case MG_VALUE_INTEGER:
					value = mgCreateValueInteger(lhs->data.i == rhs->data.i);
					break;
				case MG_VALUE_FLOAT:
					value = mgCreateValueInteger(lhs->data.i == rhs->data.f);
					break;
				default:
					break;
				}
				break;
			case MG_VALUE_FLOAT:
				switch (rhs->type)
				{
				case MG_VALUE_INTEGER:
					value = mgCreateValueInteger(lhs->data.f == rhs->data.i);
					break;
				case MG_VALUE_FLOAT:
					value = mgCreateValueInteger(_MG_FEQUAL(lhs->data.f, rhs->data.f));
					break;
				default:
					break;
				}
				break;
			case MG_VALUE_STRING:
				MG_ASSERT(lhs->data.str.s);
				switch (rhs->type)
				{
				case MG_VALUE_STRING:
					MG_ASSERT(rhs->data.str.s);
					value = mgCreateValueInteger(!strcmp(lhs->data.str.s, rhs->data.str.s));
					break;
				default:
					break;
				}
				break;
			default:
				break;
			}
		break;
	case MG_NODE_BIN_OP_NOT_EQ:
		if ((lhs->type == MG_VALUE_NULL) || (rhs->type == MG_VALUE_NULL))
			value = mgCreateValueInteger(lhs->type != rhs->type);
		else
			switch (lhs->type)
			{
			case MG_VALUE_INTEGER:
				switch (rhs->type)
				{
				case MG_VALUE_INTEGER:
					value = mgCreateValueInteger(lhs->data.i != rhs->data.i);
					break;
				case MG_VALUE_FLOAT:
					value = mgCreateValueInteger(lhs->data.i != rhs->data.f);
					break;
				default:
					break;
				}
				break;
			case MG_VALUE_FLOAT:
				switch (rhs->type)
				{
				case MG_VALUE_INTEGER:
					value = mgCreateValueInteger(lhs->data.f != rhs->data.i);
					break;
				case MG_VALUE_FLOAT:
					value = mgCreateValueInteger(lhs->data.f != rhs->data.f);
					break;
				default:
					break;
				}
				break;
			case MG_VALUE_STRING:
				switch (rhs->type)
				{
				case MG_VALUE_STRING:
					MG_ASSERT(lhs->data.str.s);
					MG_ASSERT(rhs->data.str.s);
					value = mgCreateValueInteger(strcmp(lhs->data.str.s, rhs->data.str.s));
					break;
				default:
					break;
				}
				break;
			default:
				break;
			}
		break;
	case MG_NODE_BIN_OP_LESS:
		switch (lhs->type)
		{
		case MG_VALUE_INTEGER:
			switch (rhs->type)
			{
			case MG_VALUE_INTEGER:
				value = mgCreateValueInteger(lhs->data.i < rhs->data.i);
				break;
			case MG_VALUE_FLOAT:
				value = mgCreateValueInteger(lhs->data.i < rhs->data.f);
				break;
			default:
				break;
			}
			break;
		case MG_VALUE_FLOAT:
			switch (rhs->type)
			{
			case MG_VALUE_INTEGER:
				value = mgCreateValueInteger(lhs->data.f < rhs->data.i);
				break;
			case MG_VALUE_FLOAT:
				value = mgCreateValueInteger(lhs->data.f < rhs->data.f);
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
		break;
	case MG_NODE_BIN_OP_LESS_EQ:
		switch (lhs->type)
		{
		case MG_VALUE_INTEGER:
			switch (rhs->type)
			{
			case MG_VALUE_INTEGER:
				value = mgCreateValueInteger(lhs->data.i <= rhs->data.i);
				break;
			case MG_VALUE_FLOAT:
				value = mgCreateValueInteger(lhs->data.i <= rhs->data.f);
				break;
			default:
				break;
			}
			break;
		case MG_VALUE_FLOAT:
			switch (rhs->type)
			{
			case MG_VALUE_INTEGER:
				value = mgCreateValueInteger(lhs->data.f <= rhs->data.i);
				break;
			case MG_VALUE_FLOAT:
				value = mgCreateValueInteger(lhs->data.f <= rhs->data.f);
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
		break;
	case MG_NODE_BIN_OP_GREATER:
		switch (lhs->type)
		{
		case MG_VALUE_INTEGER:
			switch (rhs->type)
			{
			case MG_VALUE_INTEGER:
				value = mgCreateValueInteger(lhs->data.i > rhs->data.i);
				break;
			case MG_VALUE_FLOAT:
				value = mgCreateValueInteger(lhs->data.i > rhs->data.f);
				break;
			default:
				break;
			}
			break;
		case MG_VALUE_FLOAT:
			switch (rhs->type)
			{
			case MG_VALUE_INTEGER:
				value = mgCreateValueInteger(lhs->data.f > rhs->data.i);
				break;
			case MG_VALUE_FLOAT:
				value = mgCreateValueInteger(lhs->data.f > rhs->data.f);
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
		break;
	case MG_NODE_BIN_OP_GREATER_EQ:
		switch (lhs->type)
		{
		case MG_VALUE_INTEGER:
			switch (rhs->type)
			{
			case MG_VALUE_INTEGER:
				value = mgCreateValueInteger(lhs->data.i >= rhs->data.i);
				break;
			case MG_VALUE_FLOAT:
				value = mgCreateValueInteger(lhs->data.i >= rhs->data.f);
				break;
			default:
				break;
			}
			break;
		case MG_VALUE_FLOAT:
			switch (rhs->type)
			{
			case MG_VALUE_INTEGER:
				value = mgCreateValueInteger(lhs->data.f >= rhs->data.i);
				break;
			case MG_VALUE_FLOAT:
				value = mgCreateValueInteger(lhs->data.f >= rhs->data.f);
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
		break;
	case MG_NODE_BIN_OP_AND:
		switch (lhs->type)
		{
		case MG_VALUE_INTEGER:
			if (lhs->data.i)
			{
				rhs = _mgVisitNode(module, _mgListGet(node->children, 1));
				MG_ASSERT(rhs);

				switch (rhs->type)
				{
				case MG_VALUE_INTEGER:
					value = mgCreateValueInteger(rhs->data.i != 0);
					break;
				case MG_VALUE_FLOAT:
					value = mgCreateValueInteger(!_MG_FEQUAL(rhs->data.f, 0.0f));
					break;
				default:
					break;
				}
			}
			else
				value = mgCreateValueInteger(0);
			break;
		case MG_VALUE_FLOAT:
			if (!_MG_FEQUAL(lhs->data.f, 0.0f))
			{
				rhs = _mgVisitNode(module, _mgListGet(node->children, 1));
				MG_ASSERT(rhs);

				switch (rhs->type)
				{
				case MG_VALUE_INTEGER:
					value = mgCreateValueInteger(rhs->data.i != 0);
					break;
				case MG_VALUE_FLOAT:
					value = mgCreateValueInteger(!_MG_FEQUAL(rhs->data.f, 0.0f));
					break;
				default:
					break;
				}
			}
			else
				value = mgCreateValueInteger(0);
			break;
		default:
			break;
		}
		break;
	case MG_NODE_BIN_OP_OR:
		switch (lhs->type)
		{
		case MG_VALUE_INTEGER:
			if (!lhs->data.i)
			{
				rhs = _mgVisitNode(module, _mgListGet(node->children, 1));
				MG_ASSERT(rhs);

				switch (rhs->type)
				{
				case MG_VALUE_INTEGER:
					value = mgCreateValueInteger(rhs->data.i != 0);
					break;
				case MG_VALUE_FLOAT:
					value = mgCreateValueInteger(!_MG_FEQUAL(rhs->data.f, 0.0f));
					break;
				default:
					break;
				}
			}
			else
				value = mgCreateValueInteger(1);
			break;
		case MG_VALUE_FLOAT:
			if (_MG_FEQUAL(lhs->data.f, 0.0f))
			{
				rhs = _mgVisitNode(module, _mgListGet(node->children, 1));
				MG_ASSERT(rhs);

				switch (rhs->type)
				{
				case MG_VALUE_INTEGER:
					value = mgCreateValueInteger(rhs->data.i != 0);
					break;
				case MG_VALUE_FLOAT:
					value = mgCreateValueInteger(!_MG_FEQUAL(rhs->data.f, 0.0f));
					break;
				default:
					break;
				}
			}
			else
				value = mgCreateValueInteger(1);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	if (value == NULL)
	{
		if (rhs)
			MG_FAIL("Error: Unsupported binary operator \"%s\" for left-hand type \"%s\" and right-hand type \"%s\"",
			        _MG_NODE_NAMES[node->type], _MG_VALUE_TYPE_NAMES[lhs->type], _MG_VALUE_TYPE_NAMES[rhs->type]);
		else
			MG_FAIL("Error: Unsupported binary operator \"%s\" for left-hand type \"%s\"",
			        _MG_NODE_NAMES[node->type], _MG_VALUE_TYPE_NAMES[lhs->type]);
	}

	MG_ASSERT(value);

	mgDestroyValue(lhs);

	if (rhs)
		mgDestroyValue(rhs);

	return value;
}


static MGValue* _mgVisitUnaryOp(MGValue *module, MGNode *node)
{
	MG_ASSERT(_mgListLength(node->children) == 1);
	MG_ASSERT(node->type != MG_NODE_INVALID);

	MGValue *operand = _mgVisitNode(module, _mgListGet(node->children, 0));
	MG_ASSERT(operand);

	MGValue *value = NULL;

	switch (node->type)
	{
	case MG_NODE_UNARY_OP_POS:
		switch (operand->type)
		{
		case MG_VALUE_INTEGER:
			value = mgCreateValueInteger(+operand->data.i);
			break;
		case MG_VALUE_FLOAT:
			value = mgCreateValueFloat(+operand->data.f);
			break;
		default:
			break;
		}
		break;
	case MG_NODE_UNARY_OP_NEG:
		switch (operand->type)
		{
		case MG_VALUE_INTEGER:
			value = mgCreateValueInteger(-operand->data.i);
			break;
		case MG_VALUE_FLOAT:
			value = mgCreateValueFloat(-operand->data.f);
			break;
		default:
			break;
		}
		break;
	case MG_NODE_UNARY_OP_NOT:
		switch (operand->type)
		{
		case MG_VALUE_INTEGER:
			value = mgCreateValueInteger(!operand->data.i);
			break;
		case MG_VALUE_FLOAT:
			value = mgCreateValueInteger(!operand->data.f);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	if (value == NULL)
		MG_FAIL("Error: Unsupported unary operator \"%s\" for type \"%s\"", _MG_NODE_NAMES[node->type], _MG_VALUE_TYPE_NAMES[operand->type]);

	mgDestroyValue(operand);

	return value;
}


static void _mgResolveImportAs(MGValue *module, MGToken *name, MGToken *alias)
{
	MG_ASSERT(name);
	MG_ASSERT(name->type == MG_TOKEN_NAME);
	MG_ASSERT(alias);
	MG_ASSERT(alias->type == MG_TOKEN_NAME);

	const size_t _nameLength = name->end.string - name->begin.string;
	MG_ASSERT(_nameLength > 0);
	char *_name = mgStringDuplicateFixed(name->begin.string, _nameLength);
	MG_ASSERT(_name);

	const size_t _aliasLength = alias->end.string - alias->begin.string;
	MG_ASSERT(_aliasLength > 0);
	char *_alias = mgStringDuplicateFixed(alias->begin.string, _aliasLength);
	MG_ASSERT(_alias);

	MGValue *importedModule = mgImportModule(module->data.module.instance, _name);
	MG_ASSERT(importedModule);
	MG_ASSERT(importedModule->type == MG_VALUE_MODULE);

	_mgSetValue(module, _alias, importedModule);

	free(_alias);
	free(_name);
}


static MGValue* _mgVisitImport(MGValue *module, MGNode *node)
{
	MG_ASSERT(module);
	MG_ASSERT(module->type == MG_VALUE_MODULE);
	MG_ASSERT(node);
	MG_ASSERT((node->type == MG_NODE_IMPORT) || (node->type == MG_NODE_IMPORT_FROM));
	MG_ASSERT(_mgListLength(node->children) > 0);

	if (node->type == MG_NODE_IMPORT)
	{
		for (size_t i = 0; i < _mgListLength(node->children); ++i)
		{
			const MGNode *const nameNode = _mgListGet(node->children, i);
			MG_ASSERT((nameNode->type == MG_NODE_NAME) || (nameNode->type == MG_NODE_AS));

			if (nameNode->type == MG_NODE_NAME)
				_mgResolveImportAs(module, nameNode->token, nameNode->token);
			else if (nameNode->type == MG_NODE_AS)
			{
				MG_ASSERT(_mgListLength(nameNode->children) == 2);
				_mgResolveImportAs(module, _mgListGet(nameNode->children, 0)->token, _mgListGet(nameNode->children, 1)->token);
			}
		}
	}
	else if (node->type == MG_NODE_IMPORT_FROM)
	{
		const MGNode *nameNode = _mgListGet(node->children, 0);
		const MGNode *aliasNode = NULL;
		MG_ASSERT(nameNode->type == MG_NODE_NAME);
		MG_ASSERT(nameNode->token);

		size_t nameLength = nameNode->token->end.string - nameNode->token->begin.string;
		MG_ASSERT(nameLength > 0);
		char *name = mgStringDuplicateFixed(nameNode->token->begin.string, nameLength);
		MG_ASSERT(name);

		size_t aliasLength = 0;
		char *alias = NULL;

		size_t nameCapacity = 0;
		size_t aliasCapacity = 0;

		MGValue *importedModule = mgImportModule(module->data.module.instance, name);
		MG_ASSERT(importedModule);
		MG_ASSERT(importedModule->type == MG_VALUE_MODULE);

		if (_mgListLength(node->children) > 1)
		{
			for (size_t i = 1; i < _mgListLength(node->children); ++i)
			{
				nameNode = _mgListGet(node->children, i);
				aliasNode = NULL;

				MG_ASSERT((nameNode->type == MG_NODE_NAME) || (nameNode->type == MG_NODE_AS));

				if (nameNode->type == MG_NODE_AS)
				{
					MG_ASSERT(_mgListLength(nameNode->children) == 2);

					aliasNode = _mgListGet(nameNode->children, 1);
					nameNode = _mgListGet(nameNode->children, 0);
				}

				MG_ASSERT(nameNode->type == MG_NODE_NAME);
				MG_ASSERT(nameNode->token);

				nameLength = nameNode->token->end.string - nameNode->token->begin.string;
				MG_ASSERT(nameLength > 0);

				if (nameLength >= nameCapacity)
				{
					nameCapacity = nameLength + 1;
					name = (char*) realloc(name, nameCapacity * sizeof(char));
					MG_ASSERT(name);
				}

				strncpy(name, nameNode->token->begin.string, nameLength);
				name[nameLength] = '\0';

				if (aliasNode)
				{
					MG_ASSERT(aliasNode->type == MG_NODE_NAME);
					MG_ASSERT(aliasNode->token);

					aliasLength = aliasNode->token->end.string - aliasNode->token->begin.string;
					MG_ASSERT(aliasLength > 0);

					if (aliasLength >= aliasCapacity)
					{
						aliasCapacity = aliasLength + 1;
						alias = (char*) realloc(alias, aliasCapacity * sizeof(char));
						MG_ASSERT(alias);
					}

					strncpy(alias, aliasNode->token->begin.string, aliasLength);
					alias[aliasLength] = '\0';
				}

				MGValue *value = mgModuleGet(importedModule, name);

				if (!value)
					_MG_FAIL(module, NULL, "Error: Undefined name \"%s\"", name);

				if (aliasNode)
					_mgSetValue(module, alias, mgReferenceValue(value));
				else
					_mgSetValue(module, name, mgReferenceValue(value));
			}
		}
		else
		{
			MGMapIterator iterator;
			mgCreateMapIterator(&iterator, importedModule->data.module.globals);

			MGValue *k, *v;
			while (mgMapNext(&iterator, &k, &v))
				_mgSetValue(module, k->data.str.s, mgReferenceValue(v));

			mgDestroyMapIterator(&iterator);
		}

		free(alias);
		free(name);
	}

	return mgCreateValueNull();
}


static MGValue* _mgVisitAssert(MGValue *module, MGNode *node)
{
#if MG_DEBUG
	MG_ASSERT(node);
	MG_ASSERT(node->type == MG_NODE_ASSERT);
	MG_ASSERT((_mgListLength(node->children) == 1) || (_mgListLength(node->children) == 2));

	MGValue *expression = _mgVisitNode(module, _mgListGet(node->children, 0));
	MG_ASSERT(expression->type == MG_VALUE_INTEGER);

	if (!expression->data.i)
	{
		if (_mgListLength(node->children) == 2)
		{
			MGValue *message = _mgVisitNode(module, _mgListGet(node->children, 1));
			MG_ASSERT(message->type == MG_VALUE_STRING);

			MG_FAIL("Error: %s", message->data.str.s);

			mgDestroyValue(message);
		}
		else
			MG_FAIL("Error: Assertion");
	}

	mgDestroyValue(expression);

#endif

	return mgCreateValueNull();
}


static MGValue* _mgVisitNode(MGValue *module, MGNode *node)
{
	switch (node->type)
	{
	case MG_NODE_MODULE:
		return _mgVisitModule(module, node);
	case MG_NODE_BLOCK:
		return _mgVisitChildren(module, node);
	case MG_NODE_NAME:
		return _mgVisitName(module, node);
	case MG_NODE_INTEGER:
		return _mgVisitInteger(module, node);
	case MG_NODE_FLOAT:
		return _mgVisitFloat(module, node);
	case MG_NODE_STRING:
		return _mgVisitString(module, node);
	case MG_NODE_TUPLE:
	case MG_NODE_LIST:
		return _mgVisitTuple(module, node);
	case MG_NODE_RANGE:
		return _mgVisitRange(module, node);
	case MG_NODE_MAP:
		return _mgVisitMap(module, node);
	case MG_NODE_BIN_OP_ADD:
	case MG_NODE_BIN_OP_SUB:
	case MG_NODE_BIN_OP_MUL:
	case MG_NODE_BIN_OP_DIV:
	case MG_NODE_BIN_OP_INT_DIV:
	case MG_NODE_BIN_OP_MOD:
	case MG_NODE_BIN_OP_COALESCE:
	case MG_NODE_BIN_OP_EQ:
	case MG_NODE_BIN_OP_NOT_EQ:
	case MG_NODE_BIN_OP_LESS:
	case MG_NODE_BIN_OP_GREATER:
	case MG_NODE_BIN_OP_LESS_EQ:
	case MG_NODE_BIN_OP_GREATER_EQ:
	case MG_NODE_BIN_OP_AND:
	case MG_NODE_BIN_OP_OR:
		return _mgVisitBinOp(module, node);
	case MG_NODE_UNARY_OP_POS:
	case MG_NODE_UNARY_OP_NEG:
	case MG_NODE_UNARY_OP_NOT:
		return _mgVisitUnaryOp(module, node);
	case MG_NODE_ASSIGN:
		return _mgVisitAssignment(module, node);
	case MG_NODE_CALL:
		return _mgVisitCall(module, node);
	case MG_NODE_FOR:
		return _mgVisitFor(module, node);
	case MG_NODE_IF:
		return _mgVisitIf(module, node);
	case MG_NODE_PROCEDURE:
	case MG_NODE_FUNCTION:
		return _mgVisitFunction(module, node);
	case MG_NODE_EMIT:
		return _mgVisitEmit(module, node);
	case MG_NODE_RETURN:
		return _mgVisitReturn(module, node);
	case MG_NODE_DELETE:
		return _mgVisitDelete(module, node);
	case MG_NODE_SUBSCRIPT:
		return _mgVisitSubscript(module, node);
	case MG_NODE_ATTRIBUTE:
		return _mgVisitAttribute(module, node);
	case MG_NODE_IMPORT:
	case MG_NODE_IMPORT_FROM:
		return _mgVisitImport(module, node);
	case MG_NODE_ASSERT:
		return _mgVisitAssert(module, node);
	case MG_NODE_NULL:
		return _mgVisitNull(module, node);
	default:
		MG_FAIL("Error: Unknown node \"%s\"", _MG_NODE_NAMES[node->type]);
	}

	return NULL;
}


inline MGValue* mgInterpret(MGValue *module)
{
	MG_ASSERT(module);
	MG_ASSERT(module->type == MG_VALUE_MODULE);
	MG_ASSERT(module->data.module.instance);
	MG_ASSERT(module->data.module.parser.root);

	return _mgVisitNode(module, module->data.module.parser.root);
}


MGValue* mgInterpretFile(MGValue *module, const char *filename)
{
	MG_ASSERT(module);
	MG_ASSERT(filename);

	if (module->data.module.filename)
	{
		free(module->data.module.filename);
		module->data.module.filename = NULL;
	}

	module->data.module.filename = mgStringDuplicate(filename);

	mgDestroyParser(&module->data.module.parser);
	mgCreateParser(&module->data.module.parser);

	MGValue *result = NULL;
	if (mgParseFile(&module->data.module.parser, filename))
		result = mgInterpret(module);

	return result;
}


MGValue* mgInterpretFileHandle(MGValue *module, FILE *file, const char *filename)
{
	MG_ASSERT(module);
	MG_ASSERT(file);

	if (module->data.module.filename)
	{
		free(module->data.module.filename);
		module->data.module.filename = NULL;
	}

	if (filename)
		module->data.module.filename = mgStringDuplicate(filename);

	mgDestroyParser(&module->data.module.parser);
	mgCreateParser(&module->data.module.parser);

	MGValue *result = NULL;
	if (mgParseFileHandle(&module->data.module.parser, file))
		result = mgInterpret(module);

	return result;
}


MGValue* mgInterpretString(MGValue *module, const char *string, const char *filename)
{
	MG_ASSERT(module);
	MG_ASSERT(string);

	if (module->data.module.filename)
	{
		free(module->data.module.filename);
		module->data.module.filename = NULL;
	}

	if (filename)
		module->data.module.filename = mgStringDuplicate(filename);

	mgDestroyParser(&module->data.module.parser);
	mgCreateParser(&module->data.module.parser);

	MGValue *result = NULL;
	if (mgParseString(&module->data.module.parser, string))
		result = mgInterpret(module);

	return result;
}
