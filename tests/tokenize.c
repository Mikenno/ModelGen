
#include "../src/modelgen.h"
#include "../src/utilities.h"
#include "../src/debug.h"

#include "test.h"


static MGbool _mgTestTokenizer(const char *in, const char *out)
{
	MGTokenizer inTokenizer, outTokenizer;
	MGToken *inToken, *outToken;

	MGbool passed = MG_TRUE;

	mgCreateTokenizer(&inTokenizer);
	mgCreateTokenizer(&outTokenizer);

	if (!mgTokenizeFile(&inTokenizer, in, NULL))
		goto fail;
	if (!mgTokenizeFile(&outTokenizer, out, NULL))
		goto fail;

#define _MG_IS_TESTABLE_TOKEN(token) \
		((token->type != MG_TOKEN_NEWLINE) && (token->type != MG_TOKEN_WHITESPACE))

#define _MG_FIND_NEXT_TESTABLE_TOKEN(token) \
		for (; !_MG_IS_TESTABLE_TOKEN(token); ++token)

	outToken = outTokenizer.tokens;

	for (inToken = inTokenizer.tokens; inToken->type != MG_TOKEN_EOF; ++inToken)
	{
		if (!_MG_IS_TESTABLE_TOKEN(inToken))
			continue;

		_MG_FIND_NEXT_TESTABLE_TOKEN(outToken);

		if (outToken->type == MG_TOKEN_EOF)
		{
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
		for (; outToken->type == MG_TOKEN_WHITESPACE; ++outToken);

		if (_MG_IS_TESTABLE_TOKEN(outToken))
		{
			const char *outTokenString = outToken->value.s;
			const size_t outTokenStringLength = outTokenString ? strlen(outTokenString) : 0;

			const char *inTokenString = inToken->begin.string;
			size_t inTokenStringLength = inToken->end.string - inToken->begin.string;

			if (inToken->type == MG_TOKEN_STRING)
			{
				inTokenString = inToken->value.s ? inToken->value.s : NULL;
				inTokenStringLength -= 2;
			}

			if ((outTokenStringLength != inTokenStringLength) || ((outTokenStringLength > 0) && strncmp(outTokenString, inTokenString, outTokenStringLength)))
			{
				printf("Error: Unexpected token value...\n");
				mgDebugInspectToken(inToken, inTokenizer.filename, MG_FALSE);
				printf("Expected...\n");
				mgDebugInspectToken(outToken, outTokenizer.filename, MG_FALSE);
				goto fail;
			}
		}

		++outToken;
	}

	_MG_FIND_NEXT_TESTABLE_TOKEN(outToken);

	if (outToken->type != MG_TOKEN_EOF)
	{
		printf("Error: Expected token of type...\n");
		mgDebugInspectToken(outToken, outTokenizer.filename, MG_FALSE);
		goto fail;
	}

#undef _MG_IS_TESTABLE_TOKEN
#undef _MG_FIND_NEXT_TESTABLE_TOKEN

	goto pass;

fail:

	passed = MG_FALSE;

pass:

	mgDestroyTokenizer(&inTokenizer);
	mgDestroyTokenizer(&outTokenizer);

	return passed;
}


MG_TEST(testEmpty)
{
	mgTestAssert(_mgTestTokenizer("tests/tokenize/empty.mg", "tests/tokenize/empty.tokens"));
}


MG_TEST(testComments)
{
	mgTestAssert(_mgTestTokenizer("tests/tokenize/comments.mg", "tests/tokenize/comments.tokens"));
}


MG_TEST(testIdentifiers)
{
	mgTestAssert(_mgTestTokenizer("tests/tokenize/identifiers.mg", "tests/tokenize/identifiers.tokens"));
}


MG_TEST(testNumbers)
{
	mgTestAssert(_mgTestTokenizer("tests/tokenize/comments.mg", "tests/tokenize/comments.tokens"));
}


int main(int argc, char *argv[])
{
	const MGUnitTest *tests[] = {
			&testEmpty,
			&testComments,
			&testIdentifiers,
			&testNumbers,
			NULL
	};

	runUnitTests(tests);

	return _mgFailedTests > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
