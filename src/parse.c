
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "parse.h"
#include "inspect.h"
#include "error.h"


#define mgParserFatalError(format, ...) \
	mgFatalError("%s:%u:%u: " format, parser->tokenizer.filename, token->begin.line, token->begin.character, __VA_ARGS__)

#define mgParserFatalErrorEx(token, format, ...) \
	mgFatalError("%s:%u:%u: " format, parser->tokenizer.filename, token->begin.line, token->begin.character, __VA_ARGS__)


#define _MG_TOKEN_SCAN_LINE(token)  for (; (token->type == MG_TOKEN_WHITESPACE) || (token->type == MG_TOKEN_COMMENT); ++token)
#define _MG_TOKEN_SCAN_LINES(token) for (; (token->type == MG_TOKEN_NEWLINE) || (token->type == MG_TOKEN_WHITESPACE) || (token->type == MG_TOKEN_COMMENT); ++token)


#define _MG_OPERATOR_PRECEDENCE_LEVELS 8
#define _MG_OPERATOR_PRECEDENCE_LONGEST_LEVEL 7

#define _MG_OPERATOR_PRECEDENCE_LEVEL_AFTER_RANGE 1

static const int _MG_OPERATOR_PRECEDENCE_TYPE_COUNT[_MG_OPERATOR_PRECEDENCE_LEVELS] = { 7, 1, 1, 2, 4, 2, 4, 1 };

static const MGTokenType _MG_OPERATOR_PRECEDENCE_TYPES[_MG_OPERATOR_PRECEDENCE_LEVELS][_MG_OPERATOR_PRECEDENCE_LONGEST_LEVEL] = {
	/* Lowest Precedence */
	{ MG_TOKEN_ASSIGN, MG_TOKEN_ADD_ASSIGN, MG_TOKEN_SUB_ASSIGN, MG_TOKEN_MUL_ASSIGN, MG_TOKEN_DIV_ASSIGN, MG_TOKEN_INT_DIV_ASSIGN, MG_TOKEN_MOD_ASSIGN },
	/* Range */
	{ MG_TOKEN_OR },
	{ MG_TOKEN_AND },
	{ MG_TOKEN_EQUAL, MG_TOKEN_NOT_EQUAL },
	{ MG_TOKEN_LESS, MG_TOKEN_LESS_EQUAL, MG_TOKEN_GREATER, MG_TOKEN_GREATER_EQUAL },
	{ MG_TOKEN_ADD, MG_TOKEN_SUB },
	{ MG_TOKEN_MUL, MG_TOKEN_DIV, MG_TOKEN_INT_DIV, MG_TOKEN_MOD },
	{ MG_TOKEN_COALESCE }
	/* Highest Precedence */
};

static const MGNodeType _MG_BIN_OP_NODE_TYPES[_MG_OPERATOR_PRECEDENCE_LEVELS][_MG_OPERATOR_PRECEDENCE_LONGEST_LEVEL] = {
	{},
	{ MG_NODE_BIN_OP_OR },
	{ MG_NODE_BIN_OP_AND },
	{ MG_NODE_BIN_OP_EQ, MG_NODE_BIN_OP_NOT_EQ },
	{ MG_NODE_BIN_OP_LESS, MG_NODE_BIN_OP_LESS_EQ, MG_NODE_BIN_OP_GREATER, MG_NODE_BIN_OP_GREATER_EQ },
	{ MG_NODE_BIN_OP_ADD, MG_NODE_BIN_OP_SUB },
	{ MG_NODE_BIN_OP_MUL, MG_NODE_BIN_OP_DIV, MG_NODE_BIN_OP_INT_DIV, MG_NODE_BIN_OP_MOD },
	{ MG_NODE_BIN_OP_COALESCE }
};

static const MGNodeType _MG_ASSIGN_NODE_TYPES[] = {
	MG_NODE_ASSIGN,
	MG_NODE_ASSIGN_ADD,
	MG_NODE_ASSIGN_SUB,
	MG_NODE_ASSIGN_MUL,
	MG_NODE_ASSIGN_DIV,
	MG_NODE_ASSIGN_INT_DIV,
	MG_NODE_ASSIGN_MOD
};


void mgCreateParser(MGParser *parser)
{
	memset(parser, 0, sizeof(MGParser));

	mgCreateTokenizer(&parser->tokenizer);
}


void mgDestroyParser(MGParser *parser)
{
	mgDestroyTokenizer(&parser->tokenizer);

	if (parser->root)
		mgDestroyNode(parser->root);
}


MGNode* mgCreateNode(MGToken *token, MGNodeType type)
{
	MGNode *node = (MGNode*) calloc(1, sizeof(MGNode));

	node->type = type;
	node->refCount = 1;
	node->token = token;
	node->tokenBegin = token;
	node->tokenEnd = token;

	return node;
}


void mgDestroyNode(MGNode *node)
{
	MG_ASSERT(node);

	if (--node->refCount > 0)
		return;

	for (size_t i = 0; i < _mgListLength(node->children); ++i)
		if (_mgListGet(node->children, i))
			mgDestroyNode(_mgListGet(node->children, i));

	_mgListDestroy(node->children);

	free(node);
}


