#ifndef MODELGEN_EVAL_H
#define MODELGEN_EVAL_H

#include "modelgen.h"

#define mgEval(instance, string) mgEvalEx(instance, string, NULL)
MGValue* mgEvalEx(MGInstance *instance, const char *string, const MGValue *locals);

#endif