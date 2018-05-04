#ifndef MODELGEN_MODULE_H
#define MODELGEN_MODULE_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "modelgen.h"

typedef struct MGMapIterator {
	MGValue *map;
	MGValue *key;
	size_t index;
} MGMapIterator;

MGValue* mgCreateValueModule(void);

void mgModuleSet(MGValue *module, const char *name, MGValue *value);
MGValue* mgModuleGet(MGValue *module, const char *name);

#define mgModuleSetInteger(module, name, i) mgModuleSet(module, name, mgCreateValueInteger(i))
#define mgModuleSetFloat(module, name, f) mgModuleSet(module, name, mgCreateValueFloat(f))
#define mgModuleSetString(module, name, s) mgModuleSet(module, name, mgCreateValueString(s))
#define mgModuleSetCFunction(module, name, cfunc) mgModuleSet(module, name, mgCreateValueCFunction(cfunc))

int mgModuleGetInteger(MGValue *module, const char *name, int defaultValue);
float mgModuleGetFloat(MGValue *module, const char *name, float defaultValue);
const char* mgModuleGetString(MGValue *module, const char *name, const char *defaultValue);

void _mgCreateMap(MGValueMap *map, size_t capacity);
void _mgDestroyMap(MGValueMap *map);

void _mgMapClear(MGValueMap *map);
#define _mgMapRemove(map, key) _mgMapSet(map, key, NULL)

void _mgMapSet(MGValueMap *map, const char *key, MGValue *value);
MGValue* _mgMapGet(const MGValueMap *map, const char *key);

#define mgCreateValueNull() mgCreateValueEx(MG_TYPE_NULL)
MGValue* mgCreateValueInteger(int i);
MGValue* mgCreateValueFloat(float f);
#define mgCreateValueString(s) mgCreateValueStringEx(s, MG_STRING_USAGE_COPY)
MGValue* mgCreateValueStringEx(const char *s, MGStringUsage usage);
MGValue* mgCreateValueCFunction(MGCFunction cfunc);
MGValue* mgCreateValueBoundCFunction(MGBoundCFunction cfunc, MGValue *value);
MGValue* mgCreateValueTuple(size_t capacity);
MGValue* mgCreateValueTupleEx(size_t n, ...);
MGValue* mgCreateValueList(size_t capacity);
MGValue* mgCreateValueMap(size_t capacity);

#define mgIntegerSet(value, _i) value->data.i = _i
#define mgIntegerGet(value) value->data.i

#define mgFloatSet(value, _f) value->data.f = _f
#define mgFloatGet(value) value->data.f

#define mgStringSet(value, s) mgStringSetEx(value, s, MG_STRING_USAGE_COPY)
void mgStringSetEx(MGValue *value, const char *s, MGStringUsage usage);
#define mgStringGet(value) ((const char*) (value)->data.str.s)
#define mgStringLength(value) (value)->data.str.length

#define mgTupleClear mgListClear

#define mgTupleLength mgListLength
#define mgTupleCapacity mgListCapacity
#define mgTupleItems mgListItems

#define mgTupleSet mgListSet
#define mgTupleGet mgListGet

#define mgTupleAdd mgListAdd
#define mgTupleInsert mgListInsert

#define mgTupleRemove mgListRemove
#define mgTupleRemoveRange mgListRemoveRange

void mgListAdd(MGValue *list, MGValue *value);
void mgListInsert(MGValue *list, intmax_t index, MGValue *value);

void mgListRemove(MGValue *list, intmax_t index);
void mgListRemoveRange(MGValue *list, intmax_t begin, intmax_t end);

void mgListClear(MGValue *list);

MGValue* mgListShallowCopy(const MGValue *list);
#define mgListDeepCopy(list) mgDeepCopyValue(list)

#define mgListLength(list) _mgListLength((list)->data.a)
#define mgListCapacity(list) _mgListCapacity((list)->data.a)
#define mgListItems(list) _mgListItems((list)->data.a)

#define mgListSet(list, index, value) _mgListSet((list)->data.a, _mgListIndexRelativeToAbsolute((list)->data.a, index), value)
#define mgListGet(list, index) _mgListGet((list)->data.a, _mgListIndexRelativeToAbsolute((list)->data.a, index))

#define mgMapClear(map) _mgMapClear(&(map)->data.m)
#define mgMapRemove(map, key) _mgMapRemove(&(map)->data.m, key)

#define mgMapSet(map, key, value) _mgMapSet(&(map)->data.m, key, value)
#define mgMapGet(map, key) _mgMapGet(&(map)->data.m, key)

#define mgMapSize(map) _mgMapSize((map)->data.m)

void mgCreateMapIterator(MGMapIterator *iterator, const MGValue *map);
void mgDestroyMapIterator(MGMapIterator *iterator);

MGbool mgMapNext(MGMapIterator *iterator, const MGValue **key, const MGValue **value);
void mgMapMerge(MGValue *destination, const MGValue *source, MGbool override);
#define mgMapDeepCopy(map) mgDeepCopyValue(map)
MGValue* mgMapCopy(const MGValue *map);

#endif