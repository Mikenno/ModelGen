#ifndef MODELGEN_MODULE_H
#define MODELGEN_MODULE_H

#include <stddef.h>
#include <stdint.h>

#include "modelgen.h"

void mgCreateModule(MGModule *module);
void mgDestroyModule(MGModule *module);

MGValue* mgCreateValue(MGValueType type);
void mgDestroyValue(MGValue *value);

void mgModuleSet(MGModule *module, const char *name, MGValue *value);
MGValue* mgModuleGet(MGModule *module, const char *name);

#define mgModuleSetInteger(module, name, i) mgModuleSet(module, name, mgCreateValueInteger(i))
#define mgModuleSetFloat(module, name, f) mgModuleSet(module, name, mgCreateValueFloat(f))
#define mgModuleSetString(module, name, s) mgModuleSet(module, name, mgCreateValueString(s))
#define mgModuleSetCFunction(module, name, cfunc) mgModuleSet(module, name, mgCreateValueCFunction(cfunc))

int mgModuleGetInteger(MGModule *module, const char *name, int defaultValue);
float mgModuleGetFloat(MGModule *module, const char *name, float defaultValue);
const char* mgModuleGetString(MGModule *module, const char *name, const char *defaultValue);

MGValue* mgCreateValueInteger(int i);
MGValue* mgCreateValueFloat(float f);
MGValue* mgCreateValueString(const char *s);
MGValue* mgCreateValueCFunction(MGCFunction cfunc);

#define mgIntegerSet(value, _i) value->data.i = _i
#define mgIntegerGet(value) value->data.i

#define mgFloatSet(value, _f) value->data.f = _f
#define mgFloatGet(value) value->data.f

void mgStringSet(MGValue *value, const char *s);
const char* mgStringGet(MGValue *value);

MGValue* mgCreateValueTuple(size_t capacity);

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

MGValue* mgCreateValueList(size_t capacity);

void mgListAdd(MGValue *list, MGValue *value);
void mgListInsert(MGValue *list, intmax_t index, MGValue *value);

void mgListRemove(MGValue *list, intmax_t index);
void mgListRemoveRange(MGValue *list, intmax_t begin, intmax_t end);

void mgListClear(MGValue *list);

#define mgListLength(list) _mgListLength(list->data.a)
#define mgListCapacity(list) _mgListCapacity(list->data.a)
#define mgListItems(list) _mgListItems(list->data.a)

#define mgListSet(list, index, value) _mgListSet(list->data.a, _mgListIndexRelativeToAbsolute(list->data.a, index), value)
#define mgListGet(list, index) _mgListGet(list->data.a, _mgListIndexRelativeToAbsolute(list->data.a, index))

#endif