
#include "test.h"
#include "tokenize.h"


int main(int argc, char *argv[])
{
	mgTestingBegin();
	mgRunTokenizerTests();
	mgTestingEnd();

	return _mgTestsFailed ? EXIT_FAILURE : EXIT_SUCCESS;
}