static void _mgAddChild(MGNode *parent, MGNode *child)
{
	MG_ASSERT(parent);
	MG_ASSERT(child);

	_mgListAdd(MGNode*, parent->children, child);

	parent->tokenEnd = child->tokenEnd;
	child->parent = parent;
}


static inline MGNode* _mgWrapNode(MGNode *node, MGNodeType type)
{
	MGNode *parent = mgCreateNode(node->tokenBegin, type);
	parent->token = NULL;
	parent->tokenBegin = node->tokenBegin;

	_mgAddChild(parent, node);

	return parent;
}


static inline MGNode* _mgDestroyNodeExtractFirst(MGNode *node)
{
	MGNode *child = _mgListGet(node->children, 0);
	_mgListSet(node->children, 0, NULL);

	child->tokenBegin = node->tokenBegin;
	child->tokenEnd = node->tokenEnd;

	mgDestroyNode(node);

	return child;
}


static inline MGNode* _mgDeepCopyNode(const MGNode *node)
{
	MG_ASSERT(node);

	MGNode *copy = (MGNode*) malloc(sizeof(MGNode));
	MG_ASSERT(copy);

	*copy = *node;
	copy->refCount = 1;

	if (_mgListLength(node->children))
	{
		_mgListCreate(MGNode*, copy->children, _mgListCapacity(node->children));
		for (size_t i = 0; i < _mgListCapacity(node->children); ++i)
			_mgListAdd(MGNode*, copy->children, _mgDeepCopyNode(_mgListGet(node->children, i)));
	}
	else if (_mgListCapacity(node->children))
		_mgListInitialize(copy->children);

	return copy;
}


MGNode* mgReferenceNode(const MGNode *node)
{
	MG_ASSERT(node);

	MGNode *referenced = (MGNode*) node;
	++referenced->refCount;

	return referenced;
}


static inline int _mgTokenTypeIsSubexpression(MGTokenType type)
{
	return (type == MG_TOKEN_NAME) ||
	       (type == MG_TOKEN_INTEGER) ||
	       (type == MG_TOKEN_FLOAT) ||
	       (type == MG_TOKEN_STRING) ||
	       (type == MG_TOKEN_LPAREN) ||
	       (type == MG_TOKEN_LSQUARE) ||
	       (type == MG_TOKEN_LBRACE) ||
	       (type == MG_TOKEN_FOR) ||
	       (type == MG_TOKEN_IF) ||
	       (type == MG_TOKEN_PROC) ||
	       (type == MG_TOKEN_EMIT) ||
	       (type == MG_TOKEN_FUNC) ||
	       (type == MG_TOKEN_RETURN) ||
	       (type == MG_TOKEN_DELETE) ||
	       (type == MG_TOKEN_SUB) ||
	       (type == MG_TOKEN_ADD) ||
	       (type == MG_TOKEN_NOT);
}


static MGNode* _mgParseExpression(MGParser *parser, MGToken *token, MGbool eatTuple);
static MGNode* _mgParseAssignmentOrExpression(MGParser *parser, MGToken *token, MGbool eatTuple);


static void _mgParseBlock(MGParser *parser, MGToken *token, MGNode *node, unsigned int indentation)
{
	MG_ASSERT(node);

	while (indentation == token->begin.character)
	{
		MGNode *expr = _mgParseAssignmentOrExpression(parser, token, MG_TRUE);
		MG_ASSERT(expr);
		_mgAddChild(node, expr);

		token = expr->tokenEnd + 1;
		_MG_TOKEN_SCAN_LINES(token);

		if (indentation < token->begin.character)
		{
			if ((token->type == MG_TOKEN_EOF) || (token->type == MG_TOKEN_RPAREN) || (token->type == MG_TOKEN_RSQUARE) || (token->type == MG_TOKEN_COMMA) || (token->type == MG_TOKEN_ELSE))
				break;

			MGNode *block = mgCreateNode(token, MG_NODE_BLOCK);
			block->token = NULL;

			_mgParseBlock(parser, token, block, token->begin.character);

			if (_mgListLength(block->children) == 1)
				_mgAddChild(node, _mgDestroyNodeExtractFirst(block));
			else
				_mgAddChild(node, block);

			token = node->tokenEnd + 1;
			_MG_TOKEN_SCAN_LINES(token);
		}

		if ((token->type == MG_TOKEN_EOF) || (token->type == MG_TOKEN_RPAREN) || (token->type == MG_TOKEN_RSQUARE) || (token->type == MG_TOKEN_COMMA) || (token->type == MG_TOKEN_ELSE))
			break;
	}
}


static MGbool _mgParseExpressionList(MGParser *parser, MGToken *token, MGNode *node, MGTokenType end)
{
	MG_ASSERT(node);

	MGbool isTuple = MG_TRUE;

	do
	{
		_MG_TOKEN_SCAN_LINES(token);

		if (token->type == end)
			break;

		MG_ASSERT(token->type != MG_TOKEN_EOF);
		isTuple = MG_FALSE;

		_mgAddChild(node, _mgParseAssignmentOrExpression(parser, token, MG_FALSE));

		token = node->tokenEnd + 1;
		_MG_TOKEN_SCAN_LINES(token);

		if (token->type == end)
			break;

		MG_ASSERT(token->type == MG_TOKEN_COMMA);
		isTuple = MG_TRUE;

		node->tokenEnd = ++token;
	}
	while (token->type != end);

	MG_ASSERT(token->type == end);

	node->tokenEnd = token;

	return isTuple;
}


