#ifndef MODELGEN_ERROR_H
#define MODELGEN_ERROR_H

void _mgFatalError(const char *file, int line, const char *format, ...);

#define mgFatalError(...) _mgFatalError(__FILE__, __LINE__, __VA_ARGS__)

#endif