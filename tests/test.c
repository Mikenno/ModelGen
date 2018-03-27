
#include "test.h"
#include "tokenize.h"
#include "parse.h"
#include "interpret.h"


int main(int argc, char *argv[])
{
	mgTestingBegin();
	mgRunTokenizerTests();
	mgRunParserTests();
	mgRunInterpreterTests();
	mgTestingEnd();

	return _mgTestsFailed ? EXIT_FAILURE : EXIT_SUCCESS;
}
