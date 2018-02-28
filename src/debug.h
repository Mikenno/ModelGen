#ifndef MODELGEN_DEBUG_H
#define MODELGEN_DEBUG_H

#include "modelgen.h"

MGbool mgDebugRead(const char *filename);
MGbool mgDebugReadHandle(FILE *file, const char *filename);

#endif