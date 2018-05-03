
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "modelgen.h"
#include "value.h"
#include "module.h"
#include "callable.h"
#include "inspect.h"
#include "utilities.h"


extern MGNode* mgReferenceNode(const MGNode *node);

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
		fputc(' ', stderr);

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


static const MGBinOpType _MG_AUG_TO_BIN_OP_TYPES[] = {
	MG_BIN_OP_ADD,
	MG_BIN_OP_SUB,
	MG_BIN_OP_MUL,
	MG_BIN_OP_DIV,
	MG_BIN_OP_INT_DIV,
	MG_BIN_OP_MOD,
};


MGValue* _mgVisitNode(MGValue *module, MGNode *node);


void _mgSetLocalValue(MGValue *module, const char *name, MGValue *value)
{
	MG_ASSERT(module);
	MG_ASSERT(module->type == MG_TYPE_MODULE);
	MG_ASSERT(module->data.module.instance);
	MG_ASSERT(module->data.module.instance->callStackTop);
	MG_ASSERT(module->data.module.instance->callStackTop->locals);
	MG_ASSERT(name);

	mgMapSet(module->data.module.instance->callStackTop->locals, name, value);
}


void _mgSetValue(MGValue *module, const char *name, MGValue *value)
{
	MG_ASSERT(module);
	MG_ASSERT(module->type == MG_TYPE_MODULE);
	MG_ASSERT(module->data.module.instance);
	MG_ASSERT(module->data.module.instance->callStackTop);
	MG_ASSERT(module->data.module.instance->callStackTop->locals);
	MG_ASSERT(name);

	if (mgMapGet(module->data.module.instance->callStackTop->locals, name) || !mgModuleGet(module, name))
		_mgSetLocalValue(module, name, value);
	else
		mgModuleSet(module, name, value);
}


const MGValue* _mgGetValue(MGValue *module, const char *name)
{
	MG_ASSERT(module);
	MG_ASSERT(module->type == MG_TYPE_MODULE);
	MG_ASSERT(module->data.module.instance);
	MG_ASSERT(module->data.module.instance->callStackTop);
	MG_ASSERT(module->data.module.instance->callStackTop->locals);
	MG_ASSERT(name);

	MGValue *value = mgMapGet(module->data.module.instance->callStackTop->locals, name);

	if (!value)
		value = mgModuleGet(module, name);

	if (!value)
		value = mgModuleGet(module->data.module.instance->base, name);

	if (!value)
		_MG_FAIL(module, NULL, "Error: Undefined name \"%s\"", name);

	return value;
}


static inline MGValue* _mgResolveSubscriptGet(MGValue *module, MGNode *node, MGValue *collection, MGValue *index)
{
	MGValue *value;

	if (!(value = mgValueSubscriptGet(collection, index)))
		MG_FAIL("Error: %s is not subscriptable with %s",
		        mgGetTypeName(collection->type), mgGetTypeName(index->type));

	return value;
}


static inline void _mgResolveSubscriptSet(MGValue *module, MGNode *node, MGValue *collection, MGValue *index, MGValue *value)
{
	if (!mgValueSubscriptSet(collection, index, value))
		MG_FAIL("Error: %s is not subscriptable with %s",
		        mgGetTypeName(collection->type), mgGetTypeName(index->type));
}


static inline MGValue* _mgResolveAttributeGet(MGValue *module, MGNode *node, MGValue *collection, const char *key)
{
	MGValue *value;

	if (!(value = mgValueAttributeGet(collection, key)))
		MG_FAIL("Error: %s has no attribute %s",
		        mgGetTypeName(collection->type), key);

	return value;
}


static inline void _mgResolveAttributeSet(MGValue *module, MGNode *node, MGValue *collection, const char *key, MGValue *value)
{
	if (!mgValueAttributeSet(collection, key, value))
		MG_FAIL("Error: %s has no attribute %s",
		        mgGetTypeName(collection->type), key);
}


