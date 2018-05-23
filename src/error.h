#ifndef MODELGEN_ERROR_H
#define MODELGEN_ERROR_H

#include "instance.h"
#include "error.h"
#include "debug.h"

extern MGInstance *_mgLastInstance;

void mgTraceback(const MGInstance *instance);

void _mgFatalError(const char *file, int line, const MGInstance *instance, const char *format, ...);

#define mgFatalError(...) _mgFatalError(__FILE__, __LINE__, _mgLastInstance, __VA_ARGS__)
#define mgFatalErrorEx(instance, ...) _mgFatalError(__FILE__, __LINE__, instance, __VA_ARGS__)

#endif