
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "modelgen.h"
#include "module.h"
#include "inspect.h"
#include "format.h"

#ifdef _WIN32
#   include <stdint.h>
#   include <windows.h>
#endif


void usage(void)
{
	printf(
		"Usage: modelgen [options] [--] [files]\n"
		"\n"
		"    -h, --help        Print this help message and exit\n"
		"    --version         Print ModelGen version and exit\n"
		"    --export=<format> Export model to stdout in the given format\n"
		"    --export <file>   Export model to <file> in the detected format\n"
		"    - --stdin         Read stdin as a file\n"
		"    --tokens          Print tokens and exit\n"
		"    --ast             Print ast and exit\n"
		"\n"
		"Formats:\n"
		"\n"
		"    obj       Wavefront .obj format\n"
		"    triangles Tightly packed triangles 32-bit floats\n"
		"              Format: xyz nxnynz (interleaved vertices)\n"
		"\n"
		"Introspection:\n"
		"\n"
		"    --profile Print elapsed time\n"
		"    --inspect Print modules and their contents on exit\n"
		"\n"
		"Debugging:\n"
		"\n"
		"    --debug-read  Print file contents and exit\n"
	);
}


int main(int argc, char *argv[])
{
	MGbool runStdin = MG_FALSE;
	MGbool debugRead = MG_FALSE;
	MGbool debugTokens = MG_FALSE;
	MGbool debugAST = MG_FALSE;
	MGbool profileTime = MG_FALSE;
	MGbool inspectModules = MG_FALSE;

	MGbool exportOBJ = MG_FALSE;
	MGbool exportTriangles = MG_FALSE;
	const char *exportFilename = NULL;

	MGInstance instance;
	mgCreateInstance(&instance);

	instance.vertexSize.position = 3;
	instance.vertexSize.normal = 3;

	MGValue *base = mgMapGet(instance.staticModules, "base");
	MG_ASSERT(base);
	MG_ASSERT(base->type == MG_TYPE_MODULE);

	int i = 1;
	const char *arg;

	for (; i < argc; ++i)
	{
		arg = argv[i];

		if (!strcmp("--version", arg))
		{
			fputs("ModelGen " MG_VERSION, stdout);

#ifdef MG_DEBUG
			fputs(" (Debug Build)", stdout);
#endif

#if defined(__i386__)
			fputs(" [32-bit]", stdout);
#elif defined(__x86_64__)
			fputs(" [64-bit]", stdout);
#endif

			putc('\n', stdout);

			return EXIT_SUCCESS;
		}
		else if (!strcmp("-h", arg) || !strcmp("--help", arg))
		{
			usage();
			return EXIT_SUCCESS;
		}
		else if (!strcmp("-", arg) || !strcmp("--stdin", arg))
			runStdin = MG_TRUE;
		else if (!strcmp("--tokens", arg))
			debugTokens = MG_TRUE;
		else if (!strcmp("--ast", arg))
			debugAST = MG_TRUE;
		else if (!strcmp("--debug-read", arg))
			debugRead = MG_TRUE;
		else if (!strcmp("--export", arg) || !strncmp("--export=", arg, 9))
		{
			exportFilename = NULL;

			const char *format = NULL;

			if (!strcmp("--export", arg))
			{
				if (i >= (argc - 1))
				{
					fputs("Error: Missing filename after --export", stderr);
					return EXIT_FAILURE;
				}

				exportFilename = argv[++i];
				format = strrchr(exportFilename, '.');

				if (format == NULL)
				{
					fprintf(stderr, "Error: Missing file extension \"%s\"\n", exportFilename);
					return EXIT_FAILURE;
				}

				++format;
			}
			else
				format = arg + 9;

			if (!strcmp(format, "obj"))
				exportOBJ = MG_TRUE;
			else if (!strcmp(format, "triangles"))
				exportTriangles = MG_TRUE;
			else
			{
				fprintf(stderr, "Error: Unknown format \"%s\"\n", format);
				return EXIT_FAILURE;
			}
		}
		else if (!strcmp("--profile", arg))
			profileTime = MG_TRUE;
		else if (!strcmp("--inspect", arg))
			inspectModules = MG_TRUE;
		else if (!strcmp("--set", arg))
		{
			if (i >= (argc - 1))
			{
				fputs("Error: Missing name after --set", stderr);
				return EXIT_FAILURE;
			}

			const char *name = argv[++i];

			if (i >= (argc - 1))
			{
				fprintf(stderr, "Error: Missing value after --set \"%s\"", name);
				return EXIT_FAILURE;
			}

			const char *value = argv[++i];

			mgModuleSet(base, name, mgCreateValueString(value));
		}
		else if (!strcmp("--", arg))
		{
			++i;
			break;
		}
		else if (arg[0] == '-')
		{
			fprintf(stderr, "Unknown option: %s\n", arg);
			return EXIT_FAILURE;
		}
		else
			break;
	}

	int err = EXIT_SUCCESS;

#ifdef _WIN32
	LARGE_INTEGER timeStart, timeStop;
	LARGE_INTEGER timerResolution;

	if (profileTime)
		QueryPerformanceCounter(&timeStart);
#endif

	if (debugRead)
	{
		if (runStdin)
			if (!mgDebugReadHandle(stdin, "<stdin>"))
				err = 1;

		for (; i < argc; ++i)
			if (!mgDebugRead(argv[i]))
				err = 1;
	}
	else if (debugTokens)
	{
		if (runStdin)
			if (!mgDebugTokenizeHandle(stdin, "<stdin>"))
				err = 1;

		for (; i < argc; ++i)
			if (!mgDebugTokenize(argv[i]))
				err = 1;
	}
	else if (debugAST)
	{
		MGParser parser;
		MGNode *root;

		if (runStdin)
		{
			mgCreateParser(&parser);

			if ((root = mgParseFileHandle(&parser, stdin)))
				mgInspectNode(root);
			else
				err = 1;

			mgDestroyParser(&parser);
		}

		for (; i < argc; ++i)
		{
			const char *filename = argv[i];

			mgCreateParser(&parser);

			if ((root = mgParseFile(&parser, filename)))
				mgInspectNode(root);
			else
				err = 1;

			mgDestroyParser(&parser);
		}
	}
	else
	{
		if (runStdin)
			mgRunFileHandle(&instance, stdin, "<stdin>");

		for (; i < argc; ++i)
			mgRunFile(&instance, argv[i], NULL);

		if (inspectModules)
		{
			putchar('\n');
			mgInspectInstance(&instance);
		}

		if (exportFilename)
		{
			FILE *f = fopen(exportFilename, exportOBJ ? "w" : "wb");

			if (f)
			{
				if (exportOBJ)
					mgExportOBJ(&instance, f);
				else if (exportTriangles)
					mgExportTriangles(&instance, f);

				fclose(f);
			}
			else
			{
				fprintf(stderr, "Error: Failed opening file \"%s\"\n", exportFilename);
			}
		}
		else
		{
			if (exportOBJ)
				mgExportOBJ(&instance, stdout);
			else if (exportTriangles)
				mgExportTriangles(&instance, stdout);
		}
	}

#ifdef _WIN32
	if (profileTime)
	{
		QueryPerformanceCounter(&timeStop);
		QueryPerformanceFrequency(&timerResolution);

		const int64_t timeInterval = timeStop.QuadPart - timeStart.QuadPart;
		const double timeSeconds = (double) timeInterval / (double) timerResolution.QuadPart;

#if MG_ANSI_COLORS
		fputs("\e[90m", stdout);
#endif

		putchar('\n');
		printf("Time Elapsed: %.6fms\n", timeSeconds * 1000.0);

#if MG_ANSI_COLORS
		fputs("\e[0m", stdout);
#endif
	}
#endif

	mgDestroyInstance(&instance);

	return err;
}
