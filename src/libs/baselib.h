#ifndef MODELGEN_BASELIB_H
#define MODELGEN_BASELIB_H

#include "../modelgen.h"

MGValue* _mg_rangei(int start, int stop, int step);

void mgLoadBaseLib(MGModule *module);

#endif