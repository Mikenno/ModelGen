
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "modelgen.h"
#include "inspect.h"


static inline void _mgAssert(const char *expression, const char *file, int line)
{
	fprintf(stderr, "%s:%d: Assertion Failed: %s\n", file, line, expression);
	exit(1);
}

#define MG_ASSERT(expression) ((expression) ? ((void)0) : _mgAssert(#expression, __FILE__, __LINE__))


static inline void _mgFail(MGModule *module, MGNode *node, const char *format, ...)
{
	fflush(stdout);

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

#define MG_FAIL(...) _mgFail(module, node, __VA_ARGS__)


static inline char* _mgStrdup(const char *str)
{
	char *s = (char*) malloc((strlen(str) + 1) * sizeof(char));
	strcpy(s, str);
	return s;
}


static inline char* _mgStrndup(const char *str, size_t count)
{
	char *s = (char*) malloc((count + 1) * sizeof(char));
	strncpy(s, str, count);
	s[count] = '\0';
	return s;
}


static MGValue* _mgDeepCopyValue(MGValue *value)
{
	MG_ASSERT(value);

	MGValue *copy = (MGValue*) malloc(sizeof(MGValue));
	memcpy(copy, value, sizeof(MGValue));

	switch (copy->type)
	{
	case MG_VALUE_STRING:
		copy->data.s = _mgStrdup(value->data.s);
		break;
	case MG_VALUE_TUPLE:
		if (value->data.a.length)
		{
			copy->data.a.items = (MGValue**) malloc(value->data.a.length * sizeof(MGValue*));
			for (size_t i = 0; i < value->data.a.length; ++i)
				copy->data.a.items[i] = _mgDeepCopyValue(value->data.a.items[i]);
		}
		break;
	default:
		break;
	}

	return copy;
}


static MGValue* _mgVisitNode(MGModule *module, MGNode *node);


static MGValue* _mgVisitChildren(MGModule *module, MGNode *node)
{
	MGValue *value = mgCreateValue(MG_VALUE_TUPLE);
	value->data.a.items = (MGValue**) malloc(node->childCount * sizeof(MGValue*));
	value->data.a.length = node->childCount;
	value->data.a.capacity = node->childCount;

	for (size_t i = 0; i < node->childCount; ++i)
		value->data.a.items[i] = _mgVisitNode(module, node->children[i]);

	return value;
}


static inline MGValue* _mgVisitModule(MGModule *module, MGNode *node)
{
	return _mgVisitChildren(module, node);
}


static MGValue* _mgVisitCall(MGModule *module, MGNode *node)
{
	MG_ASSERT(node->childCount > 0);

	MGNode *nameNode = node->children[0];
	MG_ASSERT(nameNode->type == MG_NODE_IDENTIFIER);
	MG_ASSERT(nameNode->token);

	const size_t nameLength = nameNode->token->end.string - nameNode->token->begin.string;
	MG_ASSERT(nameLength > 0);
	char *name = _mgStrndup(nameNode->token->begin.string, nameLength);
	MG_ASSERT(name);

	MGValue *func = mgGetValue(module, name);

	if (!func)
		MG_FAIL("Error: Undefined name \"%s\"", name);

	if (func->type != MG_VALUE_CFUNCTION)
		MG_FAIL("Error: \"%s\" is not callable", _MG_VALUE_TYPE_NAMES[func->type]);

	size_t argc = node->childCount - 1;
	MGValue **argv = (MGValue**) malloc(argc * sizeof(MGValue*));

	for (size_t i = 0; i < argc; ++i)
	{
		argv[i] = _mgVisitNode(module, node->children[i + 1]);
		MG_ASSERT(argv[i]);
	}

	MGValue *value = func->data.cfunc(argc, argv);

	for (size_t i = 0; i < argc; ++i)
		mgDestroyValue(argv[i]);
	free(argv);

	free(name);

	return value;
}


static MGValue* _mgVisitFor(MGModule *module, MGNode *node)
{
	MG_ASSERT(node->childCount >= 2);

	MGNode *nameNode = node->children[0];
	MG_ASSERT(nameNode->type == MG_NODE_IDENTIFIER);
	MG_ASSERT(nameNode->token);

	const size_t nameLength = nameNode->token->end.string - nameNode->token->begin.string;
	MG_ASSERT(nameLength > 0);
	char *name = _mgStrndup(nameNode->token->begin.string, nameLength);
	MG_ASSERT(name);

	MGValue *test = _mgVisitNode(module, node->children[1]);
	MG_ASSERT(test);
	MG_ASSERT(test->type == MG_VALUE_TUPLE);

	int iterations = 0;

	for (size_t i = 0; i < test->data.a.length; ++i, ++iterations)
	{
		MGValue *value = test->data.a.items[i];
		MG_ASSERT(value);

		mgSetValue(module, name, _mgDeepCopyValue(value));

		for (size_t j = 2; j < node->childCount; ++j)
		{
			MGValue *result = _mgVisitNode(module, node->children[j]);
			MG_ASSERT(result);
			mgDestroyValue(result);
		}
	}

	mgSetValue(module, name, NULL);

	mgDestroyValue(test);

	free(name);

	MGValue *value = mgCreateValue(MG_VALUE_INTEGER);
	value->data.i = iterations;

	return value;
}


static MGValue* _mgVisitIdentifier(MGModule *module, MGNode *node)
{
	MG_ASSERT(node->token);

	const size_t nameLength = node->token->end.string - node->token->begin.string;
	MG_ASSERT(nameLength > 0);
	char *name = _mgStrndup(node->token->begin.string, nameLength);
	MG_ASSERT(name);

	MGValue *value = mgGetValue(module, name);

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
	char *_value = _mgStrndup(node->token->begin.string, _valueLength);
	MG_ASSERT(_value);

	MGValue *value = mgCreateValue(MG_VALUE_INTEGER);
	value->data.i = strtol(_value, NULL, 10);

	free(_value);

	return value;
}


static MGValue* _mgVisitFloat(MGModule *module, MGNode *node)
{
	MG_ASSERT(node->token);

	const size_t _valueLength = node->token->end.string - node->token->begin.string;
	MG_ASSERT(_valueLength > 0);
	char *_value = _mgStrndup(node->token->begin.string, _valueLength);
	MG_ASSERT(_value);

	MGValue *value = mgCreateValue(MG_VALUE_FLOAT);
	value->data.f = strtof(_value, NULL);

	free(_value);

	return value;
}


static MGValue* _mgVisitString(MGModule *module, MGNode *node)
{
	MG_ASSERT(node->token);

	MGValue *value = mgCreateValue(MG_VALUE_STRING);
	value->data.s = _mgStrdup(node->token->value.s ? node->token->value.s : "");

	return value;
}


static MGValue* _mgVisitTuple(MGModule *module, MGNode *node)
{
	MG_ASSERT(node->token);

	MGValue *value = mgCreateValue(MG_VALUE_TUPLE);
	value->data.a.items = (MGValue**) malloc(node->childCount * sizeof(MGValue*));
	value->data.a.length = node->childCount;
	value->data.a.capacity = node->childCount;

	for (size_t i = 0; i < node->childCount; ++i)
		value->data.a.items[i] = _mgVisitNode(module, node->children[i]);

	return value;
}


static MGValue* _mgVisitRange(MGModule *module, MGNode *node)
{
	MG_ASSERT((node->childCount == 2) || (node->childCount == 3));

	int range[3] = { 0, 0, 1 };

	for (size_t i = 0; i < node->childCount; ++i)
	{
		MGNode *child = node->children[i];

		MG_ASSERT(child);
		MG_ASSERT(child->type == MG_NODE_INTEGER);

		MG_ASSERT(child->token);

		const size_t _valueLength = child->token->end.string - child->token->begin.string;
		MG_ASSERT(_valueLength > 0);
		char *_value = _mgStrndup(child->token->begin.string, _valueLength);
		MG_ASSERT(_value);

		range[i] = strtol(_value, NULL, 10);

		free(_value);
	}

	MG_ASSERT(range[2] > 0);

	int length = (range[1] - range[0]) / range[2] + (((range[1] - range[0]) % range[2]) != 0);

	MGValue *value = mgCreateValue(MG_VALUE_TUPLE);

	if (length > 0)
	{
		value->data.a.items = (MGValue**) malloc(length * sizeof(MGValue*));
		value->data.a.length = (size_t) length;
		value->data.a.capacity = (size_t) length;

		for (int i = 0; i < length; ++i)
		{
			MGValue *item = mgCreateValue(MG_VALUE_INTEGER);
			item->data.i = range[0] + range[2] * i;

			value->data.a.items[i] = item;
		}
	}
	else
	{
		value->data.a.items = NULL;
		value->data.a.length = 0;
		value->data.a.capacity = 0;
	}

	return value;
}


static inline MGValue* _mgVisitAssignment(MGModule *module, MGNode *node)
{
	MG_ASSERT(node->childCount == 2);

	MGNode *nameNode = node->children[0];
	MG_ASSERT(nameNode->type == MG_NODE_IDENTIFIER);
	MG_ASSERT(nameNode->token);

	const size_t nameLength = nameNode->token->end.string - nameNode->token->begin.string;
	MG_ASSERT(nameLength > 0);
	char *name = _mgStrndup(nameNode->token->begin.string, nameLength);
	MG_ASSERT(name);

	MGValue *value = _mgVisitNode(module, node->children[1]);
	mgSetValue(module, name, value);

	free(name);

	return _mgDeepCopyValue(value);
}


static MGValue* _mgVisitBinOp(MGModule *module, MGNode *node)
{
	MG_ASSERT(node->childCount == 2);
	MG_ASSERT(node->token);

	const size_t opLength = node->token->end.string - node->token->begin.string;
	MG_ASSERT(opLength > 0);
	char *op = _mgStrndup(node->token->begin.string, opLength);
	MG_ASSERT(op);

	MGValue *value = NULL;

	if (strcmp(op, "=") == 0)
		value = _mgVisitAssignment(module, node);
	else
		MG_FAIL("Error: Unknown BinOp \"%s\"", op);

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
		return _mgVisitTuple(module, node);
	case MG_NODE_RANGE:
		return _mgVisitRange(module, node);
	case MG_NODE_BIN_OP:
		return _mgVisitBinOp(module, node);
	case MG_NODE_CALL:
		return _mgVisitCall(module, node);
	case MG_NODE_FOR:
		return _mgVisitFor(module, node);
	default:
		MG_FAIL("Error: Unknown node \"%s\"", _MG_NODE_NAMES[node->type]);
	}

	return NULL;
}


MGValue* mgRunFile(MGModule *module, const char *filename)
{
	MG_ASSERT(module);
	MG_ASSERT(filename);

	module->filename = filename;

	MGValue *result = NULL;

	MGParser parser;
	mgCreateParser(&parser);

	if (mgParseFile(&parser, filename))
		result = _mgVisitNode(module, parser.root);

	mgDestroyParser(&parser);

	return result;
}


MGValue* mgRunFileHandle(MGModule *module, FILE *file)
{
	MG_ASSERT(module);
	MG_ASSERT(file);

	MGValue *result = NULL;

	MGParser parser;
	mgCreateParser(&parser);

	if (mgParseFileHandle(&parser, file))
		result = _mgVisitNode(module, parser.root);

	mgDestroyParser(&parser);

	return result;
}


MGValue* mgRunString(MGModule *module, const char *string)
{
	MG_ASSERT(module);
	MG_ASSERT(string);

	MGValue *result = NULL;

	MGParser parser;
	mgCreateParser(&parser);

	if (mgParseString(&parser, string))
		result = _mgVisitNode(module, parser.root);

	mgDestroyParser(&parser);

	return result;
}
