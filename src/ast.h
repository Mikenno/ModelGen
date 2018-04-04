#ifndef MODELGEN_AST_H
#define MODELGEN_AST_H

#define _MG_NODES \
	_MG_N(INVALID, "Invalid") \
	_MG_N(MODULE, "Module") \
	_MG_N(BLOCK, "Block") \
	_MG_N(IDENTIFIER, "Identifier") \
	_MG_N(INTEGER, "Integer") \
	_MG_N(FLOAT, "Float") \
	_MG_N(STRING, "String") \
	_MG_N(BIN_OP, "BinOp") \
	_MG_N(UNARY_OP, "UnaryOp") \
	_MG_N(TUPLE, "Tuple") \
	_MG_N(LIST, "List") \
	_MG_N(RANGE, "Range") \
	_MG_N(MAP, "Map") \
	_MG_N(CALL, "Call") \
	_MG_N(FOR, "For") \
	_MG_N(IF, "If") \
	_MG_N(ASSIGN, "Assign") \
	_MG_N(SUBSCRIPT, "Subscript") \
	_MG_N(PROCEDURE, "Procedure") \
	_MG_N(EMIT, "Emit") \
	_MG_N(FUNCTION, "Function") \
	_MG_N(RETURN, "Return")


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