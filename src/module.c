
#include <stdlib.h>
#include <string.h>

#include "modelgen.h"
#include "module.h"
#include "utilities.h"


static void _mgDestroyName(MGName *name)
{
	MG_ASSERT(name);
	MG_ASSERT(name->name);
	MG_ASSERT(name->value);

	free(name->name);
	mgDestroyValue(name->value);
}


void mgCreateModule(MGModule *module)
{
	MG_ASSERT(module);

	memset(module, 0, sizeof(MGParser));
}


void mgDestroyModule(MGModule *module)
{
	MG_ASSERT(module);

	for (size_t i = 0; i < module->length; ++i)
		_mgDestroyName(&module->names[i]);
	free(module->names);

	free(module->filename);
}


inline MGValue* mgCreateValue(MGValueType type)
{
	MGValue *value = (MGValue*) malloc(sizeof(MGValue));
	value->type = type;
	return value;
}


void mgDestroyValue(MGValue *value)
{
	MG_ASSERT(value);

	switch (value->type)
	{
	case MG_VALUE_STRING:
		free(value->data.s);
		break;
	case MG_VALUE_TUPLE:
	case MG_VALUE_LIST:
		for (size_t i = 0; i < value->data.a.length; ++i)
			mgDestroyValue(value->data.a.items[i]);
		free(value->data.a.items);
		break;
	default:
		break;
	}

	free(value);
}


static inline void _mgSetValue(MGName *names, size_t i, const char *name, MGValue *value)
{
	names[i].name = mgStringDuplicate(name);
	names[i].value = value;
}


static inline void _mgAddValue(MGModule *module, const char *name, MGValue *value)
{
	if (module->capacity == module->length)
	{
		module->capacity = module->capacity ? module->capacity << 1 : 2;
		module->names = (MGName*) realloc(module->names, module->capacity * sizeof(MGName));
	}

	_mgSetValue(module->names, module->length++, name, value);
}


static inline void _mgSwapNames(MGName *names, size_t i, size_t i2)
{
	MGName temp;
	memcpy(&temp, &names[i], sizeof(MGName));
	memcpy(&names[i], &names[i2], sizeof(MGName));
	memcpy(&names[i2], &temp, sizeof(MGName));
}


void mgModuleSet(MGModule *module, const char *name, MGValue *value)
{
	MG_ASSERT(module);
	MG_ASSERT(name);

	MGName *_name = NULL;
	for (size_t i = 0; (_name == NULL) && (i < module->length); ++i)
		if (strcmp(module->names[i].name, name) == 0)
			_name = &module->names[i];

	if (value)
	{
		if (_name)
		{
			_mgDestroyName(_name);
			_mgSetValue(module->names, (size_t) (_name - module->names), name, value);
		}
		else
			_mgAddValue(module, name, value);
	}
	else if (_name)
	{
		_mgDestroyName(_name);

		if (module->length > 1)
			if (_name != &module->names[module->length - 1])
				_mgSwapNames(module->names, (size_t) (_name - module->names), module->length - 1);

		--module->length;
	}
}


inline MGValue* mgModuleGet(MGModule *module, const char *name)
{
	MG_ASSERT(module);
	MG_ASSERT(name);

	for (size_t i = 0; i < module->length; ++i)
		if (strcmp(module->names[i].name, name) == 0)
			return module->names[i].value;

	return NULL;
}


inline int mgModuleGetInteger(MGModule *module, const char *name, int defaultValue)
{
	MGValue *value = mgModuleGet(module, name);
	return (value && (value->type == MG_VALUE_INTEGER)) ? value->data.i : defaultValue;
}


inline float mgModuleGetFloat(MGModule *module, const char *name, float defaultValue)
{
	MGValue *value = mgModuleGet(module, name);
	return (value && (value->type == MG_VALUE_FLOAT)) ? value->data.f : defaultValue;
}


inline const char* mgModuleGetString(MGModule *module, const char *name, const char *defaultValue)
{
	MGValue *value = mgModuleGet(module, name);
	return (value && (value->type == MG_VALUE_STRING)) ? value->data.s : defaultValue;
}


inline MGValue* mgCreateValueInteger(int i)
{
	MGValue *value = mgCreateValue(MG_VALUE_INTEGER);
	value->data.i = i;
	return value;
}


inline MGValue* mgCreateValueFloat(float f)
{
	MGValue *value = mgCreateValue(MG_VALUE_FLOAT);
	value->data.f = f;
	return value;
}


