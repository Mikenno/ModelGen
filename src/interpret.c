
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

	if (module->filename)
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


static MGValue* _mgDeepCopyValue(MGValue *value)
{
	MG_ASSERT(value);

	MGValue *copy = (MGValue*) malloc(sizeof(MGValue));
	memcpy(copy, value, sizeof(MGValue));

	switch (copy->type)
	{
	case MG_VALUE_STRING:
		copy->data.s = mgStringDuplicate(value->data.s);
		break;
	case MG_VALUE_TUPLE:
	case MG_VALUE_LIST:
		if (value->data.a.length)
		{
			_mgListCreate(MGValue*, copy->data.a, _mgListCapacity(value->data.a));
			for (size_t i = 0; i < _mgListLength(value->data.a); ++i)
				_mgListAdd(MGValue*, copy->data.a, _mgDeepCopyValue(_mgListGet(value->data.a, i)));
		}
		else if (value->data.a.capacity)
			_mgListInitialize(copy->data.a);
		break;
	default:
		break;
	}

	return copy;
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
		MGValue *value = _mgVisitNode(module, _mgListGet(node->children, i));

		if (frame->state == MG_STACK_FRAME_STATE_UNWINDING)
		{
			mgDestroyValue(value);

			if (frame->value)
				return _mgDeepCopyValue(frame->value);

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

	MGValue *func = NULL;
	char *name = NULL;

	if (nameNode->type == MG_NODE_IDENTIFIER)
	{
		MG_ASSERT(nameNode->token);

		const size_t nameLength = nameNode->token->end.string - nameNode->token->begin.string;
		MG_ASSERT(nameLength > 0);
		name = mgStringDuplicateFixed(nameNode->token->begin.string, nameLength);

		func = mgModuleGet(module, name);

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

	mgPushStackFrame(module->instance, &frame);

	if (func->type == MG_VALUE_CFUNCTION)
		frame.value = func->data.cfunc(module, _mgListLength(args), _mgListItems(args));
	else
	{
		MGNode *procNode = func->data.func;

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
					mgModuleSet(module, procParameterName, _mgDeepCopyValue(_mgListGet(args, i)));
				else
				{
					if (procParameterNode->type != MG_NODE_BIN_OP)
						MG_FAIL("Error: Expected argument \"%s\"", procParameterName);

					mgModuleSet(module, procParameterName, _mgVisitNode(module, _mgListGet(procParameterNode->children, 1)));
				}

				free(procParameterName);
			}

			if (_mgListLength(procNode->children) == 3)
				_mgVisitNode(module, _mgListGet(procNode->children, 2));
		}
	}

	MGValue *value = NULL;

	if (frame.value)
		value = _mgDeepCopyValue(frame.value);
	else
		value = mgCreateValueVoid();

	mgPopStackFrame(module->instance, &frame);

	mgDestroyStackFrame(&frame);

	for (size_t i = 0; i < _mgListLength(args); ++i)
		mgDestroyValue(_mgListGet(args, i));
	_mgListDestroy(args);

	free(name);

	MG_ASSERT(value);

	return value;
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

		mgModuleSet(module, name, _mgDeepCopyValue(value));

		for (size_t j = 2; j < _mgListLength(node->children); ++j)
		{
			MGValue *result = _mgVisitNode(module, _mgListGet(node->children, j));
			MG_ASSERT(result);
			mgDestroyValue(result);
		}
	}

	mgModuleSet(module, name, NULL);

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
	proc->data.func = node;

	if (nameNode->type == MG_NODE_INVALID)
		return proc;

	MG_ASSERT(nameNode->type == MG_NODE_IDENTIFIER);
	MG_ASSERT(nameNode->token);

	const size_t nameLength = nameNode->token->end.string - nameNode->token->begin.string;
	MG_ASSERT(nameLength > 0);
	char *name = mgStringDuplicateFixed(nameNode->token->begin.string, nameLength);
	MG_ASSERT(name);

	mgModuleSet(module, name, proc);

	free(name);

	return _mgDeepCopyValue(proc);
}