static void _mgParseTuple(MGParser *parser, MGToken *token, MGNode *tuple)
{
	for (;;)
	{
		_MG_TOKEN_SCAN_LINE(token);

		if (!_mgTokenTypeIsSubexpression(token->type))
			break;

		_mgAddChild(tuple, _mgParseExpression(parser, token, MG_FALSE));
		token = tuple->tokenEnd + 1;

		_MG_TOKEN_SCAN_LINE(token);

		if (token->type != MG_TOKEN_COMMA)
			break;

		tuple->tokenEnd = token++;
	}
}


static MGNode* _mgParseImport(MGParser *parser, MGToken *token)
{
	MG_ASSERT((token->type == MG_TOKEN_IMPORT) || (token->type == MG_TOKEN_FROM));

	MGNode *import = mgCreateNode(token, MG_NODE_IMPORT);

	if (token->type == MG_TOKEN_IMPORT)
	{
		++token;
		_MG_TOKEN_SCAN_LINE(token);

		_mgParseTuple(parser, token, import);
		MG_ASSERT(_mgListLength(import->children) > 0);

#if MG_DEBUG
		for (size_t i = 0; i < _mgListLength(import->children); ++i)
			MG_ASSERT((_mgListGet(import->children, i)->type == MG_NODE_NAME) || _mgListGet(import->children, i)->type == MG_NODE_AS);
#endif
	}
	else
	{
		import->type = MG_NODE_IMPORT_FROM;

		++token;
		_MG_TOKEN_SCAN_LINE(token);

		MG_ASSERT(token->type == MG_TOKEN_NAME);
		_mgAddChild(import, mgCreateNode(token++, MG_NODE_NAME));

		_MG_TOKEN_SCAN_LINE(token);

		if (token->type == MG_TOKEN_IMPORT)
		{
			++token;
			_MG_TOKEN_SCAN_LINE(token);

			if (token->type == MG_TOKEN_MUL)
				import->tokenEnd = token;
			else
			{
				_mgParseTuple(parser, token, import);
				MG_ASSERT(_mgListLength(import->children) > 0);

#if MG_DEBUG
				for (size_t i = 0; i < _mgListLength(import->children); ++i)
					MG_ASSERT((_mgListGet(import->children, i)->type == MG_NODE_NAME) || _mgListGet(import->children, i)->type == MG_NODE_AS);
#endif
			}
		}
	}

	return import;
}


static inline MGToken* _mgParseTypedName(MGParser *parser, MGToken *token, MGNode *name)
{
	MG_ASSERT(name);
	MG_ASSERT(name->type == MG_NODE_NAME);

	const size_t childIndex = _mgListLength(name->children);

	_MG_TOKEN_SCAN_LINE(token);
	MG_ASSERT(token->type == MG_TOKEN_NAME);

	MGNode *type = mgCreateNode(token++, MG_NODE_NAME);
	_mgAddChild(name, type);

	if (token->type == MG_TOKEN_LESS)
	{
		++token;

		for (;;)
		{
			token = _mgParseTypedName(parser, token, type) + 1;

			_MG_TOKEN_SCAN_LINES(token);

			if (token->type == MG_TOKEN_GREATER)
				break;

			MG_ASSERT(token->type == MG_TOKEN_COMMA);

			++token;
			_MG_TOKEN_SCAN_LINES(token);
		}

		name->tokenEnd = token++;
	}

	if (token->type == MG_TOKEN_QUESTION)
	{
		_mgListSet(name->children, childIndex, _mgWrapNode(type, MG_NODE_OPTIONAL));

		name->tokenEnd = token;
	}

	return name->tokenEnd;
}


