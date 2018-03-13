
#include "test.h"
#include "tokenize.h"
#include "parse.h"


int main(int argc, char *argv[])
{
	mgTestingBegin();
	mgRunTokenizerTests();
	mgRunParserTests();
	mgTestingEnd();

	return _mgTestsFailed ? EXIT_FAILURE : EXIT_SUCCESS;
}
