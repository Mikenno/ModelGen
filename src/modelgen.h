#ifndef MODELGEN_H
#define MODELGEN_H

#define MG_MAJOR_VERSION 0
#define MG_MINOR_VERSION 1
#define MG_PATCH_VERSION 0

#define MG_VERSION "0.1.0"

#ifdef DEBUG
#	define MG_DEBUG
#endif

typedef unsigned char MGbool;

#define MG_FALSE 0
#define MG_TRUE 1

char* mgReadFile(const char *filename, size_t *length);
char* mgReadFileHandle(FILE *file, size_t *length);

#endif