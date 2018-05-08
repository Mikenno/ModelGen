#ifndef MODELGEN_UTILITIES_H
#define MODELGEN_UTILITIES_H

#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <math.h>
#include <float.h>

#ifdef _WIN32
#   define MG_PATH_MAX MAX_PATH
#else
#   define MG_PATH_MAX FILENAME_MAX
#endif

#define _MG_INT_COUNT_DIGITS(x) ((int) floorf(log10f((float) (x))) + 1)

#define MG_EPSILON FLT_EPSILON
#define MG_APPROXIMATELY(a, b, tolerance) (((a) >= (b) - (tolerance)) && ((a) <= (b) + (tolerance)))
#define MG_FEQUAL(a, b) MG_APPROXIMATELY(a, b, MG_EPSILON)

uint32_t mgNextPowerOfTwo(uint32_t x);

int mgStringEndsWith(const char *string, const char *suffix);

char* mgStringReplaceCharacter(char *str, char find, char replace);

char* mgStringDuplicate(const char *str);
char* mgStringDuplicateEx(const char *str, size_t length);
char* mgStringDuplicateFixed(const char *str, size_t count);

char* mgStringRepeat(char *destination, const char *source, size_t length, size_t times);
char* mgStringRepeatDuplicate(const char *str, size_t length, size_t times);

char *mgIntToString(int i);
char *mgFloatToString(float f);

const char* mgBasename(const char *filename);

size_t mgDirnameEnd(const char *filename);
char* mgDirname(char *dirname, const char *filename);

int mgFileExists(const char *filename);

unsigned int mgInlineRepresentationLength(const char *str, const char *end);
char* mgInlineRepresentation(char *destination, const char *source, const char *end);

#endif