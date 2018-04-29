
#include "value.h"


const char* const _MG_VALUE_TYPE_NAMES[] = {
#define _MG_V(value, name) name,
	_MG_VALUES
#undef _MG_V
};