static void _mgResolveAssignment(MGValue *module, MGNode *names, MGValue *values, MGbool local)
{
	MG_ASSERT(module);
	MG_ASSERT(module->type == MG_TYPE_MODULE);
	MG_ASSERT(names);
	MG_ASSERT((names->type == MG_NODE_NAME) || (names->type == MG_NODE_SUBSCRIPT) || (names->type == MG_NODE_ATTRIBUTE) || (names->type == MG_NODE_TUPLE));
	MG_ASSERT(values);

	if (names->type == MG_NODE_NAME)
	{
		MG_ASSERT(names->token);

		if (local)
			_mgSetLocalValue(module, names->token->value.s, mgReferenceValue(values));
		else
			_mgSetValue(module, names->token->value.s, mgReferenceValue(values));
	}
	else if (names->type == MG_NODE_SUBSCRIPT)
	{
		MG_ASSERT(_mgListLength(names->children) == 2);

		MGValue *index = _mgVisitNode(module, _mgListGet(names->children, 1));
		MGValue *collection = _mgVisitNode(module, _mgListGet(names->children, 0));

		MG_ASSERT(collection);
		MG_ASSERT(index);

		_mgResolveSubscriptSet(module, names, collection, index, mgReferenceValue(values));

		mgDestroyValue(collection);
		mgDestroyValue(index);
	}
	else if (names->type == MG_NODE_ATTRIBUTE)
	{
		MG_ASSERT(_mgListLength(names->children) == 2);

		MGValue *collection = _mgVisitNode(module, _mgListGet(names->children, 0));
		MG_ASSERT(collection);

		MGNode *attributeNode = _mgListGet(names->children, 1);
		MG_ASSERT(attributeNode->type == MG_NODE_NAME);
		MG_ASSERT(attributeNode->token);

		if (!mgValueAttributeSet(collection, attributeNode->token->value.s, mgReferenceValue(values)))
			mgDestroyValue(values);

		mgDestroyValue(collection);
	}
	else if (names->type == MG_NODE_TUPLE)
	{
		if ((values->type != MG_TYPE_TUPLE) && (values->type != MG_TYPE_LIST))
			_MG_FAIL(module, names, "Error: %s is not iterable", mgGetTypeName(values->type));

		if (_mgListLength(names->children) != mgListLength(values))
			_MG_FAIL(module, names, "Error: Mismatched lengths for parallel assignment (%zu != %zu)", _mgListLength(names->children), mgListLength(values));

		for (size_t i = 0; i < _mgListLength(names->children); ++i)
			_mgResolveAssignment(module, _mgListGet(names->children, i), _mgListGet(values->data.a, i), local);
	}
}


static MGValue* _mgVisitChildren(MGValue *module, MGNode *node)
{
	MG_ASSERT(module);
	MG_ASSERT(module->type == MG_TYPE_MODULE);
	MG_ASSERT(module->data.module.instance);
	MG_ASSERT(module->data.module.instance->callStackTop);

	MGStackFrame *frame = module->data.module.instance->callStackTop;

	for (size_t i = 0; i < _mgListLength(node->children); ++i)
	{
		mgDestroyValue(_mgVisitNode(module, _mgListGet(node->children, i)));

		if (frame->state != MG_STACK_FRAME_STATE_ACTIVE)
			return frame->value ? mgReferenceValue(frame->value) : MG_NULL_VALUE;
	}

	return MG_NULL_VALUE;
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

	MGInstance *instance = module->data.module.instance;

	MGNode *nameNode = _mgListGet(node->children, 0);

	const MGValue *func = NULL;
	const char *name = NULL;

	if (nameNode->type == MG_NODE_NAME)
	{
		MG_ASSERT(nameNode->token);

		name = nameNode->token->value.s;
		func = _mgGetValue(module, name);

		if (!func)
			MG_FAIL("Error: Undefined name \"%s\"", name);
	}
	else
		func = _mgVisitNode(module, nameNode);

	MG_ASSERT(func);

	if (name == NULL)
		name = "<anonymous>";

	_MGList(MGValue*) args;
	_mgListCreate(MGValue*, args, _mgListLength(node->children) - 1);

	for (size_t i = 0; i < _mgListCapacity(args); ++i)
	{
		_mgListAdd(MGValue*, args, _mgVisitNode(module, _mgListGet(node->children, i + 1)));
		MG_ASSERT(_mgListGet(args, i));
	}

	MGStackFrame frame;

	if (((func->type == MG_TYPE_FUNCTION) || (func->type == MG_TYPE_PROCEDURE)) && func->data.func.locals)
		mgCreateStackFrameEx(&frame, mgReferenceValue(module), mgReferenceValue(func->data.func.locals));
	else
		mgCreateStackFrame(&frame, mgReferenceValue(module));

	frame.caller = node;
	frame.callerName = name;

	mgPushStackFrame(instance, &frame);

	MGValue *value = mgCallEx(instance, &frame, func, _mgListLength(args), (const MGValue* const*) _mgListItems(args));

	mgPopStackFrame(instance, &frame);
	mgDestroyStackFrame(&frame);

	for (size_t i = 0; i < _mgListLength(args); ++i)
		mgDestroyValue(_mgListGet(args, i));
	_mgListDestroy(args);

	MG_ASSERT(value);

	return value;
}


