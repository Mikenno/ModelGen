#ifndef MODELGEN_FORMAT_H
#define MODELGEN_FORMAT_H

#include <stdio.h>

#include "instance.h"

void mgExportOBJ(MGInstance *instance, FILE *file);
void mgExportTriangles(MGInstance *instance, FILE *file);

#endif