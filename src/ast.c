
#include "ast.h"


const char* const _MG_NODE_NAMES[] = {
#define _MG_N(node, name) name,
	_MG_NODES
#undef _MG_N
};
