
#include "modelgen.h"
#include "value.h"
#include "module.h"
#include "callable.h"
#include "error.h"


// list.add(item [, item...])
// list.extend(iterable)
// list.insert(index, item)
// list.clear()
// list.slice(begin = 0, end = list.size, step = 0): list
// list.reverse(): list
// list.sort(comparator): list


MGValue* mgTypeListAdd(const MGValue *lhs, const MGValue *rhs)
{
	if (((lhs->type == MG_TYPE_TUPLE) || (lhs->type == MG_TYPE_LIST)) && (lhs->type == rhs->type))
	{
		MGValue *list = mgCreateValueList(mgListLength(lhs) + mgListLength(rhs));
		list->type = (lhs->type == MG_TYPE_TUPLE) ? MG_TYPE_TUPLE : MG_TYPE_LIST;

		for (size_t i = 0; i < mgListLength(lhs); ++i)
			mgListAdd(list, mgReferenceValue(_mgListGet(lhs->data.a, i)));

		for (size_t i = 0; i < mgListLength(rhs); ++i)
			mgListAdd(list, mgReferenceValue(_mgListGet(rhs->data.a, i)));

		return list;
	}

	return NULL;
}


MGValue* mgListMul(const MGValue *lhs, const MGValue *rhs)
{
	const MGValue *list;
	int times;

	if (((lhs->type == MG_TYPE_TUPLE) || (lhs->type == MG_TYPE_LIST)) && (rhs->type == MG_TYPE_INTEGER))
	{
		list = lhs;
		times = rhs->data.i;
	}
	else if ((lhs->type == MG_TYPE_INTEGER) || ((rhs->type == MG_TYPE_TUPLE) || (rhs->type == MG_TYPE_LIST)))
	{
		list = rhs;
		times = lhs->data.i;
	}
	else
		return NULL;

	const size_t len = ((mgListLength(list) > 0) && (times > 0)) ? (mgListLength(list) * times) : 0;
	MGValue *repeated = (list->type == MG_TYPE_TUPLE) ? mgCreateValueTuple(len) : mgCreateValueList(len);

	for (size_t i = 0; i < len; ++i)
		mgListAdd(repeated, mgReferenceValue(_mgListGet(list->data.a, i % mgListLength(list))));

	return repeated;
}


static inline size_t _mgListRelativeIndex(const MGValue *list, const MGValue *index)
{
	size_t i = (size_t) _mgListIndexRelativeToAbsolute(list->data.a, mgIntegerGet(index));

	if ((i < 0) || (i >= mgListLength(list)))
	{
		if (mgIntegerGet(index) >= 0)
			mgFatalError("Error: %s index out of range (0 <= %d < %zu)",
			             mgGetTypeName(list->type), mgIntegerGet(index), mgListLength(list));
		else
			mgFatalError("Error: %s index out of range (-%zu <= %d < 0)",
			             mgGetTypeName(list->type), mgListLength(list), mgIntegerGet(index));
	}

	return i;
}


MGValue* mgListSubscriptGet(const MGValue *list, const MGValue *index)
{
	if (index->type != MG_TYPE_INTEGER)
		return NULL;

	MGValue *value = _mgListGet(list->data.a, _mgListRelativeIndex(list, index));
	return value ? mgReferenceValue(value) : MG_NULL_VALUE;
}


MGbool mgListSubscriptSet(const MGValue *list, const MGValue *index, MGValue *value)
{
	if (index->type == MG_TYPE_INTEGER)
	{
		const size_t i = _mgListRelativeIndex(list, index);

		mgDestroyValue(_mgListGet(list->data.a, i));

		if (value)
			_mgListSet(list->data.a, i, mgReferenceValue(value));
		else
			_mgListRemove(((MGValue*) list)->data.a, i);

		return MG_TRUE;
	}

	return MG_FALSE;
}


static MGValue* mg_list_add(MGInstance *instance, const MGValue *list, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, SIZE_MAX);

	for (size_t i = 0; i < argc; ++i)
		mgListAdd((MGValue*) list, mgReferenceValue(argv[i]));

	return MG_NULL_VALUE;
}


static MGValue* mg_list_extend(MGInstance *instance, const MGValue *list, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, 1);
	mgCheckArgumentTypes(instance, argc, argv, 2, MG_TYPE_TUPLE, MG_TYPE_LIST);

	for (size_t i = 0, end = mgListLength(argv[0]); i < end; ++i)
		mgListAdd((MGValue*) list, mgReferenceValue(_mgListGet(argv[0]->data.a, i)));

	return MG_NULL_VALUE;
}