static MGNode* _mgParseSubexpression(MGParser *parser, MGToken *token)
{
	_MG_TOKEN_SCAN_LINES(token);

	MGNode *node = NULL;

	if (token->type == MG_TOKEN_NAME)
	{
		node = mgCreateNode(token++, MG_NODE_NAME);

		// _MG_TOKEN_SCAN_LINE(token);

		if (token->type == MG_TOKEN_COLON)
		{
			_mgParseTypedName(parser, ++token, node);
			return node;
		}
	}
	else if ((token->type == MG_TOKEN_INTEGER) ||
	         (token->type == MG_TOKEN_FLOAT))
	{
		node = mgCreateNode(token, (token->type == MG_TOKEN_INTEGER) ? MG_NODE_INTEGER : MG_NODE_FLOAT);
		++token;
	}
	else if (token->type == MG_TOKEN_STRING)
		node = mgCreateNode(token++, MG_NODE_STRING);
	else if ((token->type == MG_TOKEN_SUB) ||
	         (token->type == MG_TOKEN_ADD) ||
	         (token->type == MG_TOKEN_NOT))
	{
		node = mgCreateNode(token, MG_NODE_INVALID);

		switch (token->type)
		{
		case MG_TOKEN_SUB:
			node->type = MG_NODE_UNARY_OP_NEG;
			break;
		case MG_TOKEN_ADD:
			node->type = MG_NODE_UNARY_OP_POS;
			break;
		case MG_TOKEN_NOT:
			node->type = MG_NODE_UNARY_OP_NOT;
			break;
		default:
			break;
		}

		++token;

		_mgAddChild(node, _mgParseSubexpression(parser, token));
		MG_ASSERT(_mgListLength(node->children) == 1);

		token = node->tokenEnd + 1;
	}
	else if (token->type == MG_TOKEN_LPAREN)
	{
		node = mgCreateNode(token++, MG_NODE_TUPLE);

		if (!_mgParseExpressionList(parser, token, node, MG_TOKEN_RPAREN) && (_mgListLength(node->children) == 1))
			node = _mgDestroyNodeExtractFirst(node);

		token = node->tokenEnd + 1;
	}
	else if (token->type == MG_TOKEN_LSQUARE)
	{
		node = mgCreateNode(token++, MG_NODE_LIST);

		_mgParseExpressionList(parser, token, node, MG_TOKEN_RSQUARE);

		token = node->tokenEnd + 1;
	}
	else if (token->type == MG_TOKEN_LBRACE)
	{
		node = mgCreateNode(token++, MG_NODE_MAP);

		_MG_TOKEN_SCAN_LINES(token);

		while (token->type != MG_TOKEN_RBRACE)
		{
			MG_ASSERT((token->type == MG_TOKEN_NAME) || (token->type == MG_TOKEN_STRING));

			_mgAddChild(node, mgCreateNode(token, (token->type == MG_TOKEN_NAME) ? MG_NODE_NAME : MG_NODE_STRING));

			++token;
			_MG_TOKEN_SCAN_LINE(token);
			MG_ASSERT(token->type == MG_TOKEN_COLON);

			++token;
			_MG_TOKEN_SCAN_LINES(token);

			_mgAddChild(node, _mgParseExpression(parser, token, MG_FALSE));

			token = node->tokenEnd + 1;
			_MG_TOKEN_SCAN_LINES(token);

			if (token->type == MG_TOKEN_RBRACE)
				break;

			MG_ASSERT(token->type == MG_TOKEN_COMMA);
			++token;
			_MG_TOKEN_SCAN_LINES(token);
		}

		if (token->type == MG_TOKEN_RBRACE)
			node->tokenEnd = token++;
	}
	else if (token->type == MG_TOKEN_FOR)
	{
		node = mgCreateNode(token, MG_NODE_FOR);

		MGNode *target = _mgParseExpression(parser, token + 1, MG_TRUE);
		MG_ASSERT(target);
		_mgAddChild(node, target);

		token = target->tokenEnd + 1;
		_MG_TOKEN_SCAN_LINE(token);

		MG_ASSERT(token->type == MG_TOKEN_IN);
		++token;

		MGNode *iterable = _mgParseExpression(parser, token, MG_TRUE);
		MG_ASSERT(iterable);
		_mgAddChild(node, iterable);

		token = iterable->tokenEnd + 1;
		_MG_TOKEN_SCAN_LINES(token);

		if ((node->tokenBegin->begin.character < token->begin.character) &&
			((token->type != MG_TOKEN_EOF) && (token->type != MG_TOKEN_RPAREN) && (token->type != MG_TOKEN_RSQUARE) && (token->type != MG_TOKEN_COMMA)))
		{
			MGNode *block = mgCreateNode(token, MG_NODE_BLOCK);
			block->token = NULL;

			_mgParseBlock(parser, token, block, token->begin.character);

			if (_mgListLength(block->children) == 1)
				_mgAddChild(node, _mgDestroyNodeExtractFirst(block));
			else
				_mgAddChild(node, block);

			token = node->tokenEnd + 1;
		}
	}
	else if (token->type == MG_TOKEN_WHILE)
	{
		node = mgCreateNode(token, MG_NODE_WHILE);

		MGNode *condition = _mgParseExpression(parser, token + 1, MG_TRUE);
		MG_ASSERT(condition);
		_mgAddChild(node, condition);

		token = condition->tokenEnd + 1;
		_MG_TOKEN_SCAN_LINES(token);

		if ((node->tokenBegin->begin.character < token->begin.character) &&
		    ((token->type != MG_TOKEN_EOF) && (token->type != MG_TOKEN_RPAREN) && (token->type != MG_TOKEN_RSQUARE) && (token->type != MG_TOKEN_COMMA)))
		{
			MGNode *block = mgCreateNode(token, MG_NODE_BLOCK);
			block->token = NULL;

			_mgParseBlock(parser, token, block, token->begin.character);

			if (_mgListLength(block->children) == 1)
				_mgAddChild(node, _mgDestroyNodeExtractFirst(block));
			else
				_mgAddChild(node, block);

			token = node->tokenEnd + 1;
		}
	}
	else if (token->type == MG_TOKEN_IF)
	{
		MGToken *start = token;

		node = mgCreateNode(token, MG_NODE_IF);

		_mgAddChild(node, _mgParseExpression(parser, token + 1, MG_FALSE));

		token = node->tokenEnd + 1;
		_MG_TOKEN_SCAN_LINES(token);

		if ((token->type != MG_TOKEN_EOF) && (token->type != MG_TOKEN_RPAREN) && (token->type != MG_TOKEN_RSQUARE) && (token->type != MG_TOKEN_COMMA) &&
		    (start->begin.character < token->begin.character))
		{
			MGNode *block = mgCreateNode(token, MG_NODE_BLOCK);
			block->token = NULL;

			_mgParseBlock(parser, token, block, token->begin.character);

			if (_mgListLength(block->children) == 1)
				_mgAddChild(node, _mgDestroyNodeExtractFirst(block));
			else
				_mgAddChild(node, block);

			token = node->tokenEnd + 1;
			_MG_TOKEN_SCAN_LINES(token);
		}

		MGNode *_node = node;

		MGbool end = MG_FALSE;

		while (token->type == MG_TOKEN_ELSE)
		{
			if ((start->begin.character != token->begin.character) && (start->begin.line != token->begin.line))
				break;

			if (end)
				mgParserFatalError("Error: Cannot have consecutive else", NULL);

			end = MG_TRUE;

			_node->tokenEnd = token;
			start = token;

			++token;
			_MG_TOKEN_SCAN_LINE(token);

			if (token->type == MG_TOKEN_IF)
			{
				MGNode *parent = _node;

				_node = mgCreateNode(token, MG_NODE_IF);

				_mgAddChild(_node, _mgParseExpression(parser, token + 1, MG_FALSE));
				_mgAddChild(parent, _node);

				token = _node->tokenEnd + 1;

				end = MG_FALSE;
			}

			_MG_TOKEN_SCAN_LINES(token);

			if ((token->type != MG_TOKEN_EOF) && (token->type != MG_TOKEN_RPAREN) && (token->type != MG_TOKEN_RSQUARE) && (token->type != MG_TOKEN_COMMA) &&
			    (start->begin.character < token->begin.character))
			{
				MG_ASSERT(_mgListLength(_node->children) < 3);

				if (end && (_mgListLength(_node->children) == 1))
					_mgAddChild(_node, mgCreateNode(NULL, MG_NODE_NOP));

				MGNode *block = mgCreateNode(token, MG_NODE_BLOCK);
				block->token = NULL;

				_mgParseBlock(parser, token, block, token->begin.character);

				if (_mgListLength(block->children) == 1)
					_mgAddChild(_node, _mgDestroyNodeExtractFirst(block));
				else
					_mgAddChild(_node, block);

				MG_ASSERT((end && (_mgListLength(_node->children) == 3)) || (_mgListLength(_node->children) == 2));
			}

			token = _node->tokenEnd + 1;

			_MG_TOKEN_SCAN_LINES(token);
		}

		for (; _node->parent; _node = _node->parent)
			_node->parent->tokenEnd = _node->tokenEnd;

		return node;
	}
	else if ((token->type == MG_TOKEN_PROC) || (token->type == MG_TOKEN_FUNC))
	{
		node = mgCreateNode(token, (token->type == MG_TOKEN_PROC) ? MG_NODE_PROCEDURE : MG_NODE_FUNCTION);

		++token;
		_MG_TOKEN_SCAN_LINE(token);

		MGNode *name = NULL;
		if (token->type == MG_TOKEN_NAME)
		{
			name = mgCreateNode(token++, MG_NODE_NAME);

			_MG_TOKEN_SCAN_LINE(token);

			while (token->type == MG_TOKEN_DOT)
			{
				++token;
				_MG_TOKEN_SCAN_LINE(token);
				MG_ASSERT(token->type == MG_TOKEN_NAME);

				name = _mgWrapNode(name, MG_NODE_ATTRIBUTE);

				_mgAddChild(name, mgCreateNode(token++, MG_NODE_NAME));
			}

			_mgAddChild(node, name);

			_MG_TOKEN_SCAN_LINE(token);
		}
		else
		{
			// TODO: Invalid is not the best way to differentiate between anonymous functions
			name = mgCreateNode(NULL, MG_NODE_INVALID);
			_mgAddChild(node, name);
		}

		MG_ASSERT(token->type == MG_TOKEN_LPAREN);

		MGNode *parameters = mgCreateNode(token++, MG_NODE_TUPLE);
		_mgParseExpressionList(parser, token, parameters, MG_TOKEN_RPAREN);
		_mgAddChild(node, parameters);

		for (size_t i = 0; i < _mgListLength(parameters->children); ++i)
		{
			const MGNode *parameter = _mgListGet(parameters->children, i);
			MG_ASSERT(parameter);
			MG_ASSERT((parameter->type == MG_NODE_NAME) || (parameter->type == MG_NODE_ASSIGN));

			const MGbool parameterHasDefaultArgument = parameter->type == MG_NODE_ASSIGN;
			const MGToken *parameterNameToken = parameterHasDefaultArgument ? _mgListGet(parameter->children, 0)->token : parameter->token;
			const char *parameterName = parameterNameToken->begin.string;
			const size_t parameterNameLength = parameterNameToken->end.string - parameterNameToken->begin.string;

			for (size_t j = i + 1; j < _mgListLength(parameters->children); ++j)
			{
				const MGNode *parameter2 = _mgListGet(parameters->children, j);
				MG_ASSERT(parameter2);
				MG_ASSERT((parameter2->type == MG_NODE_NAME) || (parameter2->type == MG_NODE_ASSIGN));

				const MGbool parameter2HasDefaultArgument = parameter2->type == MG_NODE_ASSIGN;
				const MGToken *parameter2NameToken = parameter2HasDefaultArgument ? _mgListGet(parameter2->children, 0)->token : parameter2->token;
				const char *parameter2Name = parameter2NameToken->begin.string;
				const size_t parameter2NameLength = parameter2NameToken->end.string - parameter2NameToken->begin.string;

				if ((parameterNameLength == parameter2NameLength) && !strncmp(parameterName, parameter2Name, parameterNameLength))
					mgParserFatalErrorEx(parameterNameToken, "Duplicate parameter \"%.*s\"", (int) parameterNameLength, parameterName);

				if (parameterHasDefaultArgument && !parameter2HasDefaultArgument)
					mgParserFatalErrorEx(parameterNameToken, "Default argument missing for parameter %zu \"%.*s\"", j + 1, (int) parameter2NameLength, parameter2Name);
			}
		}

		token = node->tokenEnd + 1;

		if (token->type == MG_TOKEN_COLON)
		{
			node->tokenEnd = _mgParseTypedName(parser, ++token, name);
			token = node->tokenEnd + 1;
		}

		_MG_TOKEN_SCAN_LINES(token);

		if ((token->type != MG_TOKEN_EOF) && (token->type != MG_TOKEN_RPAREN) && (token->type != MG_TOKEN_RSQUARE) && (token->type != MG_TOKEN_COMMA))
		{
			MG_ASSERT(node->token);
			MG_ASSERT(name);

			if ((node->token->begin.character < token->begin.character) || (name->type == MG_NODE_INVALID))
			{
				MGNode *block = mgCreateNode(token, MG_NODE_BLOCK);
				block->token = NULL;

				_mgParseBlock(parser, token, block, token->begin.character);

				if (_mgListLength(block->children) == 1)
					_mgAddChild(node, _mgDestroyNodeExtractFirst(block));
				else
					_mgAddChild(node, block);
			}
		}

		return node;
	}
	else if ((token->type == MG_TOKEN_RETURN) || (token->type == MG_TOKEN_EMIT) || (token->type == MG_TOKEN_BREAK) || (token->type == MG_TOKEN_CONTINUE))
	{
		node = mgCreateNode(token, MG_NODE_INVALID);

		switch (token->type)
		{
		case MG_TOKEN_RETURN:
			node->type = MG_NODE_RETURN;
			break;
		case MG_TOKEN_EMIT:
			node->type = MG_NODE_EMIT;
			break;
		case MG_TOKEN_BREAK:
			node->type = MG_NODE_BREAK;
			break;
		case MG_TOKEN_CONTINUE:
			node->type = MG_NODE_CONTINUE;
			break;
		default:
			break;
		}

		if (token->type != MG_TOKEN_CONTINUE)
		{
			++token;
			_MG_TOKEN_SCAN_LINE(token);

			if ((token->type != MG_TOKEN_EOF) && (token->type != MG_TOKEN_NEWLINE) && (token->type != MG_TOKEN_RPAREN) && (token->type != MG_TOKEN_RSQUARE) && (token->type != MG_TOKEN_COMMA))
				_mgAddChild(node, _mgParseExpression(parser, token, MG_TRUE));
		}

		return node;
	}
	else if (token->type == MG_TOKEN_DELETE)
	{
		node = mgCreateNode(token, MG_NODE_DELETE);
		_mgAddChild(node, _mgParseExpression(parser, ++token, MG_TRUE));

		MG_ASSERT(_mgListLength(node->children) == 1);

		return node;
	}
	else if ((token->type == MG_TOKEN_IMPORT) || (token->type == MG_TOKEN_FROM))
		return _mgParseImport(parser, token);
	else if (token->type == MG_TOKEN_ASSERT)
	{
		node = mgCreateNode(token, MG_NODE_ASSERT);
		node->token = NULL;

		_mgAddChild(node, _mgParseExpression(parser, ++token, MG_FALSE));

		token = node->tokenEnd + 1;
		_MG_TOKEN_SCAN_LINE(token);

		if (token->type == MG_TOKEN_COMMA)
			_mgAddChild(node, _mgParseExpression(parser, ++token, MG_FALSE));

		MG_ASSERT((_mgListLength(node->children) == 1) || (_mgListLength(node->children) == 2));

		return node;
	}
	else if (token->type == MG_TOKEN_NULL)
		return mgCreateNode(token, MG_NODE_NULL);

	MG_ASSERT(node);

	for (;;)
	{
		_MG_TOKEN_SCAN_LINE(token);

		if (token->type == MG_TOKEN_LPAREN)
		{
			node = _mgWrapNode(node, MG_NODE_CALL);
			++token;

			_mgParseExpressionList(parser, token, node, MG_TOKEN_RPAREN);

			token = node->tokenEnd + 1;
		}
		else if (token->type == MG_TOKEN_LSQUARE)
		{
			node = _mgWrapNode(node, MG_NODE_SUBSCRIPT);
			++token;

			_mgParseExpressionList(parser, token, node, MG_TOKEN_RSQUARE);

			token = node->tokenEnd + 1;
		}
		else if (token->type == MG_TOKEN_DOT)
		{
			node = _mgWrapNode(node, MG_NODE_ATTRIBUTE);
			++token;

			_MG_TOKEN_SCAN_LINE(token);
			MG_ASSERT(token->type == MG_TOKEN_NAME);

			_mgAddChild(node, mgCreateNode(token++, MG_NODE_NAME));
		}
		else if (token->type == MG_TOKEN_AS)
		{
			node = _mgWrapNode(node, MG_NODE_AS);
			++token;

			_MG_TOKEN_SCAN_LINE(token);
			MG_ASSERT(token->type == MG_TOKEN_NAME);

			_mgAddChild(node, mgCreateNode(token++, MG_NODE_NAME));
		}
		else
			break;
	}

	return node;
}


