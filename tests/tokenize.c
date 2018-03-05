
#include "../src/modelgen.h"

#include "test.h"


MG_TEST(testTokenTypes)
{
	MGTokenizer tokenizer;

	const char *str = "a b abc";

	mgCreateTokenizer(&tokenizer);

	mgTestAssert(mgTokenizeString(&tokenizer, str, NULL));
	mgTestAssertIntEquals(tokenizer.tokenCount, 6);

	mgTestAssertIntEquals(tokenizer.tokens[0].type, MG_TOKEN_IDENTIFIER);
	mgTestAssertIntEquals(tokenizer.tokens[1].type, MG_TOKEN_WHITESPACE);
	mgTestAssertIntEquals(tokenizer.tokens[2].type, MG_TOKEN_IDENTIFIER);
	mgTestAssertIntEquals(tokenizer.tokens[3].type, MG_TOKEN_WHITESPACE);
	mgTestAssertIntEquals(tokenizer.tokens[4].type, MG_TOKEN_IDENTIFIER);
	mgTestAssertIntEquals(tokenizer.tokens[5].type, MG_TOKEN_EOF);

	mgDestroyTokenizer(&tokenizer);
}


MG_TEST(testTokenStrings)
{
	MGTokenizer tokenizer;
	size_t i;

	const char *str = "a b abc";
	const char *tokenStrings[] = { "a", " ", "b", " ",  "abc", "" };

	mgCreateTokenizer(&tokenizer);

	mgTestAssert(mgTokenizeString(&tokenizer, str, NULL));
	mgTestAssertIntEquals(tokenizer.tokenCount, 6);

	for (i = 0; i < tokenizer.tokenCount; ++i)
	{
		mgTestAssertIntEquals(strlen(tokenStrings[i]), tokenizer.tokens[i].end.string - tokenizer.tokens[i].begin.string);
		mgTestAssert(!strncmp(tokenStrings[i], tokenizer.tokens[i].begin.string, tokenizer.tokens[i].end.string - tokenizer.tokens[i].begin.string));
	}

	mgDestroyTokenizer(&tokenizer);
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
