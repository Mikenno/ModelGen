#ifndef MODELGEN_TEST_TOKENIZE_H
#define MODELGEN_TEST_TOKENIZE_H

#include "../src/modelgen.h"
#include "../src/utilities.h"
#include "../src/inspect.h"

#include "test.h"


#define _MG_IS_TESTABLE_TOKEN(token) ((token->type != MG_TOKEN_NEWLINE) && (token->type != MG_TOKEN_WHITESPACE))
#define _MG_FIND_NEXT_TESTABLE_TOKEN(token) for (; !_MG_IS_TESTABLE_TOKEN(token); ++token)

#define _MG_TOKEN_SCAN_LINE(token) for (; token->type == MG_TOKEN_WHITESPACE; ++token)

#define _MG_VALUE_BUFFER_LENGTH 32


static void _mgTestTokenizer(const char *in, const char *out)
{
	MGTokenizer inTokenizer, outTokenizer;
	MGToken *inToken, *outToken;

	char *inTokenValueBuffer = NULL;
	char *outTokenValueBuffer = NULL;

	// Warning: This could overflow, but is highly unlikely
	char buffer[_MG_VALUE_BUFFER_LENGTH];

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

	outToken = &_mgListGet(outTokenizer.tokens, 0);

	for (inToken = &_mgListGet(inTokenizer.tokens, 0);; ++inToken)
	{
		if (!_MG_IS_TESTABLE_TOKEN(inToken))
			continue;

		_MG_FIND_NEXT_TESTABLE_TOKEN(outToken);

		if (outToken->type == MG_TOKEN_EOF)
		{
			if (inToken->type == MG_TOKEN_EOF)
				break;

			printf("Error: Unexpected token...\n");
			mgInspectToken(inToken, inTokenizer.filename, MG_FALSE);
			goto fail;
		}
		else if (outToken->type != MG_TOKEN_STRING)
		{
			printf("Error: Token type must be a string\n");
			mgInspectToken(outToken, outTokenizer.filename, MG_FALSE);
			goto fail;
		}

		if (!outToken->value.s || strcmp(_MG_TOKEN_NAMES[inToken->type], outToken->value.s))
		{
			printf("Error: Unexpected token type...\n");
			mgInspectToken(inToken, inTokenizer.filename, MG_FALSE);
			printf("Expected...\n");
			mgInspectToken(outToken, outTokenizer.filename, MG_FALSE);
			goto fail;
		}

		++outToken;
		_MG_TOKEN_SCAN_LINE(outToken);

		if (outToken->type == MG_TOKEN_STRING)
		{
			const size_t inTokenValueLength = inToken->end.string - inToken->begin.string;
			const size_t outTokenValueLength = outToken->end.string - outToken->begin.string;

			inTokenValueBuffer = (char*) realloc(inTokenValueBuffer, (inTokenValueLength + 1) * sizeof(char));
			outTokenValueBuffer = (char*) realloc(outTokenValueBuffer, (outTokenValueLength + 1) * sizeof(char));

			const char *inTokenValue = inTokenValueBuffer;
			const char *outTokenValue = outTokenValueBuffer;

			strncpy(inTokenValueBuffer, inToken->begin.string, inTokenValueLength);
			inTokenValueBuffer[inTokenValueLength] = '\0';

			if ((outToken->type == MG_TOKEN_STRING) && outToken->value.s)
			{
				outTokenValue = outToken->value.s;

				if (inToken->type == MG_TOKEN_STRING)
					if (inTokenValue[0] == '"')
						*strrchr(++inTokenValue, '"') = '\0';
			}
			else
			{
				strncpy(outTokenValueBuffer, outToken->begin.string, outTokenValueLength);
				outTokenValueBuffer[outTokenValueLength] = '\0';

				if (inToken->type != MG_TOKEN_STRING)
					if (outTokenValue[0] == '"')
						*strrchr(++outTokenValue, '"') = '\0';
			}

			if ((inToken->type == MG_TOKEN_STRING) && inToken->value.s)
				inTokenValue = inToken->value.s;

			if (strcmp(inTokenValue, outTokenValue))
			{
				printf("Error: Unexpected token value...\n");
				mgInspectToken(inToken, mgBasename(inTokenizer.filename), MG_FALSE);
				printf("Expected...\n");
				mgInspectToken(outToken, mgBasename(outTokenizer.filename), MG_FALSE);
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
				mgInspectToken(inToken, inTokenizer.filename, MG_FALSE);
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
					mgInspectToken(inToken, inTokenizer.filename, MG_FALSE);
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
				mgInspectToken(inToken, inTokenizer.filename, MG_FALSE);
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
					mgInspectToken(inToken, inTokenizer.filename, MG_FALSE);
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
		mgInspectToken(outToken, outTokenizer.filename, MG_FALSE);
		goto fail;
	}

	goto pass;

fail:

	puts("Token Dump:");

	for (inToken = &_mgListGet(inTokenizer.tokens, 0);; ++inToken)
	{
		if (!_MG_IS_TESTABLE_TOKEN(inToken))
			continue;

		mgInspectToken(inToken, mgBasename(inTokenizer.filename), MG_TRUE);

		if (inToken->type == MG_TOKEN_EOF)
			break;
	}

	++_mgTestsFailed;

pass:

	free(inTokenValueBuffer);
	free(outTokenValueBuffer);

	mgDestroyTokenizer(&inTokenizer);
	mgDestroyTokenizer(&outTokenizer);
}


static void _mgTokenizerTest(const MGTestCase *test)
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

	strcpy(out, in);
	strcpy(strrchr(out, '.'), ".tokens");

	if (!mgFileExists(out))
	{
		mgDirname(out, in);
		strcat(out, "/_");
		strcat(out, mgBasename(in));
		strcpy(strrchr(out, '.'), ".tokens");
	}

	MGTestCase test;
	test.name = out;
	test.skip = mgBasename(out)[0] == '_';
	test.func = _mgTokenizerTest;
	test.data = (void*) files;

	if (mgFileExists(out))
		mgRunTestCase(&test);
}


static inline void mgRunTokenizerTests(void)
{
	mgWalkFiles("tests/fixtures/", mgRunTokenizerTest);
}

#endif