static MGNode* _mgParseBinaryOperation(MGParser *parser, MGToken *token, int level)
{
	MGNode *node = (level < (_MG_OPERATOR_PRECEDENCE_LEVELS - 1)) ?
	               _mgParseBinaryOperation(parser, token, level + 1) :
	               _mgParseSubexpression(parser, token);
	MG_ASSERT(node);

	for (;;)
	{
		token = node->tokenEnd + 1;
		_MG_TOKEN_SCAN_LINE(token);

		int type = -1;

		for (int i = 0; i < _MG_OPERATOR_PRECEDENCE_TYPE_COUNT[level]; ++i)
		{
			if (token->type == _MG_OPERATOR_PRECEDENCE_TYPES[level][i])
			{
				type = i;
				break;
			}
		}

		if (type == -1)
			break;

		node = _mgWrapNode(node, _MG_BIN_OP_NODE_TYPES[level][type]);
		node->tokenEnd = token;

		++token;

		MGNode *child = (level < (_MG_OPERATOR_PRECEDENCE_LEVELS - 1)) ?
		                _mgParseBinaryOperation(parser, token, level + 1) :
		                _mgParseSubexpression(parser, token);
		MG_ASSERT(child);
		_mgAddChild(node, child);

		MG_ASSERT(_mgListLength(node->children) == 2);
	}

	return node;
}


