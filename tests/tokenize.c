
#include <windows.h>

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
	{
		printf("Error: Failed tokenizing \"%s\"\n", in);
		goto fail;
	}
	else if (!mgTokenizeFile(&outTokenizer, out, NULL))
	{
		printf("Error: Failed tokenizing \"%s\"\n", out);
		goto fail;
	}

#define _MG_IS_TESTABLE_TOKEN(token) \
		((token->type != MG_TOKEN_NEWLINE) && (token->type != MG_TOKEN_WHITESPACE))

#define _MG_FIND_NEXT_TESTABLE_TOKEN(token) \
		for (; !_MG_IS_TESTABLE_TOKEN(token); ++token)

	outToken = outTokenizer.tokens;

	for (inToken = inTokenizer.tokens;; ++inToken)
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

		do
			++outToken;
		while (outToken->type == MG_TOKEN_WHITESPACE);

		if (_MG_IS_TESTABLE_TOKEN(outToken) && (outToken->type != MG_TOKEN_EOF))
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
		}

		if (inToken->type == MG_TOKEN_EOF)
			break;
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


static void _mgRunTokenizerTest(const char *in)
{
	char out[MAX_PATH + 1];
	const char *files[2] = { in, out };

	MGUnitTest test;
	test.name = in;
	test.func = _mgTokenizerTest;
	test.data = (void*) files;

	strcpy(out, in);
	strcpy(strrchr(out, '.'), ".tokens");

	mgRunUnitTest(&test);
}


static void _mgRunTokenizerTests(const char *directory)
{
	char dir[MAX_PATH + 1], filename[MAX_PATH + 1], *extension;
	WIN32_FIND_DATAA find;
	HANDLE hFind;

	strcpy(dir, directory);
	strcat(dir, "/*");

	if ((hFind = FindFirstFileA(dir, &find)) == INVALID_HANDLE_VALUE)
	{
		printf("No such directory \"%s\"\n", directory);
		++_mgTestsFailed;
		return;
	}

	do
	{
		if (!strcmp(find.cFileName, ".") || !strcmp(find.cFileName, ".."))
			continue;

		strcpy(filename, directory);
		strcat(filename, "/");
		strcat(filename, find.cFileName);

		if (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			_mgRunTokenizerTests(filename);
		else
		{
			extension = strrchr(filename, '.');

			if (extension && !strcmp(extension, ".mg"))
				_mgRunTokenizerTest(filename);
		}
	}
	while (FindNextFileA(hFind, &find) != 0);

	FindClose(hFind);
}


int main(int argc, char *argv[])
{
	mgTestingBegin();
	_mgRunTokenizerTests("tests/tokenize");
	mgTestingEnd();

	return _mgTestsFailed ? EXIT_FAILURE : EXIT_SUCCESS;
}
