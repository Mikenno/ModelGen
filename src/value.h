#ifndef MODELGEN_VALUE_H
#define MODELGEN_VALUE_H

#define _MG_VALUES \
	_MG_V(INVALID, "Invalid") \
	_MG_V(INTEGER, "Integer") \
	_MG_V(FLOAT, "Float") \
	_MG_V(STRING, "String") \
	_MG_V(TUPLE, "Tuple") \
	_MG_V(LIST, "List") \
	_MG_V(MAP, "Map") \
	_MG_V(CFUNCTION, "CFunction") \
	_MG_V(PROCEDURE, "Procedure") \
	_MG_V(FUNCTION, "Function") \
	_MG_V(MODULE, "Module")


#define _MG_LONGEST_VALUE_NAME_LENGTH 9


typedef enum MGValueType {
#define _MG_V(value, name) MG_VALUE_##value,
	_MG_VALUES
#undef _MG_V
} MGValueType;


static char *_MG_VALUE_TYPE_NAMES[] = {
#define _MG_V(value, name) name,
	_MG_VALUES
#undef _MG_V
};

#endif