static MGNode* _mgParseConditional(MGParser *parser, MGToken *token)
{
	MGNode *node = _mgParseBinaryOperation(parser, token, _MG_OPERATOR_PRECEDENCE_LEVEL_AFTER_RANGE);

	token = node->tokenEnd + 1;
	_MG_TOKEN_SCAN_LINE(token);

	if (token->type == MG_TOKEN_QUESTION)
	{
		node = _mgWrapNode(node, MG_NODE_TERNARY_OP_CONDITIONAL);
		node->tokenEnd = token;

		_mgAddChild(node, _mgParseBinaryOperation(parser, ++token, _MG_OPERATOR_PRECEDENCE_LEVEL_AFTER_RANGE));

		token = node->tokenEnd + 1;
		_MG_TOKEN_SCAN_LINE(token);

		MG_ASSERT(token->type == MG_TOKEN_COLON);

		_mgAddChild(node, _mgParseBinaryOperation(parser, ++token, _MG_OPERATOR_PRECEDENCE_LEVEL_AFTER_RANGE));
	}
	else if (token->type == MG_TOKEN_ELVIS)
	{
		node = _mgWrapNode(node, MG_NODE_BIN_OP_CONDITIONAL);
		node->tokenEnd = token;

		_mgAddChild(node, _mgParseBinaryOperation(parser, ++token, _MG_OPERATOR_PRECEDENCE_LEVEL_AFTER_RANGE));
	}

	return node;
}


