#ifndef MODELGEN_FILE_H
#define MODELGEN_FILE_H

#include <stddef.h>
#include <string.h>
#include <stdio.h>

#ifdef _WIN32
#   include <windows.h>
#else
#   include <unistd.h>
#endif

#ifdef _WIN32
#   define MG_PATH_MAX MAX_PATH
#else
#   define MG_PATH_MAX FILENAME_MAX
#endif

const char* mgBasename(const char *filename);

size_t mgDirnameEnd(const char *filename);
char* mgDirname(char *dirname, const char *filename);

int mgFileExists(const char *filename);

char* mgReadFile(const char *filename, size_t *length);
char* mgReadFileHandle(FILE *file, size_t *length);

typedef void (*MGWalkFilesCallback)(const char *filename);

int mgWalkFiles(const char *directory, MGWalkFilesCallback callback);

#endif