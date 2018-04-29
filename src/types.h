#ifndef MODELGEN_TYPES_H
#define MODELGEN_TYPES_H

typedef enum MGType {
	MG_TYPE_NULL,
	MG_TYPE_INTEGER,
	MG_TYPE_FLOAT,
	MG_TYPE_STRING,
	MG_TYPE_TUPLE,
	MG_TYPE_LIST,
	MG_TYPE_MAP,
	MG_TYPE_CFUNCTION,
	MG_TYPE_FUNCTION,
	MG_TYPE_PROCEDURE,
	MG_TYPE_MODULE
} MGType;

typedef struct MGValue MGValue;

typedef void (*MGTypeCreate)(MGValue *value);
typedef void (*MGTypeCopy)(MGValue *copy, const MGValue *value);
typedef void (*MGTypeDestroy)(MGValue *value);

typedef struct MGTypeData {
	const char *name;
	MGTypeCreate create;
	MGTypeCopy copy;
	MGTypeDestroy destroy;
} MGTypeData;

extern const MGTypeData _mgTypes[];

#define mgGetType(type) (_mgTypes + (int) (type))
#define mgGetTypeName(type) mgGetType(type)->name

#define _MG_LONGEST_TYPE_NAME_LENGTH 6

#endif