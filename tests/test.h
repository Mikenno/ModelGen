#ifndef MODELGEN_TEST_TEST_H
#define MODELGEN_TEST_TEST_H

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
	void *data;
} MGUnitTest;


#define MG_TEST_NAMED(func, name) \
	void _MG_TEST_CONCATENATE(_, func)(const MGUnitTest *test); \
	MGUnitTest func = { name, _MG_TEST_CONCATENATE(_, func), NULL }; \
	void _MG_TEST_CONCATENATE(_, func)(const MGUnitTest *test)

#define MG_TEST(func) MG_TEST_NAMED(func, _MG_TEST_STRINGIZE(func))

#define MG_SKIP_TEST_NAMED(func, name) MG_TEST_NAMED(func, "_" name)
#define MG_SKIP_TEST(func) MG_SKIP_TEST_NAMED(func, _MG_TEST_STRINGIZE(func))


static unsigned int _mgTestsRun = 0;

static unsigned int _mgTestsPassed = 0;
static unsigned int _mgTestsFailed = 0;
static unsigned int _mgTestsSkipped = 0;

static clock_t _mgTestTimeBegin;
static clock_t _mgTestTimeEnd;


static void mgTestingBegin(void)
{
	_mgTestsRun = 0;

	_mgTestsPassed = 0;
	_mgTestsFailed = 0;
	_mgTestsSkipped = 0;

	_mgTestTimeBegin = clock();
}


static void mgTestingEnd(void)
{
	_mgTestTimeEnd = clock();

	const double durationSeconds = (double) (_mgTestTimeEnd - _mgTestTimeBegin) / (double) CLOCKS_PER_SEC;

	const char *pluralize = (_mgTestsRun == 1) ? "test" : "tests";

	if (durationSeconds >= 5.0f)
		printf("Ran %d %s in %.2fs", _mgTestsRun, pluralize, durationSeconds);
	else
		printf("Ran %d %s in %.4fms", _mgTestsRun, pluralize, durationSeconds * 1000.0);

	printf(" (%d passed, %d failed, %d skipped)\n", _mgTestsPassed, _mgTestsFailed, _mgTestsSkipped);
}


static void mgRunUnitTest(const MGUnitTest *test)
{
	unsigned int failedTests = _mgTestsFailed;
	unsigned int testSkip = test->name[0] == '_';

	if (test->name && (test->name[0] == '_'))
	{
		++_mgTestsSkipped;

#if MG_ANSI_COLORS
		fputs("\e[33m[SKIP]\e[0m ", stdout);
#else
		fputs("[SKIP] ", stdout);
#endif
	}
	else
	{
		test->func(test);

		if (_mgTestsFailed == failedTests)
		{
			++_mgTestsPassed;

#if MG_ANSI_COLORS
			fputs("\e[32m[PASS]\e[0m ", stdout);
#else
			fputs("[PASS] ", stdout);
#endif
		}
		else
		{
#if MG_ANSI_COLORS
			fputs("\e[31m[FAIL]\e[0m ", stdout);
#else
			fputs("[FAIL] ", stdout);
#endif
		}
	}

	puts(test->name + testSkip);
	fflush(stdout);

	++_mgTestsRun;
}


static void mgRunUnitTests(const MGUnitTest **tests)
{
	for (; *tests; ++tests)
		mgRunUnitTest(*tests);
}


#define _mgTestFail() \
	do { \
		++_mgTestsFailed; \
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