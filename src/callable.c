
#include <stdarg.h>

#include "callable.h"
#include "utilities.h"


static inline void _mgFail(const char *file, int line, const char *format, ...)
{
	fflush(stdout);

	fprintf(stderr, "%s:%d: ", file, line);

	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);

	putc('\n', stderr);
	fflush(stderr);

	exit(1);
}

#define MG_FAIL(...) _mgFail(__FILE__, __LINE__, __VA_ARGS__)


extern MGValue* _mgVisitNode(MGValue *module, MGNode *node);

extern void _mgSetLocalValue(MGValue *module, const char *name, MGValue *value);


static const MGValue* _mgLastModule(const MGInstance *instance)
{
	MG_ASSERT(instance);

	MGValue *module = NULL;
	for (const MGStackFrame *frame = instance->callStackTop;
		(module == NULL) && frame;
		module = frame->module, frame = frame->last);
	MG_ASSERT(module);

	return module;
}


MGValue* mgCall(MGInstance *instance, const MGValue *callable, size_t argc, const MGValue* const* argv)
{
	MGValue *module = (MGValue*) _mgLastModule(instance);
	MG_ASSERT(module);

	MGStackFrame frame;

	if ((callable->type != MG_VALUE_CFUNCTION) && callable->data.func.locals)
		mgCreateStackFrameEx(&frame, mgReferenceValue(module), mgReferenceValue(callable->data.func.locals));
	else
		mgCreateStackFrame(&frame, mgReferenceValue(module));

	mgPushStackFrame(instance, &frame);

	MGValue *value = mgCallEx(instance, &frame, callable, argc, argv);

	mgPopStackFrame(instance, &frame);
	mgDestroyStackFrame(&frame);

	return value;
}


MGValue* mgCallEx(MGInstance *instance, MGStackFrame *frame, const MGValue *callable, size_t argc, const MGValue* const* argv)
{
	MG_ASSERT(instance);
	MG_ASSERT(callable);
	MG_ASSERT((argc == 0) || (argv != NULL));

	if (!mgIsCallable(callable))
		MG_FAIL("Error: \"%s\" is not callable", _MG_VALUE_TYPE_NAMES[callable->type]);

	MGValue *module = (MGValue*) _mgLastModule(instance);
	MG_ASSERT(module);

	if (callable->type == MG_VALUE_CFUNCTION)
		frame->value = callable->data.cfunc(module->data.module.instance, argc, argv);
	else
	{
		MG_ASSERT(callable->data.func.module);
		MG_ASSERT(callable->data.func.node);

		MGNode *callableNode = callable->data.func.node;
		MG_ASSERT((callableNode->type == MG_NODE_PROCEDURE) || (callableNode->type == MG_NODE_FUNCTION));

		if ((callableNode->type == MG_NODE_PROCEDURE) || (callableNode->type == MG_NODE_FUNCTION))
		{
			MG_ASSERT((_mgListLength(callableNode->children) == 2) || (_mgListLength(callableNode->children) == 3));

			MGNode *funcParametersNode = _mgListGet(callableNode->children, 1);
			MG_ASSERT(funcParametersNode->type == MG_NODE_TUPLE);

			if (_mgListLength(funcParametersNode->children) < argc)
			{
				MGNode *funcNameNode = _mgListGet(callableNode->children, 0);
				char *funcName = NULL;

				if (funcNameNode->type == MG_NODE_NAME)
				{
					MG_ASSERT(funcNameNode->token);

					const size_t funcNameLength = funcNameNode->token->end.string - funcNameNode->token->begin.string;
					MG_ASSERT(funcNameLength > 0);
					funcName = mgStringDuplicateFixed(funcNameNode->token->begin.string, funcNameLength);
					MG_ASSERT(funcName);
				}

				MG_FAIL("Error: %s expected at most %zu arguments, received %zu", (funcName ? funcName : "<anonymous>"), _mgListLength(funcParametersNode->children), argc);

				free(funcName);
			}

			for (size_t i = 0; i < _mgListLength(funcParametersNode->children); ++i)
			{
				MGNode *funcParameterNode = _mgListGet(funcParametersNode->children, i);
				MG_ASSERT((funcParameterNode->type == MG_NODE_NAME) || (funcParameterNode->type == MG_NODE_ASSIGN));

				char *funcParameterName = NULL;

				if (funcParameterNode->type == MG_NODE_NAME)
				{
					const size_t funcParameterNameLength = funcParameterNode->token->end.string - funcParameterNode->token->begin.string;
					MG_ASSERT(funcParameterNameLength > 0);
					funcParameterName = mgStringDuplicateFixed(funcParameterNode->token->begin.string, funcParameterNameLength);
				}
				else if (funcParameterNode->type == MG_NODE_ASSIGN)
				{
					MG_ASSERT(_mgListLength(funcParameterNode->children) == 2);

					MGNode *funcParameterNameNode = _mgListGet(funcParameterNode->children, 0);
					MG_ASSERT(funcParameterNameNode->type == MG_NODE_NAME);
					MG_ASSERT(funcParameterNameNode->token);

					const size_t funcParameterNameLength = funcParameterNameNode->token->end.string - funcParameterNameNode->token->begin.string;
					MG_ASSERT(funcParameterNameLength > 0);
					funcParameterName = mgStringDuplicateFixed(funcParameterNameNode->token->begin.string, funcParameterNameLength);
				}

				MG_ASSERT(funcParameterName);

				if (i < argc)
					_mgSetLocalValue(module, funcParameterName, mgReferenceValue(argv[i]));
				else
				{
					if (funcParameterNode->type != MG_NODE_ASSIGN)
						MG_FAIL("Error: Expected argument \"%s\"", funcParameterName);

					_mgSetLocalValue(module, funcParameterName, _mgVisitNode(module, _mgListGet(funcParameterNode->children, 1)));
				}

				free(funcParameterName);
			}

			if (_mgListLength(callableNode->children) == 3)
				_mgVisitNode(callable->data.func.module, _mgListGet(callableNode->children, 2));
		}
	}

	return frame->value ? mgReferenceValue(frame->value) : mgCreateValueNull();
}