static MGNode* _mgParseRange(MGParser *parser, MGToken *token)
{
	MGNode *node = _mgParseConditional(parser, token);

	token = node->tokenEnd + 1;
	_MG_TOKEN_SCAN_LINE(token);

	if (token->type == MG_TOKEN_COLON)
	{
		node = _mgWrapNode(node, MG_NODE_RANGE);
		++token;

		_mgAddChild(node, _mgParseConditional(parser, token));

		token = node->tokenEnd + 1;
		_MG_TOKEN_SCAN_LINE(token);

		if (token->type == MG_TOKEN_COLON)
			_mgAddChild(node, _mgParseConditional(parser, ++token));
	}

	return node;
}


static MGNode* _mgParseExpression(MGParser *parser, MGToken *token, MGbool eatTuple)
{
	MGNode *node = _mgParseRange(parser, token);
	MG_ASSERT(node);

	token = node->tokenEnd + 1;
	_MG_TOKEN_SCAN_LINE(token);

	if (token->type == MG_TOKEN_ARROW)
	{
		if (node->type != MG_NODE_TUPLE)
			node = _mgWrapNode(node, MG_NODE_TUPLE);

		MGNode *func = mgCreateNode(token, MG_NODE_FUNCTION);
		_mgAddChild(func, mgCreateNode(NULL, MG_NODE_INVALID));
		_mgAddChild(func, node);

		node = mgCreateNode(token, MG_NODE_RETURN);
		_mgAddChild(node, _mgParseExpression(parser, ++token, MG_FALSE));
		_mgAddChild(func, node);

		node = func;

		token = node->tokenEnd + 1;
	}

	if (eatTuple)
	{
		_MG_TOKEN_SCAN_LINE(token);

		if (token->type == MG_TOKEN_COMMA)
		{
			node = _mgWrapNode(node, MG_NODE_TUPLE);
			node->tokenEnd = token++;

			_mgParseTuple(parser, token, node);
		}
	}

	return node;
}


