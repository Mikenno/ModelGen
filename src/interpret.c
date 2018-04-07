
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "modelgen.h"
#include "module.h"
#include "inspect.h"
#include "utilities.h"
#include "baselib.h"


static inline void _mgFail(const char *file, int line, MGModule *module, MGNode *node, const char *format, ...)
{
	fflush(stdout);

	fprintf(stderr, "%s:%d: ", file, line);

	if (module && module->filename)
		fprintf(stderr, "%s:", module->filename);
	if (node && node->tokenBegin)
		fprintf(stderr, "%u:%u: ", node->tokenBegin->begin.line, node->tokenBegin->begin.character);

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

	MGValue *copy = (MGValue*) malloc(sizeof(MGValue));
	MG_ASSERT(copy);

	*copy = *value;
	copy->refCount = 1;

	switch (copy->type)
	{
	case MG_VALUE_STRING:
		copy->data.s = mgStringDuplicate(value->data.s);
		break;
	case MG_VALUE_TUPLE:
	case MG_VALUE_LIST:
		if (mgListLength(value))
		{
			_mgListCreate(MGValue*, copy->data.a, mgListCapacity(value));
			for (size_t i = 0; i < mgListLength(value); ++i)
				_mgListAdd(MGValue*, copy->data.a, mgDeepCopyValue(mgListGet(value, i)));
		}
		else if (mgListCapacity(value))
			_mgListInitialize(copy->data.a);
		break;
	case MG_VALUE_MAP:
		if (_mgListLength(value->data.m))
		{
			_mgCreateMap(&copy->data.m, _mgListLength(value->data.m));
			for (size_t i = 0; i < _mgListLength(value->data.m); ++i)
			{
				const MGValueMapPair *pair = &_mgListGet(value->data.m, i);
				mgMapSet(copy, pair->key, mgDeepCopyValue(pair->value));
			}
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


static inline void _mgSetValue(MGModule *module, const char *name, MGValue *value)
{
	MG_ASSERT(module);
	MG_ASSERT(module->instance);
	MG_ASSERT(module->instance->callStackTop);
	MG_ASSERT(module->instance->callStackTop->locals);

	mgMapSet(module->instance->callStackTop->locals, name, value);
}


static inline MGValue* _mgGetValue(MGModule *module, const char *name)
{
	MG_ASSERT(module);
	MG_ASSERT(module->instance);
	MG_ASSERT(module->instance->callStackTop);
	MG_ASSERT(module->instance->callStackTop->locals);

	MGValue *value = mgMapGet(module->instance->callStackTop->locals, name);

	if (!value)
		value = mgModuleGet(module, name);

	if (!value)
		_MG_FAIL(module, NULL, "Error: Undefined name \"%s\"", name);

	return value;
}


static MGValue* _mgVisitNode(MGModule *module, MGNode *node);


static MGValue* _mgVisitChildren(MGModule *module, MGNode *node)
{
	MG_ASSERT(module);
	MG_ASSERT(module->instance);
	MG_ASSERT(module->instance->callStackTop);

	MGStackFrame *frame = module->instance->callStackTop;

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

	return mgCreateValueVoid();
}


static inline MGValue* _mgVisitModule(MGModule *module, MGNode *node)
{
	return _mgVisitChildren(module, node);
}


static MGValue* _mgVisitCall(MGModule *module, MGNode *node)
{
	MG_ASSERT(module);
	MG_ASSERT(module->instance);
	MG_ASSERT(_mgListLength(node->children) > 0);

	MGNode *nameNode = _mgListGet(node->children, 0);

	const MGValue *func = NULL;
	char *name = NULL;

	if (nameNode->type == MG_NODE_IDENTIFIER)
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
	mgCreateStackFrame(&frame);

	frame.state = MG_STACK_FRAME_STATE_ACTIVE;
	frame.module = module;
	frame.caller = node;
	frame.callerName = name;

	mgPushStackFrame(frame.module->instance, &frame);

	if (func->type == MG_VALUE_CFUNCTION)
		frame.value = func->data.cfunc(frame.module->instance, _mgListLength(args), _mgListItems(args));
	else
	{
		MG_ASSERT(func->data.func.module);
		MG_ASSERT(func->data.func.node);

		MGNode *procNode = func->data.func.node;

		if (procNode)
		{
			MG_ASSERT((procNode->type == MG_NODE_PROCEDURE) || procNode->type == MG_NODE_FUNCTION);
			MG_ASSERT((_mgListLength(procNode->children) == 2) || (_mgListLength(procNode->children) == 3));

			MGNode *procParametersNode = _mgListGet(procNode->children, 1);
			MG_ASSERT(procParametersNode->type == MG_NODE_TUPLE);

			if (_mgListLength(procParametersNode->children) < _mgListLength(args))
			{
				MGNode *procNameNode = _mgListGet(procNode->children, 0);
				MG_ASSERT(procNameNode->type == MG_NODE_IDENTIFIER);
				MG_ASSERT(procNameNode->token);

				const size_t procNameLength = procNameNode->token->end.string - procNameNode->token->begin.string;
				MG_ASSERT(procNameLength > 0);
				char *procName = mgStringDuplicateFixed(procNameNode->token->begin.string, procNameLength);
				MG_ASSERT(procName);

				MG_FAIL("Error: %s expected at most %zu arguments, received %zu", procName, _mgListLength(procParametersNode->children), _mgListLength(args));

				free(procName);
			}

			for (size_t i = 0; i < _mgListLength(procParametersNode->children); ++i)
			{
				MGNode *procParameterNode = _mgListGet(procParametersNode->children, i);
				MG_ASSERT((procParameterNode->type == MG_NODE_IDENTIFIER) || (procParameterNode->type == MG_NODE_BIN_OP));

				char *procParameterName = NULL;

				if (procParameterNode->type == MG_NODE_IDENTIFIER)
				{
					const size_t procParameterNameLength = procParameterNode->token->end.string - procParameterNode->token->begin.string;
					MG_ASSERT(procParameterNameLength > 0);
					procParameterName = mgStringDuplicateFixed(procParameterNode->token->begin.string, procParameterNameLength);
				}
				else if (procParameterNode->type == MG_NODE_BIN_OP)
				{
					MG_ASSERT(_mgListLength(procParameterNode->children) == 2);
					MG_ASSERT(procParameterNode->token);

					const size_t opLength = procParameterNode->token->end.string - procParameterNode->token->begin.string;
					MG_ASSERT(opLength > 0);
					char *op = mgStringDuplicateFixed(procParameterNode->token->begin.string, opLength);
					MG_ASSERT(op);

					MG_ASSERT(strcmp(op, "=") == 0);

					free(op);

					MGNode *procParameterNameNode = _mgListGet(procParameterNode->children, 0);
					MG_ASSERT(procParameterNameNode->type == MG_NODE_IDENTIFIER);
					MG_ASSERT(procParameterNameNode->token);

					const size_t procParameterNameLength = procParameterNameNode->token->end.string - procParameterNameNode->token->begin.string;
					MG_ASSERT(procParameterNameLength > 0);
					procParameterName = mgStringDuplicateFixed(procParameterNameNode->token->begin.string, procParameterNameLength);
				}

				MG_ASSERT(procParameterName);

				if (i < _mgListLength(args))
					_mgSetValue(module, procParameterName, mgReferenceValue(_mgListGet(args, i)));
				else
				{
					if (procParameterNode->type != MG_NODE_BIN_OP)
						MG_FAIL("Error: Expected argument \"%s\"", procParameterName);

					_mgSetValue(module, procParameterName, _mgVisitNode(module, _mgListGet(procParameterNode->children, 1)));
				}

				free(procParameterName);
			}

			if (_mgListLength(procNode->children) == 3)
				_mgVisitNode(func->data.func.module, _mgListGet(procNode->children, 2));
		}
	}

	MGValue *value = NULL;

	if (frame.value)
		value = mgReferenceValue(frame.value);
	else
		value = mgCreateValueVoid();

	mgPopStackFrame(frame.module->instance, &frame);
	mgDestroyStackFrame(&frame);

	for (size_t i = 0; i < _mgListLength(args); ++i)
		mgDestroyValue(_mgListGet(args, i));
	_mgListDestroy(args);

	free(name);

	MG_ASSERT(value);

	return value;
}


static MGValue* _mgVisitEmit(MGModule *module, MGNode *node)
{
	MG_ASSERT(module);
	MG_ASSERT(module->instance);
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

	const size_t vertexCount = _mgListLength(module->instance->vertices);

	_mgListAddUninitialized(MGVertex, module->instance->vertices);
	MGVertex *vertices = _mgListItems(module->instance->vertices);

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

	++_mgListLength(module->instance->vertices);

	mgDestroyValue(tuple);

	return mgCreateValueVoid();
}


static MGValue* _mgVisitReturn(MGModule *module, MGNode *node)
{
	MG_ASSERT(module);
	MG_ASSERT(module->instance);
	MG_ASSERT(module->instance->callStackTop);

	MGStackFrame *frame = module->instance->callStackTop;

	if (_mgListLength(node->children) > 0)
		frame->value = _mgVisitNode(module, _mgListGet(node->children, 0));
	else
		frame->value = mgCreateValueTuple(0);

	frame->state = MG_STACK_FRAME_STATE_UNWINDING;

	return mgCreateValueInteger((_mgListLength(node->children) > 0) ? 1 : 0);
}


static MGValue* _mgVisitFor(MGModule *module, MGNode *node)
{
	MG_ASSERT(_mgListLength(node->children) >= 2);

	MGNode *nameNode = _mgListGet(node->children, 0);
	MG_ASSERT(nameNode->type == MG_NODE_IDENTIFIER);
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

		_mgSetValue(module, name, mgReferenceValue(value));

		for (size_t j = 2; j < _mgListLength(node->children); ++j)
		{
			MGValue *result = _mgVisitNode(module, _mgListGet(node->children, j));
			MG_ASSERT(result);
			mgDestroyValue(result);
		}
	}

	_mgSetValue(module, name, NULL);

	mgDestroyValue(test);

	free(name);

	return mgCreateValueInteger(iterations);
}


static MGValue* _mgVisitIf(MGModule *module, MGNode *node)
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
		_condition = condition->data.s ? (int) strlen(condition->data.s) : 0;
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


static MGValue* _mgVisitProcedure(MGModule *module, MGNode *node)
{
	MG_ASSERT((_mgListLength(node->children) == 2) || (_mgListLength(node->children) == 3));

	MGNode *nameNode = _mgListGet(node->children, 0);
	MG_ASSERT((nameNode->type == MG_NODE_INVALID) || (nameNode->type == MG_NODE_IDENTIFIER));

	MGValue *proc = mgCreateValue((node->type == MG_NODE_PROCEDURE) ? MG_VALUE_PROCEDURE : MG_VALUE_FUNCTION);
	proc->data.func.module = module;
	proc->data.func.node = node;

	if (nameNode->type == MG_NODE_INVALID)
		return proc;

	MG_ASSERT(nameNode->type == MG_NODE_IDENTIFIER);
	MG_ASSERT(nameNode->token);

	const size_t nameLength = nameNode->token->end.string - nameNode->token->begin.string;
	MG_ASSERT(nameLength > 0);
	char *name = mgStringDuplicateFixed(nameNode->token->begin.string, nameLength);
	MG_ASSERT(name);

	_mgSetValue(module, name, proc);

	free(name);

	return mgReferenceValue(proc);
}


static inline size_t _mgResolveArrayIndex(MGModule *module, MGNode *node, MGValue *collection, MGValue *index)
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
			MG_FAIL("Error: \"%s\" index out of range (0 <= %zu < %zu)",
			        _MG_VALUE_TYPE_NAMES[collection->type],
			        mgIntegerGet(index), mgListLength(collection));
		}
		else
		{
			MG_FAIL("Error: \"%s\" index out of range (%zu <= %zu < 0)",
			        _MG_VALUE_TYPE_NAMES[collection->type],
			        -mgListLength(collection), mgIntegerGet(index));
		}
	}

	return i;
}


