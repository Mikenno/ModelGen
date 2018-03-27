#ifndef MODELGEN_TEST_INTERPRET_H
#define MODELGEN_TEST_INTERPRET_H

#include "../src/inspect.h"


#define _MG_OUTPUT_FILENAME "tests/_test.out"
#define _MG_ERROR_FILENAME "tests/_test.err"


static void _mgRun(const char *in)
{
#define _MG_COMMAND_FORMAT "modelgen \"%s\" > \"" _MG_OUTPUT_FILENAME "\" 2> \"" _MG_ERROR_FILENAME "\"", in

	size_t len = (size_t) snprintf(NULL, 0, _MG_COMMAND_FORMAT);
	MG_ASSERT(len >= 0);
	char *command = (char*) malloc((len + 1) * sizeof(char));
	snprintf(command, len + 1, _MG_COMMAND_FORMAT);
	command[len] = '\0';

#undef _MG_COMMAND_FORMAT

	system(command);

	free(command);
}


static void _mgInterpreterTest(const MGTestCase *test)
{
	const char *in = ((char**) test->data)[0];
	const char *out = ((char**) test->data)[1];
	const char *err = ((char**) test->data)[2];

	char *expectedOutput = NULL;
	char *expectedError = NULL;

	char *actualOutput = NULL;
	char *actualError = NULL;

	_mgRun(in);

	if (mgFileExists(out))
	{
		if (!(expectedOutput = mgReadFile(out, NULL)))
		{
			printf("Error: Failed reading \"%s\"\n", out);
			goto fail;
		}
	}
	else
	{
		expectedOutput = (char*) malloc(1 * sizeof(char));
		expectedOutput[0] = '\0';
	}

	if (mgFileExists(err))
	{
		if (!(expectedError = mgReadFile(err, NULL)))
		{
			printf("Error: Failed reading \"%s\"\n", err);
			goto fail;
		}
	}
	else
	{
		expectedError = (char*) malloc(1 * sizeof(char));
		expectedError[0] = '\0';
	}

	if (!mgFileExists(_MG_OUTPUT_FILENAME) || !(actualOutput = mgReadFile(_MG_OUTPUT_FILENAME, NULL)))
	{
		printf("Error: Failed reading \"%s\"\n", _MG_OUTPUT_FILENAME);
		goto fail;
	}

	if (!mgFileExists(_MG_ERROR_FILENAME) || !(actualError = mgReadFile(_MG_ERROR_FILENAME, NULL)))
	{
		printf("Error: Failed reading \"%s\"\n", _MG_ERROR_FILENAME);
		goto fail;
	}

	int outputMatch = strcmp(expectedOutput, actualOutput);
	int errorMatch = strcmp(expectedError, actualError);

	if (outputMatch)
	{
		printf("Expected Output:\n");
		mgInspectStringLines(expectedOutput);
		putchar('\n');

		printf("Actual Output:\n");
		mgInspectStringLines(actualOutput);
	}

	if (errorMatch)
	{
		if (outputMatch)
			putchar('\n');

		printf("Expected Error:\n");
		mgInspectStringLines(expectedError);
		putchar('\n');

		printf("Actual Error:\n");
		mgInspectStringLines(actualError);
	}

	if (outputMatch || errorMatch)
		goto fail;

	goto pass;

fail:

	++_mgTestsFailed;

pass:

	free(expectedOutput);
	free(expectedError);

	free(actualOutput);
	free(actualError);
}


static void mgRunInterpreterTest(const char *in)
{
	char out[MAX_PATH + 1];
	char err[MAX_PATH + 1];
	const char *files[3] = { in, out, err };

	if (!mgStringEndsWith(in, ".mg"))
		return;

	strcpy(out, in);
	strcpy(strrchr(out, '.'), ".out");
	strcpy(err, in);
	strcpy(strrchr(err, '.'), ".err");

	MGTestCase test;
	test.name = out;

#if _WIN32
	test.skip = (MGbool) (mgBasename(out)[0] == '_');
#else
	test.skip = MG_TRUE;
#endif

	test.func = _mgInterpreterTest;
	test.data = (void*) files;

	if (mgFileExists(out) || mgFileExists(err))
		mgRunTestCase(&test);
}


static inline void mgRunInterpreterTests(void)
{
	mgWalkFiles("tests/fixtures/", mgRunInterpreterTest);

#if _WIN32
	DeleteFileA(_MG_OUTPUT_FILENAME);
	DeleteFileA(_MG_ERROR_FILENAME);
#endif
}

#endif