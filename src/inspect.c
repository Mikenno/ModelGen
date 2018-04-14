
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "inspect.h"
#include "module.h"
#include "utilities.h"
#include "modelgen.h"


#define _MG_FILENAME_PADDING 4
#define _MG_NODE_PADDING 40

#define _MG_NODE_INDENT            "|- "
#define _MG_NODE_INDENT_LAST       "`- "
#define _MG_NODE_CHILD_INDENT      "|  "
#define _MG_NODE_CHILD_INDENT_LAST "   "
#define _MG_NODE_INDENT_LENGTH     3


void mgInspectToken(const MGToken *token)
{
	mgInspectTokenEx(token, NULL, MG_FALSE);
}


void mgInspectTokenEx(const MGToken *token, const char *filename, MGbool justify)
{
	const unsigned int len = token->end.string - token->begin.string;

	char *string2 = NULL;

	if (token->type == MG_TOKEN_STRING)
	{
		if (token->value.s)
		{
			string2 = (char*) malloc((mgInlineRepresentationLength(token->value.s, NULL) + 1) * sizeof(char));
			mgInlineRepresentation(string2, token->value.s, NULL);
		}
	}
	else if (len)
	{
		string2 = (char*) malloc((mgInlineRepresentationLength(token->begin.string, token->end.string) + 1) * sizeof(char));
		mgInlineRepresentation(string2, token->begin.string, token->end.string);
	}

	if (filename)
		printf("%s:", filename);

	if (justify)
	{
		int padding = _MG_INT_COUNT_DIGITS(token->begin.line) + _MG_INT_COUNT_DIGITS(token->begin.character);
		padding = (padding > _MG_FILENAME_PADDING) ? 0 : (_MG_FILENAME_PADDING + 1 - padding);

		printf("%u:%u:%*s %-*s \"%s\"\n",
		       token->begin.line, token->begin.character,
		       padding, "",
		       _MG_LONGEST_TOKEN_NAME_LENGTH, _MG_TOKEN_NAMES[token->type],
		       string2 ? string2 : "");
	}
	else
	{
		printf("%u:%u: %s \"%s\"\n",
		       token->begin.line, token->begin.character,
		       _MG_TOKEN_NAMES[token->type],
		       string2 ? string2 : "");
	}

	free(string2);
}


static void _mgInspectNode(const MGNode *node, char *prefix, char *prefixEnd, MGbool isLast)
{
	int width = 0;

	if (isLast)
		width += printf("%s" _MG_NODE_INDENT_LAST, prefix);
	else
		width += printf("%s" _MG_NODE_INDENT, prefix);

	width += printf("%s", _MG_NODE_NAMES[node->type]);

	if (node->token)
	{
		switch (node->token->type)
		{
		case MG_TOKEN_IDENTIFIER:
		case MG_TOKEN_INTEGER:
		case MG_TOKEN_FLOAT:
		case MG_TOKEN_STRING:
		case MG_TOKEN_ADD:
		case MG_TOKEN_SUB:
		case MG_TOKEN_MUL:
		case MG_TOKEN_DIV:
		case MG_TOKEN_MOD:
		case MG_TOKEN_ASSIGN:
		case MG_TOKEN_ADD_ASSIGN:
		case MG_TOKEN_SUB_ASSIGN:
		case MG_TOKEN_MUL_ASSIGN:
		case MG_TOKEN_MOD_ASSIGN:
		case MG_TOKEN_EQUAL:
		case MG_TOKEN_NOT_EQUAL:
		case MG_TOKEN_LESS:
		case MG_TOKEN_LESS_EQUAL:
		case MG_TOKEN_GREATER:
		case MG_TOKEN_GREATER_EQUAL:
		case MG_TOKEN_NOT:
		case MG_TOKEN_AND:
		case MG_TOKEN_OR:
			width += printf(" %.*s", (int) (node->token->end.string - node->token->begin.string), node->token->begin.string);
			break;
		default:
			break;
		}
	}

	if (isLast)
		strcpy(prefixEnd, _MG_NODE_CHILD_INDENT_LAST);
	else
		strcpy(prefixEnd, _MG_NODE_CHILD_INDENT);

	if (width < _MG_NODE_PADDING)
		printf("%*s", _MG_NODE_PADDING - width, "");
	else
		putchar(' ');

#if MG_ANSI_COLORS
	fputs("\e[90m", stdout);
#endif

	if (node->token && (node->tokenBegin == node->tokenEnd))
		mgInspectTokenEx(node->token, NULL, MG_FALSE);
	else if (node->tokenBegin && node->tokenEnd)
		printf("%u:%u->%u:%u\n",
		       node->tokenBegin->begin.line, node->tokenBegin->begin.character,
		       node->tokenEnd->end.line, node->tokenEnd->end.character);
	else
		putchar('\n');

#if MG_DEBUG_SHOW_RANGE
	if (node->tokenBegin != node->tokenEnd)
	{
		printf("%s" _MG_NODE_INDENT, prefix);
		mgDebugInspectToken(node->tokenBegin, NULL, MG_FALSE);
		printf("%s%s", prefix, node->childCount ? _MG_NODE_CHILD_INDENT : _MG_NODE_INDENT_LAST);
		mgDebugInspectToken(node->tokenEnd, NULL, MG_FALSE);
	}
#endif

#if MG_ANSI_COLORS
	fputs("\e[0m", stdout);
#endif

	for (size_t i = 0; i < _mgListLength(node->children); ++i)
	{
		if (isLast)
			strcpy(prefixEnd, _MG_NODE_CHILD_INDENT_LAST);
		else
			strcpy(prefixEnd, _MG_NODE_CHILD_INDENT);

		_mgInspectNode(_mgListGet(node->children, i), prefix, prefixEnd + _MG_NODE_INDENT_LENGTH, (MGbool) (i == (_mgListLength(node->children) - 1)));
	}
}


