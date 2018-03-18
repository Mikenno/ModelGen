#ifndef MODELGEN_UTILITIES_H
#define MODELGEN_UTILITIES_H

#include <math.h>

#define _MG_INT_COUNT_DIGITS(x) ((int) floorf(log10f((float) (x))) + 1)

const char* mgBasename(const char *filename);

unsigned int mgInlineRepresentationLength(const char *str, const char *end);
char* mgInlineRepresentation(char *destination, const char *source, const char *end);

#endif