static MGValue* mg_list_insert(MGInstance *instance, const MGValue *list, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 2, 2);
	mgCheckArgumentTypes(instance, argc, argv, 1, MG_TYPE_INTEGER, 0);

	intmax_t index = _mgListIndexRelativeToAbsolute(list->data.a, argv[0]->data.i);
	index = (index > 0) ? index : 0;
	index = (index > (intmax_t) mgListLength(list)) ? (intmax_t) mgListLength(list) : index;

	mgListInsert((MGValue*) list, index, mgReferenceValue(argv[1]));

	return MG_NULL_VALUE;
}


static MGValue* mg_list_clear(MGInstance *instance, const MGValue *list, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 0, 0);

	mgListClear((MGValue*) list);

	return MG_NULL_VALUE;
}


static MGValue* mg_list_slice(MGInstance *instance, const MGValue *list, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 0, 3);
	mgCheckArgumentTypes(instance, argc, argv, 1, MG_TYPE_INTEGER, 1, MG_TYPE_INTEGER, 1, MG_TYPE_INTEGER);

	const intmax_t length = (intmax_t) mgListLength(list);

	intmax_t start = 0;
	intmax_t stop = length;
	intmax_t step = (argc > 2) ? (intmax_t) argv[2]->data.i : 0;

	if (argc > 0)
	{
		start = _mgListIndexRelativeToAbsolute(list->data.a, argv[0]->data.i);
		start = (start > 0) ? start : 0;
	}

	if (argc > 1)
	{
		stop = _mgListIndexRelativeToAbsolute(list->data.a, argv[1]->data.i);
		stop = (stop > (intmax_t) mgListLength(list)) ? length : stop;
	}

	intmax_t difference = stop - start;

	if (difference == 0)
		return mgCreateValueList(0);

	if (step == 0)
		step = (difference > 0) - (difference < 0);

	if ((difference ^ step) < 0)
		return mgCreateValueList(0);

	intmax_t sliceLength = difference / step + ((difference % step) != 0);

	MGValue *slice = mgCreateValueList((size_t) sliceLength);
	for (intmax_t i = start; i < stop; i += step)
		mgListAdd(slice, mgReferenceValue(_mgListGet(list->data.a, i)));

	return slice;
}


static MGValue* mg_list_reverse(MGInstance *instance, const MGValue *list, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 0, 0);

	const size_t length = mgListLength(list);

	for (size_t i = 0; i < length / 2; ++i)
	{
		MGValue *item1 = _mgListGet(list->data.a, i);
		MGValue *item2 = _mgListGet(list->data.a, length - i - 1);

		_mgListSet(list->data.a, i, item2);
		_mgListSet(list->data.a, length - i - 1, item1);
	}

	return mgReferenceValue(list);
}


static MGValue* mg_list_sort(MGInstance *instance, const MGValue *list, size_t argc, const MGValue* const* argv)
{
	mgCheckArgumentCount(instance, argc, 1, 1);
	mgCheckArgumentTypes(instance, argc, argv, MG_TYPE_CFUNCTION, MG_TYPE_BOUND_CFUNCTION, MG_TYPE_FUNCTION);

	const intmax_t length = (intmax_t) mgListLength(list);

	const MGValue *comparator = argv[0];

	for (intmax_t i = length - 1; i >= 0; --i)
	{
		for (intmax_t j = 1; j <= i; ++j)
		{
			MGValue *item1 = _mgListGet(list->data.a, j - 1);
			MGValue *item2 = _mgListGet(list->data.a, j);

			const MGValue* const argv2[2] = { item1, item2 }; // Purposely not referenced

			MGValue *comparison = mgCall(instance, comparator, 2, argv2);
			MG_ASSERT(comparison);
			MG_ASSERT(comparison->type == MG_TYPE_INTEGER);

			if (comparison->data.i)
			{
				_mgListSet(list->data.a, j - 1, item2);
				_mgListSet(list->data.a, j, item1);
			}

			mgDestroyValue(comparison);
		}
	}

	return mgReferenceValue(list);
}


MGValue* mgListAttributeGet(const MGValue *list, const char *key)
{
	if (!strcmp("size", key))
		return mgCreateValueInteger((int) mgListLength(list));
	else if (!strcmp("add", key))
		return mgCreateValueBoundCFunction(mg_list_add, mgReferenceValue(list));
	else if (!strcmp("extend", key))
		return mgCreateValueBoundCFunction(mg_list_extend, mgReferenceValue(list));
	else if (!strcmp("insert", key))
		return mgCreateValueBoundCFunction(mg_list_insert, mgReferenceValue(list));
	else if (!strcmp("clear", key))
		return mgCreateValueBoundCFunction(mg_list_clear, mgReferenceValue(list));
	else if (!strcmp("slice", key))
		return mgCreateValueBoundCFunction(mg_list_slice, mgReferenceValue(list));
	else if (!strcmp("reverse", key))
		return mgCreateValueBoundCFunction(mg_list_reverse, mgReferenceValue(list));
	else if (!strcmp("sort", key))
		return mgCreateValueBoundCFunction(mg_list_sort, mgReferenceValue(list));

	return NULL;
}