void mgInspectNode(const MGNode *node)
{
	// Warning: If the height exceeds 341 nodes then we're in a world of trouble
	// TODO: Check the node's height and allocate ((height * _MG_NODE_INDENT_LENGTH + 1) * sizeof(char))
	char prefix[1024];
	prefix[0] = '\0';

	_mgInspectNode(node, prefix, prefix, MG_TRUE);
}


typedef struct _MGInspectValueMetadata {
	_MGList(const MGValue*) references;
} _MGInspectValueMetadata;


static inline void _mgCreateInspectValueMetadata(_MGInspectValueMetadata *metadata)
{
	MG_ASSERT(metadata);

	memset(metadata, 0, sizeof(_MGInspectValueMetadata));
}


static inline void _mgDestroyInspectValueMetadata(_MGInspectValueMetadata *metadata)
{
	MG_ASSERT(metadata);

	_mgListDestroy(metadata->references);
}


static inline MGbool _mgMetadataContains(const _MGInspectValueMetadata *metadata, const MGValue *value)
{
	for (size_t i = 0; i < _mgListLength(metadata->references); ++i)
		if (_mgListGet(metadata->references, i) == value)
			return MG_TRUE;

	return MG_FALSE;
}


static inline void _mgMetadataAdd(_MGInspectValueMetadata *metadata, const MGValue *value)
{
	if (!_mgMetadataContains(metadata, value))
		_mgListAdd(const MGValue*, metadata->references, value);
}


static void _mgInspectName(const MGValueMapPair *name, unsigned int depth, _MGInspectValueMetadata *metadata);
static void _mgInspectValue(const MGValue *value, unsigned int depth, _MGInspectValueMetadata *metadata);


static inline void _mgInspectValueType(const MGValue *value)
{
	fputs(_MG_VALUE_TYPE_NAMES[value->type], stdout);

	if ((value->type == MG_VALUE_TUPLE) || (value->type == MG_VALUE_LIST))
		printf("[%zu]", _mgListLength(value->data.a));
	else if (value->type == MG_VALUE_MAP)
		printf("[%zu]", _mgListLength(value->data.m));
	else if ((value->type == MG_VALUE_MODULE) && value->data.module.filename)
		printf(" \"%s\"", value->data.module.filename);
}


static inline void _mgInspectName(const MGValueMapPair *name, unsigned int depth, _MGInspectValueMetadata *metadata)
{
	for (unsigned int i = 0; i < depth; ++i)
		fputs("    ", stdout);

	fputs(name->key, stdout);
	fputs(": ", stdout);
	_mgInspectValueType(name->value);
	fputs(" = ", stdout);

	_mgInspectValue(name->value, depth, metadata);
	putchar('\n');
}


static void _mgInspectValue(const MGValue *value, unsigned int depth, _MGInspectValueMetadata *metadata)
{
	MG_ASSERT(value);
	MG_ASSERT(metadata);

	if (_mgMetadataContains(metadata, value))
	{
		printf("<Circular Reference %p>", value);
		return;
	}

	switch (value->type)
	{
	case MG_VALUE_INTEGER:
		printf("%d", value->data.i);
		break;
	case MG_VALUE_FLOAT:
		printf("%f", value->data.f);
		break;
	case MG_VALUE_STRING:
	{
		char *str = (char*) malloc((mgInlineRepresentationLength(value->data.str.s, NULL) + 1) * sizeof(char));
		mgInlineRepresentation(str, value->data.str.s, NULL);
		printf("\"%s\"", str);
		free(str);
		break;
	}
	case MG_VALUE_TUPLE:
	case MG_VALUE_LIST:
		putchar((value->type == MG_VALUE_TUPLE) ? '(' : '[');
		if (_mgListLength(value->data.m))
		{
			_mgMetadataAdd(metadata, value);
			for (size_t i = 0; i < _mgListLength(value->data.a); ++i)
			{
				if (i > 0)
					fputs(", ", stdout);
				_mgInspectValue(_mgListGet(value->data.a, i), depth, metadata);
			}
		}
		if ((_mgListLength(value->data.a) == 1) && (value->type == MG_VALUE_TUPLE))
			putchar(',');
		putchar((value->type == MG_VALUE_TUPLE) ? ')' : ']');
		break;
	case MG_VALUE_MAP:
		putchar('{');
		if (_mgMapSize(value->data.m))
		{
			_mgMetadataAdd(metadata, value);
			putchar('\n');
			for (size_t i = 0; i < _mgMapSize(value->data.m); ++i)
				_mgInspectName(&_mgListGet(value->data.m, i), depth + 1, metadata);
			for (unsigned int i = 0; i < depth; ++i)
				fputs("    ", stdout);
		}
		putchar('}');
		break;
	case MG_VALUE_CFUNCTION:
		printf("%p", value->data.cfunc);
		break;
	case MG_VALUE_PROCEDURE:
	case MG_VALUE_FUNCTION:
		printf("%p", value->data.func.node);
		if (value->data.func.locals && mgListLength(value->data.func.locals))
		{
			putchar(' ');
			_mgInspectValueType(value->data.func.locals);
			putchar(' ');
			_mgInspectValue(value->data.func.locals, depth, metadata);
		}
		break;
	case MG_VALUE_MODULE:
		_mgInspectValue(value->data.module.globals, depth, metadata);
		break;
	default:
		break;
	}
}


