
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


static inline void _mgFail(MGParser *parser, MGToken *token, const char *format, ...)
{
	fflush(stdout);

	if (parser->tokenizer.filename)
		fprintf(stderr, "%s:", parser->tokenizer.filename);
	if (token)
		fprintf(stderr, "%u:%u: ", token->begin.line, token->begin.character);

	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);

	putc('\n', stderr);
	fflush(stderr);

	exit(1);
}

#define MG_FAIL(...) _mgFail(parser, token, __VA_ARGS__)


#define _MG_TOKEN_SCAN_LINE(token)  for (; (token->type == MG_TOKEN_WHITESPACE) || (token->type == MG_TOKEN_COMMENT); ++token)
#define _MG_TOKEN_SCAN_LINES(token) for (; (token->type == MG_TOKEN_NEWLINE) || (token->type == MG_TOKEN_WHITESPACE) || (token->type == MG_TOKEN_COMMENT); ++token)


#define _MG_OPERATOR_PRECEDENCE_LEVELS 7
#define _MG_OPERATOR_PRECEDENCE_LONGEST_LEVEL 6

static const int _MG_OPERATOR_PRECEDENCE_TYPE_COUNT[_MG_OPERATOR_PRECEDENCE_LEVELS] = { 6, 1, 1, 2, 4, 2, 3 };

