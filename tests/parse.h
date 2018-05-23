#ifndef MODELGEN_TEST_PARSE_H
#define MODELGEN_TEST_PARSE_H

#include "inspect.h"
#include "utilities.h"

#include "test.h"


static size_t _mgCountNodes(MGNode *node)
{
	size_t count = 1;

	for (size_t i = 0; i < _mgListLength(node->children); ++i)
		count += _mgCountNodes(_mgListGet(node->children, i));

	return count;
}


static void _mgFlattenNodes(MGNode *node, MGNode **nodes)
{
	*nodes++ = node;

	for (size_t i = 0; i < _mgListLength(node->children); ++i)
	{
		_mgFlattenNodes(_mgListGet(node->children, i), nodes);
		nodes += _mgCountNodes(_mgListGet(node->children, i));
	}
}


static size_t _mgGetDepth(MGNode *node)
{
	size_t depth = 0;

	for (; node->parent; node = node->parent)
		++depth;

	return depth;
}


static void _mgTestParser(const char *in, const char *out)
{
	MGParser parser;
	size_t nodeCount;
	MGNode **nodes = NULL;

	char *expected = NULL;
	unsigned int line = 1;
	char *expectedLine;
	char *expectedNextLine;

	size_t expectedDepth;
	const char *expectedType;
	const char *expectedValue;
	char *expectedValueBuffer = NULL;

	size_t currentNodeIndex = 0;
	MGNode *currentNode;
	size_t currentDepth;
	const char *currentType;
	char *currentValue = NULL;

	mgCreateParser(&parser);

	if (!mgParseFile(&parser, in))
	{
		printf("Error: Failed parsing \"%s\"\n", in);
		goto fail;
	}
	else if (!(expected = mgReadFile(out, NULL)))
	{
		printf("Error: Failed reading \"%s\"\n", out);
		goto fail;
	}

	nodeCount = _mgCountNodes(parser.root);
	nodes = (MGNode**) malloc(nodeCount * sizeof(MGNode*));
	_mgFlattenNodes(parser.root, nodes);

	expectedLine = expected;

	while (expectedLine)
	{
		expectedNextLine = strchr(expectedLine, '\n');

		if (expectedNextLine)
			*expectedNextLine = '\0';

		while ((*expectedLine == ' ') || (*expectedLine == '\t'))
			++expectedLine;

		if (*expectedLine)
		{
			expectedDepth = 0;
			expectedType = NULL;
			expectedValue = NULL;

			while (*expectedLine == '-')
				++expectedLine, ++expectedDepth;

			while ((*expectedLine == ' ') || (*expectedLine == '\t'))
				++expectedLine;

			if (*expectedLine)
			{
				expectedType = expectedLine;

				while (*expectedLine && ((*expectedLine != ' ') && (*expectedLine != '\t'))) ++expectedLine;

				if (*expectedLine)
					*expectedLine++ = '\0';

				while ((*expectedLine == ' ') || (*expectedLine == '\t'))
					++expectedLine;
			}

			if (*expectedLine)
			{
				expectedValueBuffer = (char*) realloc(expectedValueBuffer, (strlen(expectedLine) + 1) * sizeof(char));
				strcpy(expectedValueBuffer, expectedLine);

				expectedValue = expectedValueBuffer;

				if (expectedValue[0] == '"')
					*strrchr(++expectedValue, '"') = '\0';
			}

			if (currentNodeIndex == nodeCount)
			{
				printf("%s:%u: Error: Expected node", mgBasename(out), line);

				if (expectedDepth)
					printf(" at depth %zu", expectedDepth);
				if (expectedType)
					printf(" of type \"%s\"", expectedType);

				putchar('\n');

				goto fail;
			}

			currentNode = nodes[currentNodeIndex++];
			currentDepth = _mgGetDepth(currentNode);
			currentType = _MG_NODE_NAMES[currentNode->type];

			if (currentDepth != expectedDepth)
			{
				printf("%s:%u: Error: Unexpected depth %zu, expected depth %zu\n",
				       mgBasename(out), line, currentDepth, expectedDepth);
				goto fail;
			}

			if (currentType && expectedType && strcmp(currentType, expectedType))
			{
				printf("%s:%u: Error: Unexpected type \"%s\", expected type \"%s\"\n",
				       mgBasename(out), line, currentType, expectedType);
				goto fail;
			}

			if (expectedValue)
			{
				if (currentNode->token)
				{
					if (currentNode->token->type == MG_TOKEN_STRING)
					{
						if (currentNode->token->value.s)
						{
							currentValue = (char*) realloc(currentValue, (mgInlineRepresentationLength(currentNode->token->value.s, NULL) + 1) * sizeof(char));
							mgInlineRepresentation(currentValue, currentNode->token->value.s, NULL);
						}
						else
						{
							currentValue = (char*) realloc(currentValue, 1 * sizeof(char));
							currentValue[0] = '\0';
						}
					}
					else
					{
						currentValue = (char*) realloc(currentValue, (mgInlineRepresentationLength(currentNode->token->begin.string, currentNode->token->end.string) + 1) * sizeof(char));
						mgInlineRepresentation(currentValue, currentNode->token->begin.string, currentNode->token->end.string);
					}
				}
				else if (currentNode->tokenBegin && currentNode->tokenEnd)
				{
					currentValue = (char*) realloc(currentValue, (mgInlineRepresentationLength(currentNode->tokenBegin->begin.string, currentNode->tokenEnd->end.string) + 1) * sizeof(char));
					mgInlineRepresentation(currentValue, currentNode->tokenBegin->begin.string, currentNode->tokenEnd->end.string);
				}
				else
				{
					puts("Error: Unknown parser error");
					goto fail;
				}

				if (strcmp(currentValue, expectedValue))
				{
					const char *currentValueQuote = currentValue[0] == '"' ? "" : "\"";
					const char *expectedValueQuote = expectedValue[0] == '"' ? "" : "\"";

					printf("%s:%u: Error: Unexpected value %s%s%s, expected value %s%s%s\n",
					       mgBasename(out), line,
					       currentValueQuote, currentValue, currentValueQuote,
					       expectedValueQuote, expectedValue, expectedValueQuote);

					goto fail;
				}
			}
		}

		expectedLine = expectedNextLine ? (expectedNextLine + 1) : NULL;
		++line;
	}

	if (currentNodeIndex != nodeCount)
	{
		puts("Error: Unexpected node...");
		goto fail;
	}

	goto pass;

fail:

	puts("AST Dump:");
	mgInspectNode(parser.root);

	++_mgTestsFailed;

pass:

	free(expectedValueBuffer);
	free(currentValue);

	free(nodes);

	mgDestroyParser(&parser);
}


static void _mgParserTest(const MGTestCase *test)
{
	const char *in = ((char**) test->data)[0];
	const char *out = ((char**) test->data)[1];

	_mgTestParser(in, out);
}


static void mgRunParserTest(const char *in)
{
	char out[MG_PATH_MAX + 1];
	const char *files[2] = { in, out };

	if (!mgStringEndsWith(in, ".mg"))
		return;

	strcpy(out, in);
	strcpy(strrchr(out, '.'), ".ast");

	if (!mgFileExists(out))
	{
		mgDirname(out, in);
		strcat(out, "/_");
		strcat(out, mgBasename(in));
		strcpy(strrchr(out, '.'), ".ast");
	}

	MGTestCase test;
	test.name = out;
	test.skip = mgBasename(out)[0] == '_';
	test.func = _mgParserTest;
	test.data = (void*) files;

	if (mgFileExists(out))
		mgRunTestCase(&test);
}


static inline void mgRunParserTests(void)
{
	mgWalkFiles("tests/fixtures/", mgRunParserTest);
}

#endif