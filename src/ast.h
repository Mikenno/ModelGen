#ifndef MODELGEN_AST_H
#define MODELGEN_AST_H

#define _MG_NODES \
	_MG_N(INVALID, "Invalid") \
	_MG_N(MODULE, "Module") \
	_MG_N(BLOCK, "Block") \
	_MG_N(NAME, "Name") \
	_MG_N(INTEGER, "Integer") \
	_MG_N(FLOAT, "Float") \
	_MG_N(STRING, "String") \
	_MG_N(BIN_OP_ADD, "BinOpAdd") \
	_MG_N(BIN_OP_SUB, "BinOpSub") \
	_MG_N(BIN_OP_MUL, "BinOpMul") \
	_MG_N(BIN_OP_DIV, "BinOpDiv") \
	_MG_N(BIN_OP_INT_DIV, "BinOpIntDiv") \
	_MG_N(BIN_OP_MOD, "BinOpMod") \
	_MG_N(BIN_OP_EQ, "BinOpEq") \
	_MG_N(BIN_OP_NOT_EQ, "BinOpNotEq") \
	_MG_N(BIN_OP_LESS, "BinOpLess") \
	_MG_N(BIN_OP_GREATER, "BinOpGreater") \
	_MG_N(BIN_OP_LESS_EQ, "BinOpLessEq") \
	_MG_N(BIN_OP_GREATER_EQ, "BinOpGreaterEq") \
	_MG_N(BIN_OP_AND, "BinOpAnd") \
	_MG_N(BIN_OP_OR, "BinOpOr") \
	_MG_N(UNARY_OP_POS, "UnaryOpPos") \
	_MG_N(UNARY_OP_NEG, "UnaryOpNeg") \
	_MG_N(UNARY_OP_NOT, "UnaryOpNot") \
	_MG_N(TUPLE, "Tuple") \
	_MG_N(LIST, "List") \
	_MG_N(RANGE, "Range") \
	_MG_N(MAP, "Map") \
	_MG_N(CALL, "Call") \
	_MG_N(FOR, "For") \
	_MG_N(IF, "If") \
	_MG_N(ASSIGN, "Assign") \
	_MG_N(ASSIGN_ADD, "AssignAdd") \
	_MG_N(ASSIGN_SUB, "AssignSub") \
	_MG_N(ASSIGN_MUL, "AssignMul") \
	_MG_N(ASSIGN_DIV, "AssignDiv") \
	_MG_N(ASSIGN_MOD, "AssignMod") \
	_MG_N(SUBSCRIPT, "Subscript") \
	_MG_N(ATTRIBUTE, "Attribute") \
	_MG_N(PROCEDURE, "Procedure") \
	_MG_N(EMIT, "Emit") \
	_MG_N(FUNCTION, "Function") \
	_MG_N(RETURN, "Return") \
	_MG_N(DELETE, "Delete") \
	_MG_N(IMPORT, "Import") \
	_MG_N(IMPORT_FROM, "ImportFrom") \
	_MG_N(AS, "As") \
	_MG_N(ASSERT, "Assert")


#define _MG_LONGEST_NODE_NAME_LENGTH 14


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