static const MGTokenType _MG_OPERATOR_PRECEDENCE_TYPES[_MG_OPERATOR_PRECEDENCE_LEVELS][_MG_OPERATOR_PRECEDENCE_LONGEST_LEVEL] = {
	/* Highest Precedence */
	{ MG_TOKEN_ASSIGN, MG_TOKEN_ADD_ASSIGN, MG_TOKEN_SUB_ASSIGN, MG_TOKEN_MUL_ASSIGN, MG_TOKEN_DIV_ASSIGN, MG_TOKEN_MOD_ASSIGN },
	{ MG_TOKEN_OR },
	{ MG_TOKEN_AND },
	{ MG_TOKEN_EQUAL, MG_TOKEN_NOT_EQUAL },
	{ MG_TOKEN_LESS, MG_TOKEN_LESS_EQUAL, MG_TOKEN_GREATER, MG_TOKEN_GREATER_EQUAL },
	{ MG_TOKEN_ADD, MG_TOKEN_SUB },
	{ MG_TOKEN_MUL, MG_TOKEN_DIV, MG_TOKEN_MOD }
	/* Lowest Precedence */
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


MGNode* mgCreateNode(MGToken *token)
{
	MGNode *node = (MGNode*) calloc(1, sizeof(MGNode));

	node->token = token;
	node->tokenBegin = token;
	node->tokenEnd = token;

	return node;
}


void mgDestroyNode(MGNode *node)
{
	for (size_t i = 0; i < node->childCount; ++i)
		if (node->children[i])
			mgDestroyNode(node->children[i]);
	free(node->children);

	free(node);
}


static void _mgAddChild(MGNode *parent, MGNode *child)
{
	MG_ASSERT(parent);
	MG_ASSERT(child);

	if (parent->childCapacity == parent->childCount)
	{
		parent->childCapacity = parent->childCapacity ? parent->childCapacity << 1 : 2;
		parent->children = (MGNode**) realloc(parent->children, parent->childCapacity * sizeof(MGNode*));
	}

	parent->tokenEnd = child->tokenEnd;
	parent->children[parent->childCount++] = child;
	child->parent = parent;
}


static inline MGNode* _mgWrapNode(MGToken *token, MGNode *node)
{
	MGNode *parent = mgCreateNode(node->tokenBegin);
	parent->tokenBegin = node->tokenBegin;
	parent->token = NULL;

	_mgAddChild(parent, node);

	return parent;
}


static inline MGNode* _mgDestroyNodeExtractFirst(MGNode *node)
{
	MGNode *child = node->children[0];
	node->children[0] = NULL;

	child->tokenBegin = node->tokenBegin;
	child->tokenEnd = node->tokenEnd;

	mgDestroyNode(node);

	return child;
}


static MGNode* _mgParseExpression(MGParser *parser, MGToken *token);


static void _mgParseBlock(MGParser *parser, MGToken *token, MGNode *node, unsigned int indentation)
{
	MG_ASSERT(node);

	while (indentation == token->begin.character)
	{
		if ((token->type == MG_TOKEN_EOF) || (token->type == MG_TOKEN_RPAREN) || (token->type == MG_TOKEN_RSQUARE) || (token->type == MG_TOKEN_COMMA))
			break;

		MGNode *expr = _mgParseExpression(parser, token);
		MG_ASSERT(expr);
		_mgAddChild(node, expr);

		token = expr->tokenEnd + 1;
		_MG_TOKEN_SCAN_LINES(token);

		if (indentation < token->begin.character)
		{
			if ((token->type == MG_TOKEN_EOF) || (token->type == MG_TOKEN_RPAREN) || (token->type == MG_TOKEN_RSQUARE) || (token->type == MG_TOKEN_COMMA))
				break;

			MGNode *block = mgCreateNode(token);
			block->type = MG_NODE_BLOCK;
			block->token = NULL;

			_mgParseBlock(parser, token, block, token->begin.character);

			if (block->childCount == 1)
				_mgAddChild(node, _mgDestroyNodeExtractFirst(block));
			else
				_mgAddChild(node, block);

			token = node->tokenEnd + 1;
			_MG_TOKEN_SCAN_LINES(token);
		}
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

		MGNode *child = _mgParseExpression(parser, token);
		MG_ASSERT(child);
		_mgAddChild(node, child);

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


static MGNode* _mgParseSubexpression(MGParser *parser, MGToken *token)
{
	_MG_TOKEN_SCAN_LINES(token);

	MGNode *node = NULL;

	if ((token->type == MG_TOKEN_INTEGER) ||
		(token->type == MG_TOKEN_FLOAT))
	{
		node = mgCreateNode(token);
		node->type = (token->type == MG_TOKEN_INTEGER) ? MG_NODE_INTEGER : MG_NODE_FLOAT;
		node->tokenEnd = token++;
	}
	else if (token->type == MG_TOKEN_IDENTIFIER)
	{
		node = mgCreateNode(token);
		node->type = MG_NODE_IDENTIFIER;
		node->tokenEnd = token++;
	}
	else if (token->type == MG_TOKEN_STRING)
	{
		node = mgCreateNode(token);
		node->type = MG_NODE_STRING;
		node->tokenEnd = token++;
	}
	else if ((token->type == MG_TOKEN_SUB) ||
	         (token->type == MG_TOKEN_ADD) ||
	         (token->type == MG_TOKEN_NOT))
	{
		node = mgCreateNode(token);
		node->type = MG_NODE_UNARY_OP;
		++token;

		_mgAddChild(node, _mgParseSubexpression(parser, token));

		MG_ASSERT(node->childCount == 1);

		token = node->tokenEnd + 1;
	}
	else if (token->type == MG_TOKEN_LPAREN)
	{
		node = mgCreateNode(token);
		node->type = MG_NODE_TUPLE;
		++token;

		if (!_mgParseExpressionList(parser, token, node, MG_TOKEN_RPAREN) && (node->childCount == 1))
			node = _mgDestroyNodeExtractFirst(node);

		token = node->tokenEnd + 1;
	}
	else if (token->type == MG_TOKEN_LSQUARE)
	{
		node = mgCreateNode(token);
		node->type = MG_NODE_LIST;
		++token;

		_mgParseExpressionList(parser, token, node, MG_TOKEN_RSQUARE);

		token = node->tokenEnd + 1;
	}
	else if (token->type == MG_TOKEN_FOR)
	{
		node = mgCreateNode(token);
		node->type = MG_NODE_FOR;

		MGNode *name = _mgParseSubexpression(parser, token + 1);
		MG_ASSERT(name);
		_mgAddChild(node, name);

		token = name->tokenEnd + 1;
		_MG_TOKEN_SCAN_LINE(token);

		MG_ASSERT(token->type == MG_TOKEN_IN);
		++token;

		MGNode *test = _mgParseSubexpression(parser, token);
		MG_ASSERT(test);
		_mgAddChild(node, test);

		token = test->tokenEnd + 1;
		_MG_TOKEN_SCAN_LINES(token);

		if (node->tokenBegin->begin.character < token->begin.character)
		{
			MGNode *block = mgCreateNode(token);
			block->type = MG_NODE_BLOCK;
			block->token = NULL;

			_mgParseBlock(parser, token, block, token->begin.character);

			if (block->childCount == 1)
				_mgAddChild(node, _mgDestroyNodeExtractFirst(block));
			else
				_mgAddChild(node, block);

			token = node->tokenEnd + 1;
		}
	}
	else if (token->type == MG_TOKEN_IF)
	{
		MGToken *start = token;

		node = mgCreateNode(token);
		node->type = MG_NODE_IF;

		MGNode *condition = _mgParseExpression(parser, token + 1);
		MG_ASSERT(condition);
		_mgAddChild(node, condition);

		token = node->tokenEnd + 1;
		_MG_TOKEN_SCAN_LINES(token);

		if (start->begin.character >= token->begin.character)
			MG_FAIL("Error: Expected indentation");

		MGNode *block = mgCreateNode(token);
		block->type = MG_NODE_BLOCK;
		block->token = NULL;

		_mgParseBlock(parser, token, block, token->begin.character);

		if (block->childCount == 1)
			_mgAddChild(node, _mgDestroyNodeExtractFirst(block));
		else
			_mgAddChild(node, block);

		token = node->tokenEnd + 1;
		_MG_TOKEN_SCAN_LINES(token);

		MGNode *_node = node;

		MGbool end = MG_FALSE;

		while (token->type == MG_TOKEN_ELSE)
		{
			if (end)
				MG_FAIL("Error: Cannot have consecutive else");

			end = MG_TRUE;

			start = token;

			++token;
			_MG_TOKEN_SCAN_LINE(token);

			if (token->type == MG_TOKEN_IF)
			{
				MGNode *parent = _node;

				_node = mgCreateNode(token);
				_node->type = MG_NODE_IF;

				condition = _mgParseExpression(parser, token + 1);
				MG_ASSERT(condition);
				_mgAddChild(_node, condition);

				_mgAddChild(parent, _node);

				token = _node->tokenEnd + 1;

				end = MG_FALSE;
			}

			_MG_TOKEN_SCAN_LINES(token);

			if (start->begin.character >= token->begin.character)
				MG_FAIL("Error: Expected indentation");

			block = mgCreateNode(token);
			block->type = MG_NODE_BLOCK;
			block->token = NULL;

			_mgParseBlock(parser, token, block, token->begin.character);

			if (block->childCount == 1)
				_mgAddChild(_node, _mgDestroyNodeExtractFirst(block));
			else
				_mgAddChild(_node, block);

			token = _node->tokenEnd + 1;

			_MG_TOKEN_SCAN_LINES(token);
		}

		for (; _node->parent; _node = _node->parent)
			_node->parent->tokenEnd = _node->tokenEnd;

		token = node->tokenEnd;
	}
	else if (token->type == MG_TOKEN_PROC)
	{
		node = mgCreateNode(token);
		node->type = MG_NODE_PROCEDURE;

		++token;
		_MG_TOKEN_SCAN_LINE(token);

		MGNode *name = NULL;
		if (token->type == MG_TOKEN_IDENTIFIER)
		{
			name = mgCreateNode(token);
			name->type = MG_NODE_IDENTIFIER;
			_mgAddChild(node, name);

			++token;
			_MG_TOKEN_SCAN_LINE(token);
		}
		else
		{
			// TODO: Invalid is not the best way to differentiate between anonymous procedures
			name = mgCreateNode(token);
			name->type = MG_NODE_INVALID;
			_mgAddChild(node, name);
		}

		MG_ASSERT(token->type == MG_TOKEN_LPAREN);

		MGNode *parameters = mgCreateNode(token);
		parameters->type = MG_NODE_TUPLE;
		++token;
		_mgParseExpressionList(parser, token, parameters, MG_TOKEN_RPAREN);
		_mgAddChild(node, parameters);

		token = node->tokenEnd + 1;
		_MG_TOKEN_SCAN_LINES(token);

		if ((token->type != MG_TOKEN_RPAREN) && (token->type != MG_TOKEN_RSQUARE) && (token->type != MG_TOKEN_COMMA))
		{
			if ((node->token->begin.character < token->begin.character) || (name->type == MG_NODE_INVALID))
			{
				MGNode *block = mgCreateNode(token);
				block->type = MG_NODE_BLOCK;
				block->token = NULL;

				_mgParseBlock(parser, token, block, token->begin.character);

				if (block->childCount == 1)
					_mgAddChild(node, _mgDestroyNodeExtractFirst(block));
				else
					_mgAddChild(node, block);

				token = node->tokenEnd + 1;
			}
		}
	}

	MG_ASSERT(node);

	_MG_TOKEN_SCAN_LINE(token);

	if (token->type == MG_TOKEN_COLON)
	{
		node = _mgWrapNode(token, node);
		node->type = MG_NODE_RANGE;

		++token;
		_MG_TOKEN_SCAN_LINE(token);

		MG_ASSERT(token->type == MG_TOKEN_INTEGER);
		MGNode *child = mgCreateNode(token);
		child->type = MG_NODE_INTEGER;
		_mgAddChild(node, child);

		++token;
		_MG_TOKEN_SCAN_LINE(token);

		if (token->type == MG_TOKEN_COLON)
		{
			++token;
			_MG_TOKEN_SCAN_LINE(token);

			MG_ASSERT(token->type == MG_TOKEN_INTEGER);
			child = mgCreateNode(token);
			child->type = MG_NODE_INTEGER;
			_mgAddChild(node, child);
		}
	}

	token = node->tokenEnd + 1;

	for (;;)
	{
		_MG_TOKEN_SCAN_LINE(token);

		if (token->type == MG_TOKEN_LPAREN)
		{
			node = _mgWrapNode(token, node);
			node->type = MG_NODE_CALL;
			++token;

			_mgParseExpressionList(parser, token, node, MG_TOKEN_RPAREN);

			token = node->tokenEnd + 1;
		}
		else if (token->type == MG_TOKEN_LSQUARE)
		{
			node = _mgWrapNode(token, node);
			node->type = MG_NODE_INDEX;
			++token;

			_mgParseExpressionList(parser, token, node, MG_TOKEN_RSQUARE);

			token = node->tokenEnd + 1;
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

		MGbool match = MG_FALSE;

		for (int i = 0; i < _MG_OPERATOR_PRECEDENCE_TYPE_COUNT[level]; ++i)
		{
			if (token->type == _MG_OPERATOR_PRECEDENCE_TYPES[level][i])
			{
				match = MG_TRUE;
				break;
			}
		}

		if (!match)
			break;

		node = _mgWrapNode(token, node);
		node->type = MG_NODE_BIN_OP;
		node->token = token;
		node->tokenEnd = token;

		++token;

		MGNode *child = (level < (_MG_OPERATOR_PRECEDENCE_LEVELS - 1)) ?
		                _mgParseBinaryOperation(parser, token, level + 1) :
		                _mgParseSubexpression(parser, token);
		MG_ASSERT(child);
		_mgAddChild(node, child);

		MG_ASSERT(node->childCount == 2);
	}

	return node;
}


static inline MGNode* _mgParseExpression(MGParser *parser, MGToken *token)
{
	return _mgParseBinaryOperation(parser, token, 0);
}


static MGNode* _mgParseModule(MGParser *parser, MGToken *token)
{
	MGNode *node = mgCreateNode(token);
	node->type = MG_NODE_MODULE;
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
		mgInspectToken(token, parser->tokenizer.filename, MG_FALSE);

#if MG_ANSI_COLORS
		fputs("\e[0m", stdout);
#endif

		MG_ASSERT(token->type != MG_TOKEN_EOF);
	}

	return node;
}


static inline MGNode* _mgParse(MGParser *parser)
{
	parser->root = _mgParseModule(parser, parser->tokenizer.tokens);

	return parser->root;
}


MGNode* mgParseFile(MGParser *parser, const char *filename)
{
	if (!mgTokenizeFile(&parser->tokenizer, filename, NULL))
		return NULL;

	return _mgParse(parser);
}


MGNode* mgParseFileHandle(MGParser *parser, FILE *file)
{
	if (!mgTokenizeFileHandle(&parser->tokenizer, file, NULL))
		return NULL;

	return _mgParse(parser);
}


MGNode* mgParseString(MGParser *parser, const char *string)
{
	if (!mgTokenizeString(&parser->tokenizer, string, NULL))
		return NULL;

	return _mgParse(parser);
}
