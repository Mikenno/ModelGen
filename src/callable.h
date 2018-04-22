#ifndef MODELGEN_CALLABLE_H
#define MODELGEN_CALLABLE_H

#include "modelgen.h"
#include "module.h"

#define mgIsCallable(value) ((value->type == MG_VALUE_CFUNCTION) || (value->type == MG_VALUE_PROCEDURE) || (value->type == MG_VALUE_FUNCTION))

#define mgGetCalleeName(instance) (instance->callStackTop->callerName ? (const char*)instance->callStackTop->callerName : "<anonymous>")
#define mgGetCallerName(instance) (instance->callStackTop->last->callerName ? (const char*)instance->callStackTop->last->callerName : "<anonymous>")

MGValue* mgCall(MGInstance *instance, const MGValue *callable, size_t argc, const MGValue* const* argv);
MGValue* mgCallEx(MGInstance *instance, MGStackFrame *frame, const MGValue *callable, size_t argc, const MGValue* const* argv);

#endif