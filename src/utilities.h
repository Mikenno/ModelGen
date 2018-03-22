#ifndef MODELGEN_UTILITIES_H
#define MODELGEN_UTILITIES_H

#include <stddef.h>
#include <math.h>

#define _MG_INT_COUNT_DIGITS(x) ((int) floorf(log10f((float) (x))) + 1)

int mgStringEndsWith(const char *string, const char *suffix);

char* mgDuplicateString(const char *str);
char* mgDuplicateFixedString(const char *str, size_t count);

const char* mgBasename(const char *filename);

size_t mgDirnameEnd(const char *filename);
char* mgDirname(char *dirname, const char *filename);

unsigned int mgInlineRepresentationLength(const char *str, const char *end);
char* mgInlineRepresentation(char *destination, const char *source, const char *end);

#endif