void mgInspectValue(const MGValue *value)
{
	mgInspectValueEx(value, MG_TRUE);
}


void mgInspectValueEx(const MGValue *value, MGbool end)
{
	_MGInspectValueMetadata metadata;

	_mgCreateInspectValueMetadata(&metadata);
	_mgInspectValue(value, 0, &metadata);
	_mgDestroyInspectValueMetadata(&metadata);

	if (end)
		putchar('\n');
}


inline void mgInspectInstance(const MGInstance *instance)
{
	printf("Instance [%zu:%zu] ", mgListLength(instance->modules), mgListCapacity(instance->modules));

	mgInspectValue(instance->modules);
}


void mgInspectStackFrame(const MGStackFrame *frame)
{
	MG_ASSERT(frame);
	MG_ASSERT(frame->module);

	printf("StackFrame\n");

	if (frame->callerName)
		printf("Callee: %s\n", frame->callerName);

	if (frame->caller && frame->caller->tokenBegin)
	{
		MGToken *token = frame->caller->tokenBegin ? frame->caller->tokenBegin : frame->caller->token;

		if (token)
		{
			MG_ASSERT(frame->module);
			MG_ASSERT(frame->module->type == MG_VALUE_MODULE);
			MG_ASSERT(frame->module->data.module.filename);

			printf("Caller: %s:%u:%u\n",
			       frame->module->data.module.filename,
			       frame->caller->tokenBegin->begin.line,
			       frame->caller->tokenBegin->begin.character);
		}
	}

	printf("State: %s\n", _MG_STACK_FRAME_STATE_NAMES[frame->state]);

	if (frame->value)
	{
		printf("Value = ");
		mgInspectValue(frame->value);
	}
}


void mgInspectStringLines(const char *str)
{
	unsigned int lineCount = 1;

	for (const char *c = str; *c; ++c)
		if (*c == '\n')
			++lineCount;

	const char *currentLine = str;
	unsigned int line = 1;

	while (currentLine)
	{
		char *nextLine = strchr(currentLine, '\n');

		if (nextLine)
			*nextLine = '\0';

		printf("%*u: %s\n", _MG_INT_COUNT_DIGITS(lineCount), line, currentLine);

		if (nextLine)
			*nextLine = '\n';

		currentLine = nextLine ? (nextLine + 1) : NULL;
		++line;
	}
}


void _mgDebugTokenizePrint(const char *filename, char *str, size_t len)
{
	if (str)
	{
		printf("Tokenizing: %s\n", filename);

		MGToken token;
		mgTokenReset(str, &token);

		filename = mgBasename(filename);

		do
		{
			mgTokenizeNext(&token);
			mgInspectTokenEx(&token, filename, MG_TRUE);
		}
		while (token.type != MG_TOKEN_EOF);
	}
	else
	{
		fprintf(stderr, "Failed Reading: %s\n", filename);
	}
}


MGbool mgDebugRead(const char *filename)
{
	size_t len;
	char *str = mgReadFile(filename, &len);

	printf("Reading: %s\n", filename);
	mgInspectStringLines(str);

	free(str);

	return str ? MG_TRUE : MG_FALSE;
}


MGbool mgDebugReadHandle(FILE *file, const char *filename)
{
	size_t len;
	char *str = mgReadFileHandle(file, &len);

	printf("Reading: %s\n", filename);
	mgInspectStringLines(str);

	free(str);

	return str ? MG_TRUE : MG_FALSE;
}


MGbool mgDebugTokenize(const char *filename)
{
	size_t len;
	char *str = mgReadFile(filename, &len);

	_mgDebugTokenizePrint(filename, str, len);

	free(str);

	return str ? MG_TRUE : MG_FALSE;
}


MGbool mgDebugTokenizeHandle(FILE *file, const char *filename)
{
	size_t len;
	char *str = mgReadFileHandle(file, &len);

	_mgDebugTokenizePrint(filename, str, len);

	free(str);

	return str ? MG_TRUE : MG_FALSE;
}
