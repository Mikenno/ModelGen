#ifndef MODELGEN_COMPOSITE_TYPES_H
#define MODELGEN_COMPOSITE_TYPES_H

#include <stdint.h>

#include "value.h"


typedef struct MGMapIterator {
	MGValue *map;
	MGValue *key;
	size_t index;
} MGMapIterator;


MGValue* mgCreateValueCFunction(MGCFunction cfunc);
MGValue* mgCreateValueBoundCFunction(MGBoundCFunction cfunc, MGValue *value);


MGValue* mgCreateValueTuple(size_t capacity);
MGValue* mgCreateValueTupleEx(size_t n, ...);

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

MGValue* mgListShallowCopy(const MGValue *list);
#define mgListDeepCopy(list) mgDeepCopyValue(list)

#define mgListLength(list) _mgListLength((list)->data.a)
#define mgListCapacity(list) _mgListCapacity((list)->data.a)
#define mgListItems(list) _mgListItems((list)->data.a)

#define mgListSet(list, index, value) _mgListSet((list)->data.a, _mgListIndexRelativeToAbsolute((list)->data.a, index), value)
#define mgListGet(list, index) ((const MGValue*) _mgListGet((list)->data.a, _mgListIndexRelativeToAbsolute((list)->data.a, index)))


void _mgCreateMap(MGValueMap *map, size_t capacity);
void _mgDestroyMap(MGValueMap *map);

void _mgMapClear(MGValueMap *map);
#define _mgMapRemove(map, key) _mgMapSet(map, key, NULL)

void _mgMapSet(MGValueMap *map, const char *key, MGValue *value);
const MGValue* _mgMapGet(const MGValueMap *map, const char *key);


MGValue* mgCreateValueMap(size_t capacity);

#define mgMapClear(map) _mgMapClear(&(map)->data.m)
#define mgMapRemove(map, key) _mgMapRemove(&(map)->data.m, key)

#define mgMapSet(map, key, value) _mgMapSet(&(map)->data.m, key, value)
#define mgMapGet(map, key) ((const MGValue*) _mgMapGet(&(map)->data.m, key))

#define mgMapSize(map) _mgMapSize((map)->data.m)

void mgMapMerge(MGValue *destination, const MGValue *source, MGbool override);

MGValue* mgMapShallowCopy(const MGValue *map);
#define mgMapDeepCopy(map) mgDeepCopyValue(map)


void mgCreateMapIterator(MGMapIterator *iterator, const MGValue *map);
void mgDestroyMapIterator(MGMapIterator *iterator);

MGbool mgMapIteratorNext(MGMapIterator *iterator, const MGValue **key, const MGValue **value);

#endif