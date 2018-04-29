
#include "tokens.h"


const char* const _MG_TOKEN_NAMES[] = {
#define _MG_T(token, name) name,
	_MG_TOKENS
#undef _MG_T
};
