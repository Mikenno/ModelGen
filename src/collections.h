#ifndef MODELGEN_COLLECTIONS_H
#define MODELGEN_COLLECTIONS_H

#include <stddef.h>
#include <stdlib.h>

#define _MGList(type) \
	struct { size_t length, capacity; type *items; }

#define _mgListInitialize(list) \
	((list).length = 0, (list).capacity = 0, (list).items = NULL)

#define _mgListCreate(type, list, n) \
	((list).length = 0, (list).capacity = (n), (list).items = (type*) malloc((n) * sizeof(type)))

#define _mgListDestroy(list) \
	free((list).items)

#define _mgListClear(list) \
	(list).length = 0

#define _mgListResize(type, list, n) \
	((list).capacity = (n), (list).items = (type*) realloc((list).items, (n) * sizeof(type)))

#define _mgListGrow(type, list) \
	((list).capacity = (list).capacity ? (list).capacity << 1 : 2, \
	(list).items = (type*) realloc((list).items, (list).capacity * sizeof(type)))

#define _mgListLength(list) (list).length
#define _mgListCapacity(list) (list).capacity
#define _mgListItems(list) (list).items

#define _mgListSet(list, index, item) (list).items[index] = item
#define _mgListGet(list, index) (list).items[index]

#define _mgListPop(list) \
	(list).items[--(list).length]

#define _mgListAddUninitialized(type, list) \
	do { \
		if ((list).capacity == (list).length) \
			_mgListGrow(type, list); \
	} while (0)

#define _mgListAdd(type, list, item) \
	do { \
		if ((list).capacity == (list).length) \
			_mgListGrow(type, list); \
		(list).items[(list).length++] = item; \
	} while (0)

#define _mgListInsert(type, list, index, item) \
	do { \
		if ((_mgListLength(list) == (index)) || (_mgListLength(list) == 0)) \
			_mgListAdd(type, list, item); \
		else { \
			size_t _i = _mgListLength(list) - 1; \
			_mgListAdd(type, list, _mgListGet(list, _i)); \
			for (_i = _mgListLength(list) - 2; _i > (index); --_i) \
				_mgListSet(list, _i, _mgListGet(list, _i - 1)); \
			_mgListSet(list, index, item); \
		} \
	} while (0)

#define _mgListRemove(list, index) \
	do { \
		for (size_t _i = (index) + 1; _i < _mgListLength(list); ++_i) \
			_mgListSet(list, _i - 1, _mgListGet(list, _i)); \
		--(list).length; \
	} while (0)

#define _mgListRemoveRange(list, begin, end) \
	do { \
		if (((begin) == 0) && ((end) == (_mgListLength(list) - 1))) \
			_mgListClear(list); \
		else { \
			size_t _move = (end) - (begin) + 1; \
			for (size_t _i = (end) + 1; _i < _mgListLength(list); ++_i) \
				_mgListSet(list, _i - _move, _mgListGet(list, _i)); \
			(list).length -= _move; \
		} \
	} while (0)

#define _mgListIndexRelativeToAbsolute(list, index) \
	(((index) < 0) ? (intmax_t) (list).length + (index) : (index))


#define _MGPair(k, v) struct { k key; v value; }


#define _mgMapSize(map) _mgListLength(map)

#endif