static inline MGValue* _mgResolveArrayGet(MGModule *module, MGNode *node, MGValue *collection, MGValue *index)
{
	return mgReferenceValue(_mgListGet(collection->data.a, _mgResolveArrayIndex(module, node, collection, index)));
}


static inline void _mgResolveArraySet(MGModule *module, MGNode *node, MGValue *collection, MGValue *index, MGValue *value)
{
	const size_t i = _mgResolveArrayIndex(module, node, collection, index);

	mgDestroyValue(_mgListGet(collection->data.a, i));
	_mgListSet(collection->data.a, i, mgReferenceValue(value));
}


static inline MGValue* _mgResolveMapGet(MGModule *module, MGNode *node, MGValue *collection, MGValue *key)
{
	MG_ASSERT(collection);
	MG_ASSERT(collection->type == MG_VALUE_MAP);
	MG_ASSERT(key);
	MG_ASSERT(key->type == MG_VALUE_STRING);
	MG_ASSERT(key->data.s);

	MGValue *value = mgMapGet(collection, key->data.s);

	if (value == NULL)
		MG_FAIL("Error: Undefined key \"%s\"", key->data.s);

	return mgReferenceValue(value);
}


static inline void _mgResolveMapSet(MGValue *collection, MGValue *key, MGValue *value)
{
	MG_ASSERT(collection);
	MG_ASSERT(collection->type == MG_VALUE_MAP);
	MG_ASSERT(key);
	MG_ASSERT(key->type == MG_VALUE_STRING);
	MG_ASSERT(key->data.s);
	MG_ASSERT(value);

	mgMapSet(collection, key->data.s, value);
}


