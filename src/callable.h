#ifndef MODELGEN_CALLABLE_H
#define MODELGEN_CALLABLE_H

#include "modelgen.h"
#include "value.h"

#define mgIsCallable(value) ((value->type == MG_TYPE_CFUNCTION) || (value->type == MG_TYPE_PROCEDURE) || (value->type == MG_TYPE_FUNCTION))

#define mgGetCalleeName(instance) (instance->callStackTop->callerName ? (const char*) instance->callStackTop->callerName : "<anonymous>")
#define mgGetCallerName(instance) (instance->callStackTop->last->callerName ? (const char*) instance->callStackTop->last->callerName : "<anonymous>")

MGValue* mgCall(MGInstance *instance, const MGValue *callable, size_t argc, const MGValue* const* argv);
MGValue* mgCallEx(MGInstance *instance, MGStackFrame *frame, const MGValue *callable, size_t argc, const MGValue* const* argv);

void mgCheckArgumentCount(MGInstance *instance, size_t argc, size_t min, size_t max);
void mgCheckArgumentTypes(MGInstance *instance, size_t argc, const MGValue* const* argv, ...);

#endif