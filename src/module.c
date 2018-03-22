
#include <stdlib.h>
#include <string.h>

#include "modelgen.h"
#include "utilities.h"


static void _mgDestroyName(MGName *name)
{
	MG_ASSERT(name);

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
}


MGValue* mgCreateValue(MGValueType type)
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
	names[i].name = mgDuplicateString(name);
	names[i].value = value;
}


static void _mgAddValue(MGModule *module, const char *name, MGValue *value)
{
	if (module->capacity == module->length)
	{
		module->capacity = module->capacity ? module->capacity << 1 : 2;
		module->names = (MGName*) realloc(module->names, module->capacity * sizeof(MGName));
	}

	_mgSetValue(module->names, module->length++, name, value);
}


static void _mgSwapNames(MGName *names, size_t i, size_t i2)
{
	MGName temp;
	memcpy(&temp, &names[i], sizeof(MGName));
	memcpy(&names[i], &names[i2], sizeof(MGName));
	memcpy(&names[i2], &temp, sizeof(MGName));
}


void mgSetValue(MGModule *module, const char *name, MGValue *value)
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


MGValue* mgGetValue(MGModule *module, const char *name)
{
	MG_ASSERT(module);
	MG_ASSERT(name);

	for (size_t i = 0; i < module->length; ++i)
		if (strcmp(module->names[i].name, name) == 0)
			return module->names[i].value;
	return NULL;
}


void mgSetValueInteger(MGModule *module, const char *name, int i)
{
	MG_ASSERT(module);
	MG_ASSERT(name);

	MGValue *value = mgCreateValue(MG_VALUE_INTEGER);
	value->data.i = i;
	mgSetValue(module, name, value);
}


void mgSetValueFloat(MGModule *module, const char *name, float f)
{
	MG_ASSERT(module);
	MG_ASSERT(name);

	MGValue *value = mgCreateValue(MG_VALUE_FLOAT);
	value->data.f = f;
	mgSetValue(module, name, value);
}


void mgSetValueString(MGModule *module, const char *name, const char *s)
{
	MG_ASSERT(module);
	MG_ASSERT(name);
	MG_ASSERT(s);

	MGValue *value = mgCreateValue(MG_VALUE_STRING);
	value->data.s = mgDuplicateString(s);
	mgSetValue(module, name, value);
}


void mgSetValueCFunction(MGModule *module, const char *name, MGCFunction cfunc)
{
	MG_ASSERT(module);
	MG_ASSERT(name);
	MG_ASSERT(cfunc);

	MGValue *value = mgCreateValue(MG_VALUE_CFUNCTION);
	value->data.cfunc = cfunc;
	mgSetValue(module, name, value);
}


int mgGetValueInteger(MGModule *module, const char *name, int defaultValue)
{
	MG_ASSERT(module);
	MG_ASSERT(name);

	MGValue *value = mgGetValue(module, name);
	return (value && (value->type == MG_VALUE_INTEGER)) ? value->data.i : defaultValue;
}


float mgGetValueFloat(MGModule *module, const char *name, float defaultValue)
{
	MG_ASSERT(module);
	MG_ASSERT(name);

	MGValue *value = mgGetValue(module, name);
	return (value && (value->type == MG_VALUE_FLOAT)) ? value->data.f : defaultValue;
}


const char* mgGetValueString(MGModule *module, const char *name, const char *defaultValue)
{
	MG_ASSERT(module);
	MG_ASSERT(name);

	MGValue *value = mgGetValue(module, name);
	return (value && (value->type == MG_VALUE_STRING)) ? value->data.s : defaultValue;
}