static inline MGValue* _mgResolveSubscriptGet(MGModule *module, MGNode *node, MGValue *collection, MGValue *index)
{
	if ((collection->type == MG_VALUE_TUPLE) || (collection->type == MG_VALUE_LIST))
		return _mgResolveArrayGet(module, node, collection, index);
	else if (collection->type == MG_VALUE_MAP)
		return _mgResolveMapGet(module, node, collection, index);
	else
		MG_FAIL("Error: \"%s\" is not subscriptable", _MG_VALUE_TYPE_NAMES[collection->type]);

	return NULL;
}


static inline void _mgResolveSubscriptSet(MGModule *module, MGNode *node, MGValue *collection, MGValue *index, MGValue *value)
{
	if ((collection->type == MG_VALUE_TUPLE) || (collection->type == MG_VALUE_LIST))
		_mgResolveArraySet(module, node, collection, index, value);
	else if (collection->type == MG_VALUE_MAP)
		_mgResolveMapSet(collection, index, value);
	else
		MG_FAIL("Error: \"%s\" is not subscriptable", _MG_VALUE_TYPE_NAMES[collection->type]);
}


static MGValue* _mgVisitSubscript(MGModule *module, MGNode *node)
{
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


static MGValue* _mgVisitAttribute(MGModule *module, MGNode *node)
{
	MG_ASSERT(_mgListLength(node->children) == 2);

	MGValue *object = _mgVisitNode(module, _mgListGet(node->children, 0));
	MG_ASSERT(object);
	MG_ASSERT(object->type == MG_VALUE_MAP);

	const MGNode *attribute = _mgListGet(node->children, 1);
	MG_ASSERT(attribute);
	MG_ASSERT(attribute->type == MG_NODE_IDENTIFIER);
	MG_ASSERT(attribute->token);

	const size_t nameLength = attribute->token->end.string - attribute->token->begin.string;
	MG_ASSERT(nameLength > 0);
	char *name = mgStringDuplicateFixed(attribute->token->begin.string, nameLength);
	MG_ASSERT(name);

	MGValue *value = mgMapGet(object, name);

	if (value == NULL)
		MG_FAIL("Error: \"%s\" has no attribute \"%s\"",
		        _MG_VALUE_TYPE_NAMES[object->type], name);

	free(name);

	return mgReferenceValue(value);
}


static MGValue* _mgVisitIdentifier(MGModule *module, MGNode *node)
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


static MGValue* _mgVisitInteger(MGModule *module, MGNode *node)
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


static MGValue* _mgVisitFloat(MGModule *module, MGNode *node)
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


static inline MGValue* _mgVisitString(MGModule *module, MGNode *node)
{
	MG_ASSERT(node->token);

	return mgCreateValueString(node->token->value.s);
}


static MGValue* _mgVisitTuple(MGModule *module, MGNode *node)
{
	MG_ASSERT(node->token);
	MG_ASSERT((node->type == MG_NODE_TUPLE) || (node->type == MG_NODE_LIST));

	MGValue *value = mgCreateValueTuple(_mgListLength(node->children));
	value->type = (node->type == MG_NODE_TUPLE) ? MG_VALUE_TUPLE : MG_VALUE_LIST;

	for (size_t i = 0; i < _mgListLength(node->children); ++i)
		mgTupleAdd(value, _mgVisitNode(module, _mgListGet(node->children, i)));

	return value;
}


static MGValue* _mgVisitRange(MGModule *module, MGNode *node)
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


static MGValue* _mgVisitMap(MGModule *module, MGNode *node)
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
		MG_ASSERT(key->data.s);
		MG_ASSERT(value);

		mgMapSet(map, key->data.s, value);

		mgDestroyValue(key);
	}

	return map;
}


