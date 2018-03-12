#ifndef MODELGEN_AST_H
#define MODELGEN_AST_H

#define _MG_NODES \
	_MG_N(INVALID, "Invalid") \
	_MG_N(MODULE, "Module") \
	_MG_N(IDENTIFIER, "Identifier") \
	_MG_N(INTEGER, "Integer") \
	_MG_N(FLOAT, "Float") \
	_MG_N(STRING, "String") \
	_MG_N(BIN_OP, "BinOp") \
	_MG_N(UNARY_OP, "UnaryOp") \
	_MG_N(TUPLE, "Tuple") \
	_MG_N(CALL, "Call") \
	_MG_N(ASSIGN, "Assign") \
	_MG_N(INDEX, "Index")


#define _MG_LONGEST_NODE_NAME_LENGTH 10


typedef enum MGNodeType {
#define _MG_N(node, name) MG_NODE_##node,
	_MG_NODES
#undef _MG_N
} MGNodeType;


static char *_MG_NODE_NAMES[] = {
#define _MG_N(node, name) name,
	_MG_NODES
#undef _MG_N
};

#endif