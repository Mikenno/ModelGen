#ifndef MODELGEN_TEST_TOKENIZE_H
#define MODELGEN_TEST_TOKENIZE_H

#include "../src/modelgen.h"
#include "../src/debug.h"

#include "test.h"
#include "file.h"


#define _MG_IS_TESTABLE_TOKEN(token) ((token->type != MG_TOKEN_NEWLINE) && (token->type != MG_TOKEN_WHITESPACE))
#define _MG_FIND_NEXT_TESTABLE_TOKEN(token) for (; !_MG_IS_TESTABLE_TOKEN(token); ++token)

#define _MG_TOKEN_SCAN_LINE(token) for (; token->type == MG_TOKEN_WHITESPACE; ++token)

#define _MG_VALUE_BUFFER_LENGTH 32


static MGbool _mgTestTokenizer(const char *in, const char *out)
{
	MGTokenizer inTokenizer, outTokenizer;
	MGToken *inToken, *outToken;

	// Warning: This could overflow, but is highly unlikely
	char buffer[_MG_VALUE_BUFFER_LENGTH];

	MGbool passed = MG_TRUE;

	mgCreateTokenizer(&inTokenizer);
	mgCreateTokenizer(&outTokenizer);

	if (!mgTokenizeFile(&inTokenizer, in, NULL))
	{
		printf("Error: Failed tokenizing \"%s\"\n", in);
		goto fail;
	}
	else if (!mgTokenizeFile(&outTokenizer, out, NULL))
	{
		printf("Error: Failed tokenizing \"%s\"\n", out);
		goto fail;
	}

	outToken = outTokenizer.tokens;

	for (inToken = inTokenizer.tokens;; ++inToken)
	{
		if (!_MG_IS_TESTABLE_TOKEN(inToken))
			continue;

		_MG_FIND_NEXT_TESTABLE_TOKEN(outToken);

		if (outToken->type == MG_TOKEN_EOF)
		{
			if (inToken->type == MG_TOKEN_EOF)
				break;

			printf("Error: Unexpected token...\n");
			mgDebugInspectToken(inToken, inTokenizer.filename, MG_FALSE);
			goto fail;
		}
		else if (outToken->type != MG_TOKEN_STRING)
		{
			printf("Error: Token type must be a string\n");
			mgDebugInspectToken(outToken, outTokenizer.filename, MG_FALSE);
			goto fail;
		}

		if (!outToken->value.s || strcmp(_MG_TOKEN_NAMES[inToken->type], outToken->value.s))
		{
			printf("Error: Unexpected token type...\n");
			mgDebugInspectToken(inToken, inTokenizer.filename, MG_FALSE);
			printf("Expected...\n");
			mgDebugInspectToken(outToken, outTokenizer.filename, MG_FALSE);
			goto fail;
		}

		++outToken;
		_MG_TOKEN_SCAN_LINE(outToken);

		if (outToken->type == MG_TOKEN_STRING)
		{
			const char *outTokenString = outToken->value.s;
			const size_t outTokenStringLength = outTokenString ? strlen(outTokenString) : 0;

			const char *inTokenString = inToken->begin.string;
			size_t inTokenStringLength = inToken->end.string - inToken->begin.string;

			if (inToken->type == MG_TOKEN_STRING)
			{
				inTokenString = inToken->value.s;

				if (inTokenString)
					inTokenStringLength = strlen(inTokenString);
				else
				{
					inTokenString = NULL;
					inTokenStringLength = 0;
				}
			}

			if ((outTokenStringLength != inTokenStringLength) || ((outTokenStringLength > 0) && strncmp(outTokenString, inTokenString, outTokenStringLength)))
			{
				printf("Error: Unexpected token value...\n");
				mgDebugInspectToken(inToken, inTokenizer.filename, MG_FALSE);
				printf("Expected...\n");
				mgDebugInspectToken(outToken, outTokenizer.filename, MG_FALSE);
				goto fail;
			}

			++outToken;
			_MG_TOKEN_SCAN_LINE(outToken);
		}

		if (outToken->type == MG_TOKEN_INTEGER)
		{
			snprintf(buffer, _MG_VALUE_BUFFER_LENGTH, "%u", inToken->begin.line);

			if (strncmp(buffer, outToken->begin.string, outToken->end.string - outToken->begin.string))
			{
				printf("Error: Unexpected begin line %u, expected %.*s\n",
				       inToken->begin.line, outToken->end.string - outToken->begin.string, outToken->begin.string);
				mgDebugInspectToken(inToken, inTokenizer.filename, MG_FALSE);
				goto fail;
			}

			++outToken;
			_MG_TOKEN_SCAN_LINE(outToken);
		}

		if (outToken->type == MG_TOKEN_COLON)
		{
			++outToken;
			_MG_TOKEN_SCAN_LINE(outToken);

			if (outToken->type == MG_TOKEN_INTEGER)
			{
				snprintf(buffer, _MG_VALUE_BUFFER_LENGTH, "%u", inToken->begin.character);

				if (strncmp(buffer, outToken->begin.string, outToken->end.string - outToken->begin.string))
				{
					printf("Error: Unexpected begin character %u, expected %.*s\n",
					       inToken->begin.character, outToken->end.string - outToken->begin.string, outToken->begin.string);
					mgDebugInspectToken(inToken, inTokenizer.filename, MG_FALSE);
					goto fail;
				}

				++outToken;
				_MG_TOKEN_SCAN_LINE(outToken);
			}
		}

		if (outToken->type == MG_TOKEN_INTEGER)
		{
			snprintf(buffer, _MG_VALUE_BUFFER_LENGTH, "%u", inToken->end.line);

			if (strncmp(buffer, outToken->begin.string, outToken->end.string - outToken->begin.string))
			{
				printf("Error: Unexpected end line %u, expected %.*s\n",
				       inToken->end.line, outToken->end.string - outToken->begin.string, outToken->begin.string);
				mgDebugInspectToken(inToken, inTokenizer.filename, MG_FALSE);
				goto fail;
			}

			++outToken;
			_MG_TOKEN_SCAN_LINE(outToken);
		}

		if (outToken->type == MG_TOKEN_COLON)
		{
			++outToken;
			_MG_TOKEN_SCAN_LINE(outToken);

			if (outToken->type == MG_TOKEN_INTEGER)
			{
				snprintf(buffer, _MG_VALUE_BUFFER_LENGTH, "%u", inToken->end.character);

				if (strncmp(buffer, outToken->begin.string, outToken->end.string - outToken->begin.string))
				{
					printf("Error: Unexpected end character %u, expected %.*s\n",
					       inToken->end.character, outToken->end.string - outToken->begin.string, outToken->begin.string);
					mgDebugInspectToken(inToken, inTokenizer.filename, MG_FALSE);
					goto fail;
				}

				++outToken;
			}
		}

		if (inToken->type == MG_TOKEN_EOF)
			break;

		for (; (outToken->type != MG_TOKEN_NEWLINE) && (outToken->type != MG_TOKEN_EOF); ++outToken);
	}

	_MG_FIND_NEXT_TESTABLE_TOKEN(outToken);

	if (outToken->type != MG_TOKEN_EOF)
	{
		printf("Error: Expected token of type...\n");
		mgDebugInspectToken(outToken, outTokenizer.filename, MG_FALSE);
		goto fail;
	}

	goto pass;

fail:

	passed = MG_FALSE;
	++_mgTestsFailed;

pass:

	mgDestroyTokenizer(&inTokenizer);
	mgDestroyTokenizer(&outTokenizer);

	return passed;
}


static void _mgTokenizerTest(const MGUnitTest *test)
{
	const char *in = ((char**) test->data)[0];
	const char *out = ((char**) test->data)[1];

	_mgTestTokenizer(in, out);
}


static void mgRunTokenizerTest(const char *in)
{
	char out[MAX_PATH + 1];
	const char *files[2] = { in, out };

	if (!mgStringEndsWith(in, ".mg"))
		return;

	MGUnitTest test;
	test.name = out;
	test.func = _mgTokenizerTest;
	test.data = (void*) files;

	strcpy(out, in);
	strcpy(strrchr(out, '.'), ".tokens");

	if (mgFileExists(out))
		mgRunUnitTest(&test);
}


static inline void mgRunTokenizerTests(void)
{
	mgWalkFiles("tests/fixtures/", mgRunTokenizerTest);
}

#endif