static MGValue* _mgVisitEmit(MGValue *module, MGNode *node)
{
	MG_ASSERT(module);
	MG_ASSERT(module->data.module.instance);
	MG_ASSERT(node->type == MG_NODE_EMIT);
	MG_ASSERT(_mgListLength(node->children) == 1);

	MGInstance *instance = module->data.module.instance;

	const unsigned int vertexSize = mgInstanceGetVertexSize(instance);

	MGValue *tuple = _mgVisitNode(module, _mgListGet(node->children, 0));
	MG_ASSERT(tuple);

	if (tuple->type != MG_TYPE_TUPLE)
		MG_FAIL("Error: Expected \"%s\", received \"%s\"",
		        mgGetTypeName(MG_TYPE_TUPLE), mgGetTypeName(tuple->type));
	else if (mgTupleLength(tuple) != vertexSize)
		MG_FAIL("Error: Expected tuple with a length of %u, received a tuple with a length of %zu",
		        vertexSize, mgTupleLength(tuple));

	const size_t vertexCount = _mgListLength(instance->vertices);

	_mgListAddUninitialized(MGVertex, instance->vertices);
	MGVertex *vertices = _mgListItems(instance->vertices);

	for (unsigned int i = 0; i < vertexSize; ++i)
	{
		if (mgTupleGet(tuple, i)->type == MG_TYPE_INTEGER)
			vertices[vertexCount][i] = (float) mgTupleGet(tuple, i)->data.i;
		else if (mgTupleGet(tuple, i)->type == MG_TYPE_FLOAT)
			vertices[vertexCount][i] = mgTupleGet(tuple, i)->data.f;
		else
			MG_FAIL("Error: Expected \"%s\" or \"%s\", received \"%s\"",
			        mgGetTypeName(MG_TYPE_INTEGER), mgGetTypeName(MG_TYPE_FLOAT),
			        mgGetTypeName(tuple->type));
	}

	++_mgListLength(instance->vertices);

	mgDestroyValue(tuple);

	return MG_NULL_VALUE;
}


static inline MGValue* _mgVisitReturn(MGValue *module, MGNode *node)
{
	MG_ASSERT(module);
	MG_ASSERT(module->data.module.instance);
	MG_ASSERT(module->data.module.instance->callStackTop);

	MGStackFrame *frame = module->data.module.instance->callStackTop;

	frame->state = MG_STACK_FRAME_STATE_RETURN;

	if (_mgListLength(node->children) > 0)
		frame->value = _mgVisitNode(module, _mgListGet(node->children, 0));

	return frame->value ? mgReferenceValue(frame->value) : MG_NULL_VALUE;
}


static void _mgDelete(MGValue *module, MGNode *node)
{
	if (node->type == MG_NODE_NAME)
	{
		MG_ASSERT(node->token);

#if MG_DEBUG
		// Check if the name is defined
		_mgGetValue(module, node->token->value.s);
#endif

		_mgSetValue(module, node->token->value.s, NULL);
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

		MGValue *collection = _mgVisitNode(module, _mgListGet(node->children, 0));
		MG_ASSERT(collection);

		const MGNode *attributeNode = _mgListGet(node->children, 1);
		MG_ASSERT(attributeNode->type == MG_NODE_NAME);
		MG_ASSERT(attributeNode->token);

#if MG_DEBUG
		// Check if the name is defined
		mgDestroyValue(_mgResolveAttributeGet(module, node, collection, attributeNode->token->value.s));
#endif

		_mgResolveAttributeSet(module, node, collection, attributeNode->token->value.s, NULL);

		mgDestroyValue(collection);
	}
	else
		MG_FAIL("Error: Cannot delete \"%s\"", _MG_NODE_NAMES[node->type]);
}


static inline MGValue* _mgVisitDelete(MGValue *module, MGNode *node)
{
	MG_ASSERT(_mgListLength(node->children) == 1);

	_mgDelete(module, _mgListGet(node->children, 0));

	return MG_NULL_VALUE;
}


