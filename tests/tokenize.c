
#include "../src/modelgen.h"

#include "test.h"


MG_TEST(testTokenTypes)
{
	MGToken *tokens;
	size_t tokenCount;

	const char *str = "a b abc";

	mgTestAssert(tokens = mgTokenizeString(str, &tokenCount));

	mgTestAssertIntEquals(tokenCount, 6);

	mgTestAssertIntEquals(tokens[0].type, MG_TOKEN_IDENTIFIER);
	mgTestAssertIntEquals(tokens[1].type, MG_TOKEN_WHITESPACE);
	mgTestAssertIntEquals(tokens[2].type, MG_TOKEN_IDENTIFIER);
	mgTestAssertIntEquals(tokens[3].type, MG_TOKEN_WHITESPACE);
	mgTestAssertIntEquals(tokens[4].type, MG_TOKEN_IDENTIFIER);
	mgTestAssertIntEquals(tokens[5].type, MG_TOKEN_EOF);

	free(tokens);
}


MG_TEST(testTokenStrings)
{
	MGToken *tokens;
	size_t tokenCount;
	size_t i;

	const char *str = "a b abc";
	const char *tokenStrings[] = { "a", " ", "b", " ",  "abc", "" };

	mgTestAssert(tokens = mgTokenizeString(str, &tokenCount));

	mgTestAssertIntEquals(tokenCount, 6);

	for (i = 0; i < tokenCount; ++i)
	{
		mgTestAssertIntEquals(strlen(tokenStrings[i]), tokens[i].end.string - tokens[i].begin.string);
		mgTestAssert(!strncmp(tokenStrings[i], tokens[i].begin.string, tokens[i].end.string - tokens[i].begin.string));
	}

	free(tokens);
}


int main(int argc, char *argv[])
{
	const MGUnitTest *tests[] = {
			&testTokenTypes,
			&testTokenStrings,
			NULL
	};

	runUnitTests(tests);

	return _mgFailedTests > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