static MGNode* _mgParseAssignment(MGParser *parser, MGToken *token, int level, MGbool eatTuple)
{
	MGNode *node = (level < (_MG_OPERATOR_PRECEDENCE_LEVEL_AFTER_RANGE - 1)) ?
	               _mgParseAssignment(parser, token, level + 1, eatTuple) :
	               _mgParseExpression(parser, token, eatTuple);
	MG_ASSERT(node);

	token = node->tokenEnd + 1;
	_MG_TOKEN_SCAN_LINE(token);

	int type = -1;

	for (int i = 0; i < _MG_OPERATOR_PRECEDENCE_TYPE_COUNT[level]; ++i)
	{
		if (token->type == _MG_OPERATOR_PRECEDENCE_TYPES[level][i])
		{
			type = i;
			break;
		}
	}

	if (type == -1)
		return node;

	node = _mgWrapNode(node, _MG_ASSIGN_NODE_TYPES[type]);
	node->tokenEnd = token;

	++token;

	MGNode *child = (level < (_MG_OPERATOR_PRECEDENCE_LEVEL_AFTER_RANGE - 1)) ?
	                _mgParseAssignment(parser, token, level + 1, eatTuple) :
	                _mgParseExpression(parser, token, eatTuple);
	MG_ASSERT(child);
	_mgAddChild(node, child);

	MG_ASSERT(_mgListLength(node->children) == 2);

	const MGNode *target = _mgListGet(node->children, 0);

	if (node->type == MG_NODE_ASSIGN)
	{
		if ((target->type != MG_NODE_NAME) && (target->type != MG_NODE_SUBSCRIPT) && (target->type != MG_NODE_ATTRIBUTE) && (target->type != MG_NODE_TUPLE))
			mgParserFatalError("Illegal assignment to \"%s\"", _MG_NODE_NAMES[target->type]);
	}
	else
	{
		if ((target->type != MG_NODE_NAME) && (target->type != MG_NODE_SUBSCRIPT) && (target->type != MG_NODE_ATTRIBUTE))
			mgParserFatalError("Illegal augmented assignment to \"%s\"", _MG_NODE_NAMES[target->type]);
	}

	return node;
}


#if defined(__GNUC__)
static inline __attribute__((always_inline)) MGNode* _mgParseAssignmentOrExpression(MGParser *parser, MGToken *token, MGbool eatTuple)
#elif defined(_MSC_VER)
static __forceinline MGNode* _mgParseAssignmentOrExpression(MGParser *parser, MGToken *token, MGbool eatTuple)
#else
static inline MGNode* _mgParseAssignmentOrExpression(MGParser *parser, MGToken *token, MGbool eatTuple)
#endif
{
	return _mgParseAssignment(parser, token, 0, eatTuple);
}


static MGNode* _mgParseModule(MGParser *parser, MGToken *token)
{
	MGNode *node = mgCreateNode(token, MG_NODE_MODULE);
	node->token = NULL;

	_MG_TOKEN_SCAN_LINES(token);

	while (token->type != MG_TOKEN_EOF)
	{
		_mgParseBlock(parser, token, node, token->begin.character);

		token = node->tokenEnd + 1;
		_MG_TOKEN_SCAN_LINES(token);
	}

	node->tokenEnd = token;

	if (token->type != MG_TOKEN_EOF)
	{
#if MG_ANSI_COLORS
		fputs("\e[90m", stdout);
#endif

		printf("Error: Unexpected token, expected %s\n", _MG_TOKEN_NAMES[MG_TOKEN_EOF]);
		mgInspectTokenEx(token, parser->tokenizer.filename, MG_FALSE);

#if MG_ANSI_COLORS
		fputs("\e[0m", stdout);
#endif

		MG_ASSERT(token->type != MG_TOKEN_EOF);
	}

	return node;
}


inline MGNode* mgParse(MGParser *parser)
{
	MG_ASSERT(parser->tokenizer.filename);

	parser->root = _mgParseModule(parser, parser->tokenizer.tokens.items);

	return parser->root;
}


MGNode* mgParseFile(MGParser *parser, const char *filename)
{
	if (!mgTokenizeFile(&parser->tokenizer, filename, NULL))
		return NULL;

	return mgParse(parser);
}


MGNode* mgParseFileHandle(MGParser *parser, FILE *file)
{
	if (!mgTokenizeFileHandle(&parser->tokenizer, file, NULL))
		return NULL;

	return mgParse(parser);
}


MGNode* mgParseString(MGParser *parser, const char *string)
{
	if (!mgTokenizeString(&parser->tokenizer, string, NULL))
		return NULL;

	return mgParse(parser);
}