static MGValue* _mgVisitFor(MGValue *module, MGNode *node)
{
	MG_ASSERT(module);
	MG_ASSERT(module->data.module.instance);
	MG_ASSERT(_mgListLength(node->children) >= 2);

	MGNode *name = _mgListGet(node->children, 0);
	MG_ASSERT(name);

	MGValue *iterable = _mgVisitNode(module, _mgListGet(node->children, 1));
	MG_ASSERT(iterable);
	MG_ASSERT((iterable->type == MG_TYPE_TUPLE) || (iterable->type == MG_TYPE_LIST));

	MGStackFrame *frame = module->data.module.instance->callStackTop;

	MGValue *value = NULL;

	for (size_t i = 0; i < _mgListLength(iterable->data.a); ++i)
	{
		value = _mgListGet(iterable->data.a, i);
		MG_ASSERT(value);

		_mgResolveAssignment(module, name, value, MG_TRUE);

		for (size_t j = 2; j < _mgListLength(node->children); ++j)
		{
			MGValue *result = _mgVisitNode(module, _mgListGet(node->children, j));
			MG_ASSERT(result);
			mgDestroyValue(result);

			if (frame->state == MG_STACK_FRAME_STATE_RETURN)
			{
				value = frame->value ? mgReferenceValue(frame->value) : MG_NULL_VALUE;
				goto end;
			}
			else if (frame->state == MG_STACK_FRAME_STATE_BREAK)
			{
				frame->state = MG_STACK_FRAME_STATE_ACTIVE;
				value = frame->value ? mgReferenceValue(frame->value) : mgReferenceValue(value);
				goto end;
			}
			else if (frame->state == MG_STACK_FRAME_STATE_CONTINUE)
			{
				frame->state = MG_STACK_FRAME_STATE_ACTIVE;
				break;
			}
		}
	}

	if (value)
		value = mgReferenceValue(value);

end:

	mgDestroyValue(iterable);

	return value ? value : MG_NULL_VALUE;
}


static MGValue* _mgVisitWhile(MGValue *module, MGNode *node)
{
	MG_ASSERT(_mgListLength(node->children) <= 2);

	MGStackFrame *frame = module->data.module.instance->callStackTop;

	for (;;)
	{
		MGValue *condition = _mgVisitNode(module, _mgListGet(node->children, 0));
		MG_ASSERT(condition);
		MG_ASSERT(condition->type == MG_TYPE_INTEGER);

		if (!condition->data.i)
		{
			mgDestroyValue(condition);
			break;
		}

		mgDestroyValue(condition);

		for (size_t j = 1; j < _mgListLength(node->children); ++j)
		{
			mgDestroyValue(_mgVisitNode(module, _mgListGet(node->children, j)));

			if (frame->state == MG_STACK_FRAME_STATE_RETURN)
				return frame->value ? mgReferenceValue(frame->value) : MG_NULL_VALUE;
			else if (frame->state == MG_STACK_FRAME_STATE_BREAK)
			{
				frame->state = MG_STACK_FRAME_STATE_ACTIVE;
				return frame->value ? mgReferenceValue(frame->value) : MG_NULL_VALUE;
			}
			else if (frame->state == MG_STACK_FRAME_STATE_CONTINUE)
			{
				frame->state = MG_STACK_FRAME_STATE_ACTIVE;
				break;
			}
		}
	}

	return MG_NULL_VALUE;
}


static inline MGValue* _mgVisitBreak(MGValue *module, MGNode *node)
{
	MG_ASSERT(module);
	MG_ASSERT(module->data.module.instance);
	MG_ASSERT(module->data.module.instance->callStackTop);
	MG_ASSERT(_mgListLength(node->children) < 2);

	MGStackFrame *frame = module->data.module.instance->callStackTop;

	frame->state = MG_STACK_FRAME_STATE_BREAK;

	if (_mgListLength(node->children) > 0)
		frame->value = _mgVisitNode(module, _mgListGet(node->children, 0));

	return frame->value ? mgReferenceValue(frame->value) : MG_NULL_VALUE;
}


static inline MGValue* _mgVisitContinue(MGValue *module, MGNode *node)
{
	MG_ASSERT(module);
	MG_ASSERT(module->data.module.instance);
	MG_ASSERT(module->data.module.instance->callStackTop);
	MG_ASSERT(_mgListLength(node->children) == 0);

	MGStackFrame *frame = module->data.module.instance->callStackTop;

	frame->state = MG_STACK_FRAME_STATE_CONTINUE;

	return MG_NULL_VALUE;
}


