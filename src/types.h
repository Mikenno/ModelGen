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
	MG_TYPE_BOUND_CFUNCTION,
	MG_TYPE_FUNCTION,
	MG_TYPE_PROCEDURE,
	MG_TYPE_MODULE
} MGType;

typedef char MGbool;
typedef MGbool MGtribool;

#define MG_INDETERMINATE (MGbool) (-1)
#define MG_FALSE (MGbool) 0
#define MG_TRUE (MGbool) 1

typedef struct MGInstance MGInstance;
typedef struct MGValue MGValue;

typedef MGValue* (*MGCFunction)(MGInstance *instance, size_t argc, const MGValue* const* argv);
typedef MGValue* (*MGBoundCFunction)(MGInstance *instance, const MGValue *value, size_t argc, const MGValue* const* argv);

typedef void (*MGTypeCreate)(MGValue *value);
typedef void (*MGTypeCopy)(MGValue *copy, const MGValue *value, MGbool shallow);
typedef void (*MGTypeDestroy)(MGValue *value);

typedef MGValue* (*MGTypeConvert)(const MGValue *value, MGType type);

typedef MGbool (*MGTypeTruthValue)(const MGValue *value);

typedef char* (*MGTypeToString)(const MGValue *value);

typedef MGValue* (*MGTypeUnaryOp)(const MGValue *operand);
typedef MGValue* (*MGTypeBinOp)(const MGValue *lhs, const MGValue *rhs);
typedef MGtribool (*MGTypeBinOpCompare)(const MGValue *lhs, const MGValue *rhs);

typedef MGValue* (*MGTypeSubscriptGet)(const MGValue *collection, const MGValue *index);
typedef MGbool (*MGTypeSubscriptSet)(const MGValue *collection, const MGValue *index, MGValue *value);

typedef MGValue* (*MGTypeAttributeGet)(const MGValue *collection, const char *key);
typedef MGbool (*MGTypeAttributeSet)(const MGValue *collection, const char *key, MGValue *value);

typedef struct MGTypeData {
	const char *name;
	MGTypeCreate create;
	MGTypeCopy copy;
	MGTypeDestroy destroy;
	MGTypeConvert convert;
	MGTypeTruthValue truth;
	MGTypeToString str;
	MGTypeUnaryOp pos;
	MGTypeUnaryOp neg;
	MGTypeUnaryOp inv;
	MGTypeBinOp add;
	MGTypeBinOp sub;
	MGTypeBinOp mul;
	MGTypeBinOp div;
	MGTypeBinOp intdiv;
	MGTypeBinOp mod;
	MGTypeBinOpCompare eq;
	MGTypeBinOpCompare lt;
	MGTypeBinOpCompare le;
	MGTypeSubscriptGet subGet;
	MGTypeSubscriptSet subSet;
	MGTypeAttributeGet attrGet;
	MGTypeAttributeSet attrSet;
} MGTypeData;

extern const MGTypeData _mgTypes[];

#define mgGetType(type) (_mgTypes + (int) (type))
#define mgGetTypeName(type) mgGetType(type)->name

#define _MG_LONGEST_TYPE_NAME_LENGTH 6

MGType mgLookupType(const char *name);
const MGTypeData* mgLookupTypeData(const char *name);

#endif