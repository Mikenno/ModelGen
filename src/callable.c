
#include <stdarg.h>

#include "callable.h"
#include "module.h"
#include "error.h"
#include "utilities.h"


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

	if ((callable->type != MG_TYPE_CFUNCTION) && callable->data.func.locals)
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
		mgFatalError("Error: \"%s\" is not callable", mgGetTypeName(callable->type));

	MGValue *module = (MGValue*) _mgLastModule(instance);
	MG_ASSERT(module);

	if (callable->type == MG_TYPE_CFUNCTION)
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
				const char *funcName = NULL;

				if (funcNameNode->type == MG_NODE_NAME)
				{
					MG_ASSERT(funcNameNode->token);

					funcName = funcNameNode->token->value.s;
				}

				mgFatalError("Error: %s expected at most %zu arguments, received %zu", (funcName ? funcName : "<anonymous>"), _mgListLength(funcParametersNode->children), argc);
			}

			for (size_t i = 0; i < _mgListLength(funcParametersNode->children); ++i)
			{
				MGNode *funcParameterNode = _mgListGet(funcParametersNode->children, i);
				MG_ASSERT((funcParameterNode->type == MG_NODE_NAME) || (funcParameterNode->type == MG_NODE_ASSIGN));

				const char *funcParameterName = NULL;

				if (funcParameterNode->type == MG_NODE_NAME)
				{
					MG_ASSERT(funcParameterNode->token);

					funcParameterName = funcParameterNode->token->value.s;
				}
				else if (funcParameterNode->type == MG_NODE_ASSIGN)
				{
					MG_ASSERT(_mgListLength(funcParameterNode->children) == 2);

					MGNode *funcParameterNameNode = _mgListGet(funcParameterNode->children, 0);
					MG_ASSERT(funcParameterNameNode->type == MG_NODE_NAME);
					MG_ASSERT(funcParameterNameNode->token);

					funcParameterName = funcParameterNameNode->token->value.s;
				}

				MG_ASSERT(funcParameterName);

				if (i < argc)
					_mgSetLocalValue(module, funcParameterName, mgReferenceValue(argv[i]));
				else
				{
					if (funcParameterNode->type != MG_NODE_ASSIGN)
						mgFatalError("Error: Expected argument \"%s\"", funcParameterName);

					_mgSetLocalValue(module, funcParameterName, _mgVisitNode(module, _mgListGet(funcParameterNode->children, 1)));
				}
			}

			if (_mgListLength(callableNode->children) == 3)
				_mgVisitNode(callable->data.func.module, _mgListGet(callableNode->children, 2));
		}
	}

	return frame->value ? mgReferenceValue(frame->value) : mgCreateValueNull();
}


void mgCheckArgumentCount(MGInstance *instance, size_t argc, size_t min, size_t max)
{
	MG_ASSERT(instance);

	if ((min == max) && (argc != min))
		mgFatalError("Error: %s expects exactly %zu argument%s, received %zu", mgGetCalleeName(instance), min, ((min == 1) ? "" : "s"), argc);
	else if (argc < min)
		mgFatalError("Error: %s expected at least %zu argument%s, received %zu", mgGetCalleeName(instance), min, ((min == 1) ? "" : "s"), argc);
	else if (argc > max)
		mgFatalError("Error: %s expected at most %zu argument%s, received %zu", mgGetCalleeName(instance), max, ((max == 1) ? "" : "s"), argc);
}


void mgCheckArgumentTypes(MGInstance *instance, size_t argc, const MGValue* const* argv, ...)
{
	MG_ASSERT(instance);

	va_list args, args2;
	va_start(args, argv);

	for (size_t i = 0; i < argc; ++i)
	{
		int types = va_arg(args, int);

		va_copy(args2, args);

		MGbool match = (MGbool) (types == 0);

		for (int j = 0; j < types; ++j)
			if (argv[i]->type == va_arg(args, MGType))
				match = MG_TRUE;

		if (!match)
		{
			size_t messageLength = 100 + (size_t) types * (_MG_LONGEST_TYPE_NAME_LENGTH + 10);
			char *message = (char*) malloc(messageLength * sizeof(char*));
			char *end = message;

			end += snprintf(end, messageLength - (end - message), "Error: %s expected argument %zu as", mgGetCalleeName(instance), i + 1);

			for (int j = 0; j < types; ++j)
				end += snprintf(end, messageLength - (end - message), "%s \"%s\"", (j > 0) ? " or" : "", mgGetTypeName(va_arg(args2, MGType)));

			end += snprintf(end, messageLength - (end - message), ", received \"%s\"", mgGetTypeName(argv[i]->type));

			message[messageLength - 1] = '\0';
			*end = '\0';

			mgFatalError("%s", message);

			free(message);
		}

		va_end(args2);
	}

	va_end(args);
}