static inline MGValue* _mgVisitAssignment(MGModule *module, MGNode *node)
{
	MG_ASSERT(_mgListLength(node->children) == 2);

	MGValue *value = _mgVisitNode(module, _mgListGet(node->children, 1));
	MG_ASSERT(value);

	MGNode *lhs = _mgListGet(node->children, 0);
	MG_ASSERT(lhs);
	MG_ASSERT((lhs->type == MG_NODE_IDENTIFIER) || (lhs->type == MG_NODE_SUBSCRIPT) || (lhs->type == MG_NODE_ATTRIBUTE) || (lhs->type == MG_NODE_TUPLE));

	if (lhs->type == MG_NODE_IDENTIFIER)
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
		MG_ASSERT(object->type == MG_VALUE_MAP);

		MGNode *attributeNode = _mgListGet(lhs->children, 1);
		MG_ASSERT(attributeNode->type == MG_NODE_IDENTIFIER);
		MG_ASSERT(attributeNode->token);

		const size_t nameLength = attributeNode->token->end.string - attributeNode->token->begin.string;
		MG_ASSERT(nameLength > 0);
		char *name = mgStringDuplicateFixed(attributeNode->token->begin.string, nameLength);
		MG_ASSERT(name);

		mgMapSet(object, name, mgReferenceValue(value));

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
			if (_mgListGet(lhs->children, i)->type != MG_NODE_IDENTIFIER)
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

			_mgSetValue(module, name, mgReferenceValue(mgListGet(value, i)));
		}

		free(name);
	}

	return value;
}