static MGValue* _mgVisitIf(MGValue *module, MGNode *node)
{
	MG_ASSERT(_mgListLength(node->children) > 0);

	MGValue *condition = _mgVisitNode(module, _mgListGet(node->children, 0));
	MG_ASSERT(condition);

	MGbool _condition = mgValueTruthValue(condition);

	mgDestroyValue(condition);

	if (_mgListLength(node->children) > 1)
	{
		if (_condition)
			return _mgVisitNode(module, _mgListGet(node->children, 1));
		else if (_mgListLength(node->children) > 2)
			return _mgVisitNode(module, _mgListGet(node->children, 2));
	}

	return mgCreateValueInteger(_condition);
}


static MGValue* _mgVisitFunction(MGValue *module, MGNode *node)
{
	MG_ASSERT(module);
	MG_ASSERT(module->data.module.instance);
	MG_ASSERT((_mgListLength(node->children) == 2) || (_mgListLength(node->children) == 3));

	MGNode *nameNode = _mgListGet(node->children, 0);
	MG_ASSERT((nameNode->type == MG_NODE_INVALID) || (nameNode->type == MG_NODE_NAME) || (nameNode->type == MG_NODE_ATTRIBUTE));

	MGValue *func = mgCreateValueEx((node->type == MG_NODE_FUNCTION) ? MG_TYPE_FUNCTION : MG_TYPE_PROCEDURE);

	func->data.func.module = mgReferenceValue(module);
	func->data.func.node = mgReferenceNode(node);

	MGbool isClosure = MG_FALSE;

	if (module->data.module.instance->callStackTop && module->data.module.instance->callStackTop->last)
	{
		for (MGNode *parent = node->parent; parent; parent = parent->parent)
		{
			if ((parent->type == MG_NODE_FUNCTION) || (parent->type == MG_NODE_PROCEDURE))
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

		_mgSetValue(module, nameNode->token->value.s, mgReferenceValue(func));
	}
	else if (nameNode->type == MG_NODE_ATTRIBUTE)
	{
		MG_ASSERT(_mgListLength(nameNode->children) == 2);

		MGValue *collection = _mgVisitNode(module, _mgListGet(nameNode->children, 0));
		MG_ASSERT(collection);

		MGNode *attributeNode = _mgListGet(nameNode->children, 1);
		MG_ASSERT(attributeNode->type == MG_NODE_NAME);
		MG_ASSERT(attributeNode->token);

		if (!mgValueAttributeSet(collection, attributeNode->token->value.s, mgReferenceValue(func)))
			mgDestroyValue(func);

		mgDestroyValue(collection);
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
	MG_ASSERT(value);

	mgDestroyValue(collection);
	mgDestroyValue(index);

	return value;
}


static MGValue* _mgVisitAttribute(MGValue *module, MGNode *node)
{
	MG_ASSERT(_mgListLength(node->children) == 2);

	MGValue *collection = _mgVisitNode(module, _mgListGet(node->children, 0));
	MG_ASSERT(collection);

	const MGNode *attribute = _mgListGet(node->children, 1);
	MG_ASSERT(attribute);
	MG_ASSERT(attribute->type == MG_NODE_NAME);
	MG_ASSERT(attribute->token);

	MGValue *value = _mgResolveAttributeGet(module, node, collection, attribute->token->value.s);
	MG_ASSERT(value);

	mgDestroyValue(collection);

	return value;
}


#if defined(__GNUC__)
static inline __attribute__((always_inline)) MGValue* _mgVisitName(MGValue *module, MGNode *node)
#elif defined(_MSC_VER)
static __forceinline MGValue* _mgVisitName(MGValue *module, MGNode *node)
#else
static inline MGValue* _mgVisitName(MGValue *module, MGNode *node)
#endif
{
	MG_ASSERT(node->token);

	return mgReferenceValue(_mgGetValue(module, node->token->value.s));
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
	value->type = (node->type == MG_NODE_TUPLE) ? MG_TYPE_TUPLE : MG_TYPE_LIST;

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
	MG_ASSERT(start->type == MG_TYPE_INTEGER);

	stop = _mgVisitNode(module, _mgListGet(node->children, 1));
	MG_ASSERT(stop);
	MG_ASSERT(stop->type == MG_TYPE_INTEGER);

	if (_mgListLength(node->children) == 3)
	{
		step = _mgVisitNode(module, _mgListGet(node->children, 2));
		MG_ASSERT(step);
		MG_ASSERT(step->type == MG_TYPE_INTEGER);
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
		MG_ASSERT(key->type == MG_TYPE_STRING);
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

	_mgResolveAssignment(module, lhs, value, MG_FALSE);

	return value;
}


static inline MGValue* _mgVisitAugmentedAssignment(MGValue *module, MGNode *node)
{
	MG_ASSERT(_mgListLength(node->children) == 2);

	MGValue *rhs = _mgVisitNode(module, _mgListGet(node->children, 1));
	MG_ASSERT(rhs);

	MGNode *lhsNode = _mgListGet(node->children, 0);

	MGValue *lhs = NULL;
	MGValue *lhsCollection = NULL, *lhsIndex = NULL;
	const MGNode *lhsAttributeNode = NULL;

	switch (lhsNode->type)
	{
	case MG_NODE_NAME:
		lhs = _mgVisitName(module, lhsNode);
		break;
	case MG_NODE_SUBSCRIPT:
		lhsIndex = _mgVisitNode(module, _mgListGet(lhsNode->children, 1));
		lhsCollection = _mgVisitNode(module, _mgListGet(lhsNode->children, 0));
		lhs = _mgResolveSubscriptGet(module, lhsNode, lhsCollection, lhsIndex);
		break;
	case MG_NODE_ATTRIBUTE:
		lhsAttributeNode = _mgListGet(lhsNode->children, 1);
		lhsCollection = _mgVisitNode(module, _mgListGet(lhsNode->children, 0));
		lhs = _mgResolveAttributeGet(module, lhsNode, lhsCollection, lhsAttributeNode->token->value.s);
		break;
	default:
		MG_FAIL("Error: Unsupported augmented assignment with \"%s\"", _MG_NODE_NAMES[lhsNode->type]);
	}

	MGValue *value = mgValueBinaryOp(lhs, rhs, _MG_AUG_TO_BIN_OP_TYPES[node->type - MG_NODE_ASSIGN_ADD]);

	switch (lhsNode->type)
	{
	case MG_NODE_NAME:
		_mgSetValue(module, lhsNode->token->value.s, mgReferenceValue(value));
		break;
	case MG_NODE_SUBSCRIPT:
		_mgResolveSubscriptSet(module, lhsNode, lhsCollection, lhsIndex, mgReferenceValue(value));
		mgDestroyValue(lhsCollection);
		mgDestroyValue(lhsIndex);
		break;
	case MG_NODE_ATTRIBUTE:
		if (!mgValueAttributeSet(lhsCollection, lhsAttributeNode->token->value.s, mgReferenceValue(value)))
			mgDestroyValue(value);
		mgDestroyValue(lhsCollection);
		break;
	default:
		break;
	}

	mgDestroyValue(lhs);
	mgDestroyValue(rhs);

	return value;
}


static inline MGValue* _mgVisitUnaryOp(MGValue *module, MGNode *node, MGUnaryOpType operation)
{
	MG_ASSERT(_mgListLength(node->children) == 1);

	MGValue *operand = _mgVisitNode(module, _mgListGet(node->children, 0));
	MG_ASSERT(operand);

	MGValue *value = mgValueUnaryOp(operand, operation);
	MG_ASSERT(value);

	mgDestroyValue(operand);

	return value;
}


static inline MGValue* mgVisitBinOp(MGValue *module, MGNode *node, MGBinOpType operation)
{
	MG_ASSERT(_mgListLength(node->children) == 2);
	MG_ASSERT(node->type != MG_NODE_INVALID);

	MGValue *lhs = _mgVisitNode(module, _mgListGet(node->children, 0));
	MGValue *rhs = _mgVisitNode(module, _mgListGet(node->children, 1));
	MG_ASSERT(lhs);
	MG_ASSERT(rhs);

	MGValue *value = mgValueBinaryOp(lhs, rhs, operation);

	mgDestroyValue(lhs);
	mgDestroyValue(rhs);

	return value;
}


static MGValue* _mgVisitBinOpLogical(MGValue *module, MGNode *node)
{
	MG_ASSERT(_mgListLength(node->children) == 2);

	MGValue *lhs = _mgVisitNode(module, _mgListGet(node->children, 0));
	MGValue *rhs = NULL;
	MG_ASSERT(lhs);

	MGValue *value = NULL;

	switch (node->type)
	{
	case MG_NODE_BIN_OP_AND:
		if (!mgValueTruthValue(lhs))
			value = mgCreateValueInteger(MG_FALSE);
		else
		{
			rhs = _mgVisitNode(module, _mgListGet(node->children, 1));
			MG_ASSERT(rhs);
			value = mgCreateValueInteger(mgValueTruthValue(rhs));
		}
		break;
	case MG_NODE_BIN_OP_OR:
		if (mgValueTruthValue(lhs))
			value = mgCreateValueInteger(MG_TRUE);
		else
		{
			rhs = _mgVisitNode(module, _mgListGet(node->children, 1));
			MG_ASSERT(rhs);
			value = mgCreateValueInteger(mgValueTruthValue(rhs));
		}
		break;
	case MG_NODE_BIN_OP_COALESCE:
		if (lhs->type != MG_TYPE_NULL)
			value = mgReferenceValue(lhs);
		else
		{
			rhs = _mgVisitNode(module, _mgListGet(node->children, 1));
			MG_ASSERT(rhs);
			value = mgReferenceValue(rhs);
		}
		break;
	default:
		break;
	}

	if (value == NULL)
	{
		if (rhs)
			MG_FAIL("Error: Unsupported binary operator %s for left-hand type %s and right-hand type %s",
			        _MG_NODE_NAMES[node->type], mgGetTypeName(lhs->type), mgGetTypeName(rhs->type));
		else
			MG_FAIL("Error: Unsupported binary operator %s for left-hand type %s",
			        _MG_NODE_NAMES[node->type], mgGetTypeName(lhs->type));
	}

	mgDestroyValue(lhs);

	if (rhs)
		mgDestroyValue(rhs);

	return value;
}


static inline MGValue* _mgVisitConditional(MGValue *module, MGNode *node)
{
	MG_ASSERT((_mgListLength(node->children) == 2) || (_mgListLength(node->children) == 3));

	MGValue *condition = _mgVisitNode(module, _mgListGet(node->children, 0));
	MG_ASSERT(condition);

	MGbool _condition = mgValueTruthValue(condition);

	if (_mgListLength(node->children) == 3)
	{
		mgDestroyValue(condition);

		return _mgVisitNode(module, _mgListGet(node->children, _condition ? 1 : 2));
	}
	else
	{
		if (!_condition)
			mgDestroyValue(condition);

		return _condition ? condition : _mgVisitNode(module, _mgListGet(node->children, 1));
	}
}


static inline void _mgResolveImportAs(MGValue *module, MGToken *name, MGToken *alias)
{
	MG_ASSERT(name);
	MG_ASSERT(name->type == MG_TOKEN_NAME);
	MG_ASSERT(alias);
	MG_ASSERT(alias->type == MG_TOKEN_NAME);

	MGValue *importedModule = mgImportModule(module->data.module.instance, name->value.s);
	MG_ASSERT(importedModule);
	MG_ASSERT(importedModule->type == MG_TYPE_MODULE);

	_mgSetValue(module, alias->value.s, importedModule);
}


static MGValue* _mgVisitImport(MGValue *module, MGNode *node)
{
	MG_ASSERT(module);
	MG_ASSERT(module->type == MG_TYPE_MODULE);
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

		MGValue *importedModule = mgImportModule(module->data.module.instance, nameNode->token->value.s);
		MG_ASSERT(importedModule);
		MG_ASSERT(importedModule->type == MG_TYPE_MODULE);

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

					MG_ASSERT(aliasNode->type == MG_NODE_NAME);
					MG_ASSERT(aliasNode->token);
				}

				MG_ASSERT(nameNode->type == MG_NODE_NAME);
				MG_ASSERT(nameNode->token);

				MGValue *value = mgModuleGet(importedModule, nameNode->token->value.s);

				if (!value)
					_MG_FAIL(module, NULL, "Error: Undefined name \"%s\"", nameNode->token->value.s);

				if (aliasNode)
					_mgSetValue(module, aliasNode->token->value.s, mgReferenceValue(value));
				else
					_mgSetValue(module, nameNode->token->value.s, mgReferenceValue(value));
			}
		}
		else
		{
			MGMapIterator iterator;
			mgCreateMapIterator(&iterator, importedModule->data.module.globals);

			const MGValue *k, *v;
			while (mgMapNext(&iterator, &k, &v))
				_mgSetValue(module, k->data.str.s, mgReferenceValue(v));

			mgDestroyMapIterator(&iterator);
		}
	}

	return MG_NULL_VALUE;
}