inline MGValue* mgCreateValueString(const char *s)
{
	MGValue *value = mgCreateValue(MG_VALUE_STRING);
	value->data.s = mgStringDuplicate(s ? s : "");
	return value;
}


inline MGValue* mgCreateValueCFunction(MGCFunction cfunc)
{
	MG_ASSERT(cfunc);

	MGValue *value = mgCreateValue(MG_VALUE_CFUNCTION);
	value->data.cfunc = cfunc;
	return value;
}


inline void mgStringSet(MGValue *value, const char *s)
{
	free(value->data.s);
	value->data.s = mgStringDuplicate(s ? s : "");
}


inline const char* mgStringGet(MGValue *value)
{
	return value->data.s;
}


inline MGValue* mgCreateValueTuple(size_t capacity)
{
	MGValue *value = mgCreateValueList(capacity);
	value->type = MG_VALUE_TUPLE;
	return value;
}


MGValue* mgCreateValueList(size_t capacity)
{
	MGValue *value = mgCreateValue(MG_VALUE_LIST);

	if (capacity > 0)
	{
		value->data.a.items = (MGValue**) malloc(capacity * sizeof(MGValue*));
		value->data.a.capacity = capacity;
	}
	else
	{
		value->data.a.items = NULL;
		value->data.a.capacity = 0;
	}

	value->data.a.length = 0;

	return value;
}


void mgListAdd(MGValue *list, MGValue *value)
{
	MG_ASSERT(list);
	MG_ASSERT((list->type == MG_VALUE_TUPLE) || (list->type == MG_VALUE_LIST));
	MG_ASSERT(value);

	if (list->data.a.capacity == list->data.a.length)
	{
		list->data.a.capacity = list->data.a.capacity ? list->data.a.capacity << 1 : 2;
		list->data.a.items = (MGValue**) realloc(list->data.a.items, list->data.a.capacity * sizeof(MGValue*));
	}

	list->data.a.items[list->data.a.length++] = value;
}


void mgListInsert(MGValue *list, intmax_t index, MGValue *value)
{
	MG_ASSERT(list);
	MG_ASSERT((list->type == MG_VALUE_TUPLE) || (list->type == MG_VALUE_LIST));
	MG_ASSERT(value);

	index = mgRelativeToAbsolute(list, index);
	MG_ASSERT((index >= 0) && (index <= mgListGetLength(list)));

	if (mgListGetLength(list) == 0)
	{
		mgListAdd(list, value);
		return;
	}

	mgListAdd(list, mgListGet(list, -1));

	for (intmax_t i = mgListGetLength(list) - 2; i > index; --i)
		mgListSet(list, i, mgListGet(list, i - 1));

	mgListSet(list, index, value);
}


void mgListRemove(MGValue *list, intmax_t index)
{
	MG_ASSERT(list);
	MG_ASSERT((list->type == MG_VALUE_TUPLE) || (list->type == MG_VALUE_LIST));

	index = mgRelativeToAbsolute(list, index);
	MG_ASSERT((index >= 0) && (index < mgListGetLength(list)));

	mgDestroyValue(list->data.a.items[index]);

	for (intmax_t i = index + 1; i < mgListGetLength(list); ++i)
		mgListSet(list, i - 1, mgListGet(list, i));

	--list->data.a.length;
}


void mgListRemoveRange(MGValue *list, intmax_t begin, intmax_t end)
{
	MG_ASSERT(list);
	MG_ASSERT((list->type == MG_VALUE_TUPLE) || (list->type == MG_VALUE_LIST));

	begin = mgRelativeToAbsolute(list, begin);
	end = mgRelativeToAbsolute(list, end);
	MG_ASSERT(begin <= end);
	MG_ASSERT((begin >= 0) && (end < mgListGetLength(list)));

	if ((begin == 0) && (end == (mgListGetLength(list) - 1)))
	{
		mgListClear(list);
		return;
	}

	intmax_t move = end - begin + 1;

	for (intmax_t i = begin; i <= end; ++i)
		mgDestroyValue(mgListGet(list, i));

	for (intmax_t i = end + 1; i < mgListGetLength(list); ++i)
		mgListSet(list, i - move, mgListGet(list, i));

	list->data.a.length -= move;
}


inline void mgListClear(MGValue *list)
{
	MG_ASSERT(list);
	MG_ASSERT((list->type == MG_VALUE_TUPLE) || (list->type == MG_VALUE_LIST));

	for (size_t i = 0; i < list->data.a.length; ++i)
		mgDestroyValue(list->data.a.items[i]);

	list->data.a.length = 0;
}