static MGValue* _mgVisitBinOp(MGModule *module, MGNode *node)
{
	MG_ASSERT(_mgListLength(node->children) == 2);
	MG_ASSERT(node->token);

	const size_t opLength = node->token->end.string - node->token->begin.string;
	MG_ASSERT(opLength > 0);
	char *op = mgStringDuplicateFixed(node->token->begin.string, opLength);
	MG_ASSERT(op);

	if (strcmp(op, "=") == 0)
	{
		free(op);
		return _mgVisitAssignment(module, node);
	}

	MGValue *lhs = _mgVisitNode(module, _mgListGet(node->children, 0));
	MG_ASSERT(lhs);
	MGValue *rhs = NULL;

	if ((strcmp(op, "and") != 0) && (strcmp(op, "or") != 0))
	{
		rhs = _mgVisitNode(module, _mgListGet(node->children, 1));
		MG_ASSERT(rhs);
	}

	MGValue *value = NULL;

	if (strcmp(op, "+") == 0)
	{
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
				MG_ASSERT(rhs->data.s);
				size_t len = (size_t) snprintf(NULL, 0, "%d%s", lhs->data.i, rhs->data.s);
				MG_ASSERT(len >= 0);
				char *s = (char*) malloc((len + 1) * sizeof(char));
				snprintf(s, len + 1, "%d%s", lhs->data.i, rhs->data.s);
				s[len] = '\0';
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
				MG_ASSERT(rhs->data.s);
				size_t len = (size_t) snprintf(NULL, 0, "%f%s", lhs->data.f, rhs->data.s);
				MG_ASSERT(len >= 0);
				char *s = (char*) malloc((len + 1) * sizeof(char));
				snprintf(s, len + 1, "%f%s", lhs->data.f, rhs->data.s);
				s[len] = '\0';
				value = mgCreateValueString(s);
				free(s);
				break;
			default:
				break;
			}
			break;
		case MG_VALUE_STRING:
			MG_ASSERT(lhs->data.s);
			char *s = NULL;
			size_t len = 0;
			switch (rhs->type)
			{
			case MG_VALUE_INTEGER:
				len = (size_t) snprintf(NULL, 0, "%s%d", lhs->data.s, rhs->data.i);
				MG_ASSERT(len >= 0);
				s = (char*) malloc((len + 1) * sizeof(char));
				snprintf(s, len + 1, "%s%d", lhs->data.s, rhs->data.i);
				s[len] = '\0';
				break;
			case MG_VALUE_FLOAT:
				len = (size_t) snprintf(NULL, 0, "%s%f", lhs->data.s, rhs->data.f);
				MG_ASSERT(len >= 0);
				s = (char*) malloc((len + 1) * sizeof(char));
				snprintf(s, len + 1, "%s%f", lhs->data.s, rhs->data.f);
				s[len] = '\0';
				break;
			case MG_VALUE_STRING:
				MG_ASSERT(rhs->data.s);
				s = (char*) malloc((strlen(lhs->data.s) + strlen(rhs->data.s) + 1) * sizeof(char));
				MG_ASSERT(s);
				strcpy(s, lhs->data.s);
				strcat(s, rhs->data.s);
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
					mgTupleAdd(value, mgReferenceValue(mgListGet(lhs, i)));

				for (size_t i = 0; i < mgListLength(rhs); ++i)
					mgTupleAdd(value, mgReferenceValue(mgListGet(rhs, i)));
			}
			break;
		default:
			break;
		}
	}
	else if (strcmp(op, "-") == 0)
	{
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
	}
	else if (strcmp(op, "*") == 0)
	{
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
				const size_t len = strlen(rhs->data.s);

				if ((len > 0) && (lhs->data.i > 0))
				{
					char *str = mgStringRepeatDuplicate(rhs->data.s, len, (size_t) lhs->data.i);
					value = mgCreateValueString(str);
					free(str);
				}
				else
					value = mgCreateValueString(NULL);

				break;
			}
			case MG_VALUE_TUPLE:
			{
				const size_t len = ((mgListLength(rhs) > 0) && (lhs->data.i > 0)) ? (mgListLength(rhs) * lhs->data.i) : 0;
				value = mgCreateValueTuple(len);

				for (size_t i = 0; i < len; ++i)
					mgTupleAdd(value, mgReferenceValue(mgListGet(rhs, i % mgListLength(rhs))));

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
				const size_t len = strlen(lhs->data.s);

				if ((len > 0) && (rhs->data.i > 0))
				{
					char *str = mgStringRepeatDuplicate(lhs->data.s, len, (size_t) rhs->data.i);
					value = mgCreateValueString(str);
					free(str);
				}
				else
					value = mgCreateValueString(NULL);
			}
			break;
		case MG_VALUE_TUPLE:
			if (rhs->type == MG_VALUE_INTEGER)
			{
				const size_t len = ((mgListLength(lhs) > 0) && (rhs->data.i > 0)) ? (mgListLength(lhs) * rhs->data.i) : 0;
				value = mgCreateValueTuple(len);

				for (size_t i = 0; i < len; ++i)
					mgTupleAdd(value, mgReferenceValue(mgListGet(lhs, i % mgListLength(lhs))));
			}
			break;
		default:
			break;
		}
	}
	else if (strcmp(op, "/") == 0)
	{
		switch (lhs->type)
		{
		case MG_VALUE_INTEGER:
			switch (rhs->type)
			{
			case MG_VALUE_INTEGER:
				value = mgCreateValueInteger(lhs->data.i / rhs->data.i);
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
	}
	else if (strcmp(op, "%") == 0)
	{
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
	}
	else if (strcmp(op, ">") == 0)
	{
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
	}
	else if (strcmp(op, ">=") == 0)
	{
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
	}
	else if (strcmp(op, "<") == 0)
	{
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
	}
	else if (strcmp(op, "<=") == 0)
	{
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
	}
	else if (strcmp(op, "==") == 0)
	{
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
			MG_ASSERT(lhs->data.s);
			switch (rhs->type)
			{
			case MG_VALUE_STRING:
				MG_ASSERT(rhs->data.s);
				value = mgCreateValueInteger(!strcmp(lhs->data.s, rhs->data.s));
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
	}
	else if (strcmp(op, "!=") == 0)
	{
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
				MG_ASSERT(lhs->data.s);
				MG_ASSERT(rhs->data.s);
				value = mgCreateValueInteger(strcmp(lhs->data.s, rhs->data.s));
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
	}
	else if (strcmp(op, "and") == 0)
	{
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
	}
	else if (strcmp(op, "or") == 0)
	{
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
	}

	if (value == NULL)
	{
		if (rhs)
			MG_FAIL("Error: Unsupported binary operator \"%s\" for left-hand type \"%s\" and right-hand type \"%s\"",
			        op, _MG_VALUE_TYPE_NAMES[lhs->type], _MG_VALUE_TYPE_NAMES[rhs->type]);
		else
			MG_FAIL("Error: Unsupported binary operator \"%s\" for left-hand type \"%s\"",
			        op, _MG_VALUE_TYPE_NAMES[lhs->type]);
	}

	MG_ASSERT(value);

	mgDestroyValue(lhs);

	if (rhs)
		mgDestroyValue(rhs);

	free(op);

	return value;
}


static MGValue* _mgVisitUnaryOp(MGModule *module, MGNode *node)
{
	MG_ASSERT(_mgListLength(node->children) == 1);
	MG_ASSERT(node->token);

	const size_t opLength = node->token->end.string - node->token->begin.string;
	MG_ASSERT(opLength > 0);
	char *op = mgStringDuplicateFixed(node->token->begin.string, opLength);
	MG_ASSERT(op);

	MGValue *operand = _mgVisitNode(module, _mgListGet(node->children, 0));
	MG_ASSERT(operand);

	MGValue *value = NULL;

	if (strcmp(op, "-") == 0)
	{
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
	}
	else if (strcmp(op, "+") == 0)
	{
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
	}
	else if ((strcmp(op, "not") == 0) || strcmp(op, "!") == 0)
	{
		switch (operand->type)
		{
		case MG_VALUE_INTEGER:
			value = mgCreateValueInteger(!operand->data.i);
			break;
		case MG_VALUE_FLOAT:
			value = mgCreateValueFloat(!operand->data.f);
			break;
		default:
			break;
		}
	}

	if (value == NULL)
		MG_FAIL("Error: Unsupported unary operator \"%s\" for type \"%s\"", op, _MG_VALUE_TYPE_NAMES[operand->type]);

	mgDestroyValue(operand);

	free(op);

	return value;
}


static MGValue* _mgVisitNode(MGModule *module, MGNode *node)
{
	switch (node->type)
	{
	case MG_NODE_MODULE:
		return _mgVisitModule(module, node);
	case MG_NODE_BLOCK:
		return _mgVisitChildren(module, node);
	case MG_NODE_IDENTIFIER:
		return _mgVisitIdentifier(module, node);
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
	case MG_NODE_BIN_OP:
		return _mgVisitBinOp(module, node);
	case MG_NODE_UNARY_OP:
		return _mgVisitUnaryOp(module, node);
	case MG_NODE_CALL:
		return _mgVisitCall(module, node);
	case MG_NODE_FOR:
		return _mgVisitFor(module, node);
	case MG_NODE_IF:
		return _mgVisitIf(module, node);
	case MG_NODE_PROCEDURE:
	case MG_NODE_FUNCTION:
		return _mgVisitProcedure(module, node);
	case MG_NODE_EMIT:
		return _mgVisitEmit(module, node);
	case MG_NODE_RETURN:
		return _mgVisitReturn(module, node);
	case MG_NODE_SUBSCRIPT:
		return _mgVisitSubscript(module, node);
	case MG_NODE_ATTRIBUTE:
		return _mgVisitAttribute(module, node);
	default:
		MG_FAIL("Error: Unknown node \"%s\"", _MG_NODE_NAMES[node->type]);
	}

	return NULL;
}


inline MGValue* mgInterpret(MGModule *module)
{
	MG_ASSERT(module);
	MG_ASSERT(module->instance);
	MG_ASSERT(module->parser.root);

	return _mgVisitNode(module, module->parser.root);
}


MGValue* mgInterpretFile(MGModule *module, const char *filename)
{
	MG_ASSERT(module);
	MG_ASSERT(filename);

	if (module->filename)
	{
		free(module->filename);
		module->filename = NULL;
	}

	module->filename = mgStringDuplicate(filename);

	mgDestroyParser(&module->parser);
	mgCreateParser(&module->parser);

	MGValue *result = NULL;
	if (mgParseFile(&module->parser, filename))
		result = mgInterpret(module);

	return result;
}


MGValue* mgInterpretFileHandle(MGModule *module, FILE *file, const char *filename)
{
	MG_ASSERT(module);
	MG_ASSERT(file);

	if (module->filename)
	{
		free(module->filename);
		module->filename = NULL;
	}

	if (filename)
		module->filename = mgStringDuplicate(filename);

	mgDestroyParser(&module->parser);
	mgCreateParser(&module->parser);

	MGValue *result = NULL;
	if (mgParseFileHandle(&module->parser, file))
		result = mgInterpret(module);

	return result;
}


MGValue* mgInterpretString(MGModule *module, const char *string, const char *filename)
{
	MG_ASSERT(module);
	MG_ASSERT(string);

	if (module->filename)
	{
		free(module->filename);
		module->filename = NULL;
	}

	if (filename)
		module->filename = mgStringDuplicate(filename);

	mgDestroyParser(&module->parser);
	mgCreateParser(&module->parser);

	MGValue *result = NULL;
	if (mgParseString(&module->parser, string))
		result = mgInterpret(module);

	return result;
}