static MGValue* _mgVisitAssert(MGValue *module, MGNode *node)
{
#if MG_DEBUG
	MG_ASSERT(node);
	MG_ASSERT(node->type == MG_NODE_ASSERT);
	MG_ASSERT((_mgListLength(node->children) == 1) || (_mgListLength(node->children) == 2));

	MGValue *expression = _mgVisitNode(module, _mgListGet(node->children, 0));
	MG_ASSERT(expression->type == MG_TYPE_INTEGER);

	if (!expression->data.i)
	{
		if (_mgListLength(node->children) == 2)
		{
			MGValue *message = _mgVisitNode(module, _mgListGet(node->children, 1));
			MG_ASSERT(message->type == MG_TYPE_STRING);

			MG_FAIL("Error: %s", message->data.str.s);

			mgDestroyValue(message);
		}
		else
			MG_FAIL("Error: Assertion");
	}

	mgDestroyValue(expression);

#endif

	return MG_NULL_VALUE;
}


MGValue* _mgVisitNode(MGValue *module, MGNode *node)
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
		return mgVisitBinOp(module, node, MG_BIN_OP_ADD);
	case MG_NODE_BIN_OP_SUB:
		return mgVisitBinOp(module, node, MG_BIN_OP_SUB);
	case MG_NODE_BIN_OP_MUL:
		return mgVisitBinOp(module, node, MG_BIN_OP_MUL);
	case MG_NODE_BIN_OP_DIV:
		return mgVisitBinOp(module, node, MG_BIN_OP_DIV);
	case MG_NODE_BIN_OP_INT_DIV:
		return mgVisitBinOp(module, node, MG_BIN_OP_INT_DIV);
	case MG_NODE_BIN_OP_MOD:
		return mgVisitBinOp(module, node, MG_BIN_OP_MOD);
	case MG_NODE_BIN_OP_EQ:
		return mgVisitBinOp(module, node, MG_BIN_OP_EQ);
	case MG_NODE_BIN_OP_NOT_EQ:
		return mgVisitBinOp(module, node, MG_BIN_OP_NOT_EQ);
	case MG_NODE_BIN_OP_LESS:
		return mgVisitBinOp(module, node, MG_BIN_OP_LESS);
	case MG_NODE_BIN_OP_LESS_EQ:
		return mgVisitBinOp(module, node, MG_BIN_OP_LESS_EQ);
	case MG_NODE_BIN_OP_GREATER:
		return mgVisitBinOp(module, node, MG_BIN_OP_GREATER);
	case MG_NODE_BIN_OP_GREATER_EQ:
		return mgVisitBinOp(module, node, MG_BIN_OP_GREATER_EQ);
	case MG_NODE_BIN_OP_AND:
	case MG_NODE_BIN_OP_OR:
	case MG_NODE_BIN_OP_COALESCE:
		return _mgVisitBinOpLogical(module, node);
	case MG_NODE_BIN_OP_CONDITIONAL:
	case MG_NODE_TERNARY_OP_CONDITIONAL:
		return _mgVisitConditional(module, node);
	case MG_NODE_UNARY_OP_POS:
		return _mgVisitUnaryOp(module, node, MG_UNARY_OP_POSITIVE);
	case MG_NODE_UNARY_OP_NEG:
		return _mgVisitUnaryOp(module, node, MG_UNARY_OP_NEGATIVE);
	case MG_NODE_UNARY_OP_NOT:
		return _mgVisitUnaryOp(module, node, MG_UNARY_OP_INVERSE);
	case MG_NODE_ASSIGN:
		return _mgVisitAssignment(module, node);
	case MG_NODE_ASSIGN_ADD:
	case MG_NODE_ASSIGN_SUB:
	case MG_NODE_ASSIGN_MUL:
	case MG_NODE_ASSIGN_DIV:
	case MG_NODE_ASSIGN_INT_DIV:
	case MG_NODE_ASSIGN_MOD:
		return _mgVisitAugmentedAssignment(module, node);
	case MG_NODE_CALL:
		return _mgVisitCall(module, node);
	case MG_NODE_FOR:
		return _mgVisitFor(module, node);
	case MG_NODE_WHILE:
		return _mgVisitWhile(module, node);
	case MG_NODE_BREAK:
		return _mgVisitBreak(module, node);
	case MG_NODE_CONTINUE:
		return _mgVisitContinue(module, node);
	case MG_NODE_IF:
		return _mgVisitIf(module, node);
	case MG_NODE_FUNCTION:
	case MG_NODE_PROCEDURE:
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
	case MG_NODE_NOP:
		return MG_NULL_VALUE;
	default:
		MG_FAIL("Error: Unknown node \"%s\"", _MG_NODE_NAMES[node->type]);
	}

	return NULL;
}


inline MGValue* mgInterpret(MGValue *module)
{
	MG_ASSERT(module);
	MG_ASSERT(module->type == MG_TYPE_MODULE);
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
