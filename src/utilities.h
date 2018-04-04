#ifndef MODELGEN_UTILITIES_H
#define MODELGEN_UTILITIES_H

#include <stddef.h>
#include <stdint.h>
#include <math.h>

#define _MG_INT_COUNT_DIGITS(x) ((int) floorf(log10f((float) (x))) + 1)

#define _MG_EPSILON 1E-6f
#define _MG_FEQUAL(x, y) ((((y) - _MG_EPSILON) < (x)) && ((x) < ((y) + _MG_EPSILON)))

uint32_t mgNextPowerOfTwo(uint32_t x);

int mgStringEndsWith(const char *string, const char *suffix);

char* mgStringReplaceCharacter(char *str, char find, char replace);

char* mgStringDuplicate(const char *str);
char* mgStringDuplicateFixed(const char *str, size_t count);

char* mgStringRepeat(char *destination, const char *source, size_t length, size_t times);
char* mgStringRepeatDuplicate(const char *str, size_t length, size_t times);

const char* mgBasename(const char *filename);

size_t mgDirnameEnd(const char *filename);
char* mgDirname(char *dirname, const char *filename);

unsigned int mgInlineRepresentationLength(const char *str, const char *end);
char* mgInlineRepresentation(char *destination, const char *source, const char *end);

#endif