static MGValue* _mgVisitSubscript(MGModule *module, MGNode *node)
{
	MG_ASSERT(_mgListLength(node->children) == 2);

	MGValue *value = _mgVisitNode(module, _mgListGet(node->children, 0));
	MGValue *index = _mgVisitNode(module, _mgListGet(node->children, 1));

	MG_ASSERT(value);
	MG_ASSERT(index);

	if ((value->type != MG_VALUE_TUPLE) && (value->type != MG_VALUE_LIST))
		MG_FAIL("Error: \"%s\" is not subscriptable", _MG_VALUE_TYPE_NAMES[value->type]);

	if (index->type != MG_VALUE_INTEGER)
		MG_FAIL("Error: \"%s\" index must be \"%s\" and not \"%s\"",
		        _MG_VALUE_TYPE_NAMES[value->type], _MG_VALUE_TYPE_NAMES[MG_VALUE_INTEGER], _MG_VALUE_TYPE_NAMES[index->type]);

	if ((mgIntegerGet(index) < 0) || (mgIntegerGet(index) >= mgListLength(value)))
		MG_FAIL("Error: \"%s\" index out of range", _MG_VALUE_TYPE_NAMES[value->type]);

	MGValue *result = _mgDeepCopyValue(mgTupleGet(value, mgIntegerGet(index)));

	mgDestroyValue(value);
	mgDestroyValue(index);

	return result;
}


static MGValue* _mgVisitIdentifier(MGModule *module, MGNode *node)
{
	MG_ASSERT(node->token);

	const size_t nameLength = node->token->end.string - node->token->begin.string;
	MG_ASSERT(nameLength > 0);
	char *name = mgStringDuplicateFixed(node->token->begin.string, nameLength);
	MG_ASSERT(name);

	MGValue *value = mgModuleGet(module, name);

	if (!value)
		MG_FAIL("Error: Undefined name \"%s\"", name);

	free(name);

	return _mgDeepCopyValue(value);
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


static inline MGValue* _mgVisitAssignment(MGModule *module, MGNode *node)
{
	MG_ASSERT(_mgListLength(node->children) == 2);

	MGNode *nameNode = _mgListGet(node->children, 0);
	MG_ASSERT(nameNode->type == MG_NODE_IDENTIFIER);
	MG_ASSERT(nameNode->token);

	const size_t nameLength = nameNode->token->end.string - nameNode->token->begin.string;
	MG_ASSERT(nameLength > 0);
	char *name = mgStringDuplicateFixed(nameNode->token->begin.string, nameLength);
	MG_ASSERT(name);

	MGValue *value = _mgVisitNode(module, _mgListGet(node->children, 1));
	mgModuleSet(module, name, value);

	free(name);

	return _mgDeepCopyValue(value);
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
				value = mgCreateValueTuple(lhs->data.a.length + rhs->data.a.length);
				value->type = (lhs->type == MG_VALUE_TUPLE) ? MG_VALUE_TUPLE : MG_VALUE_LIST;

				for (size_t i = 0; i < lhs->data.a.length; ++i)
					mgTupleAdd(value, _mgDeepCopyValue(lhs->data.a.items[i]));

				for (size_t i = 0; i < rhs->data.a.length; ++i)
					mgTupleAdd(value, _mgDeepCopyValue(rhs->data.a.items[i]));
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
				const size_t len = ((rhs->data.a.length > 0) && (lhs->data.i > 0)) ? (rhs->data.a.length * lhs->data.i) : 0;
				value = mgCreateValueTuple(len);

				for (size_t i = 0; i < len; ++i)
					mgTupleAdd(value, _mgDeepCopyValue(rhs->data.a.items[i % rhs->data.a.length]));

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
				const size_t len = ((lhs->data.a.length > 0) && (rhs->data.i > 0)) ? (lhs->data.a.length * rhs->data.i) : 0;
				value = mgCreateValueTuple(len);

				for (size_t i = 0; i < len; ++i)
					mgTupleAdd(value, _mgDeepCopyValue(lhs->data.a.items[i % lhs->data.a.length]));
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
	case MG_NODE_RETURN:
		return _mgVisitReturn(module, node);
	case MG_NODE_SUBSCRIPT:
		return _mgVisitSubscript(module, node);
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
