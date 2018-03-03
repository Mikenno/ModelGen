#ifndef MODELGEN_TEST_H
#define MODELGEN_TEST_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>


#define _MG_TEST_STRINGIZE_(x) #x
#define _MG_TEST_STRINGIZE(x) _MG_TEST_STRINGIZE_(x)

#define _MG_TEST_CONCATENATE_(x, y) x##y
#define _MG_TEST_CONCATENATE(x, y) _MG_TEST_CONCATENATE_(x, y)


typedef struct MGUnitTest {
	const char *name;
	void (*func)(const struct MGUnitTest *test);
} MGUnitTest;


#define MG_TEST_NAMED(func, name) \
	void _MG_TEST_CONCATENATE(_, func)(const MGUnitTest *test); \
	MGUnitTest func = { name, _MG_TEST_CONCATENATE(_, func) }; \
	void _MG_TEST_CONCATENATE(_, func)(const MGUnitTest *test)

#define MG_TEST(func) MG_TEST_NAMED(func, _MG_TEST_STRINGIZE(func))

#define MG_SKIP_TEST_NAMED(func, name) MG_TEST_NAMED(func, "_" name)
#define MG_SKIP_TEST(func) MG_SKIP_TEST_NAMED(func, _MG_TEST_STRINGIZE(func))


static unsigned int _mgTestsRun = 0;
static unsigned int _mgFailedTests = 0;


static void runUnitTests(const MGUnitTest **tests)
{
	clock_t startTime, endTime;
	float benchSeconds;

	unsigned int failedTests = _mgFailedTests;
	unsigned int testSkip = 0;
	const MGUnitTest *test;

	startTime = clock();

	while ((test = *tests))
	{
		testSkip = test->name[0] == '_';

		if (test->name && (test->name[0] == '_'))
			fputs("[SKIP] ", stdout);
		else
		{
			test->func(test);

			if (_mgFailedTests == failedTests)
				fputs("[PASS] ", stdout);
			else
				fputs("[FAIL] ", stdout);
		}

		puts(test->name + testSkip);
		fflush(stdout);

		failedTests = _mgFailedTests;

		++tests;
		++_mgTestsRun;
	}

	endTime = clock();
	benchSeconds = (float) (endTime - startTime) / (float) CLOCKS_PER_SEC;

	if (benchSeconds >= 5.0f)
		printf("Ran %d tests in %.2fs\n", _mgTestsRun, (float) (endTime - startTime) / (float) CLOCKS_PER_SEC);
	else
		printf("Ran %d tests in %.4fms\n", _mgTestsRun, (float) (endTime - startTime) / (float) CLOCKS_PER_SEC * 1000.0f);
}


#define _mgTestFail() \
	do { \
		++_mgFailedTests; \
		return;\
	} while (0)


#define mgTestFail(reason) \
	do { \
		printf("%s:%d: Failed: %s\n", __FILE__, __LINE__, reason); \
		_mgTestFail(); \
	} while (0)


#define mgTestAssert(expression) \
	do { \
		if (!(expression)) \
			mgTestFail(_MG_TEST_STRINGIZE(expression)); \
	} while (0)


#define _mgTestAssertIntFail(x, y, op) \
	do { \
		printf("%s:%d: Failed: %d %s %d\n", __FILE__, __LINE__, x, op, y); \
		_mgTestFail(); \
	} while (0)


#define mgTestAssertIntEquals(x, y) \
	do { \
		if ((x) != (y)) \
			_mgTestAssertIntFail(x, y, "=="); \
	} while (0)


#define mgTestAssertIntNotEquals(x, y) \
	do { \
		if ((x) == (y)) \
			_mgTestAssertIntFail(x, y, "!="); \
	} while (0)


int _mgTestAssertStringEquals(const char *x, const char *y)
{
	if (x == y)
		return 0;
	else if (!x || !y)
		return 1;
	return strcmp(x, y);
}


void _mgTestAssertPrintStringEquals(const char *x, const char *y, const char *op)
{
	if (x)
		printf("\"%s\"", x);
	else
		fputs("NULL", stdout);

	printf(" %s ", op);

	if (y)
		printf("\"%s\"", y);
	else
		fputs("NULL", stdout);
}


#define _mgTestAssertStringFail(x, y, op) \
	do { \
		printf("%s:%d: Failed: ", __FILE__, __LINE__); \
		_mgTestAssertPrintStringEquals(x, y, op); \
		putchar('\n'); \
		_mgTestFail(); \
	} while (0)


#define mgTestAssertStringEquals(x, y) \
	do { \
		if (_mgTestAssertStringEquals(x, y)) \
			_mgTestAssertStringFail(x, y, "=="); \
	} while (0)


#define mgTestAssertStringNotEquals(x, y) \
	do { \
		if (!_mgTestAssertStringEquals(x, y)) \
			_mgTestAssertStringFail(x, y, "!="); \
	} while (0)

#endif