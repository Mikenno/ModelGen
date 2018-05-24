
#include "module.h"
#include "composite.h"
#include "debug.h"


MGValue* mgCreateValueModule(void)
{
	MGValue *module = mgCreateValue(MG_TYPE_MODULE);
	MG_ASSERT(module);

	module->data.module.instance = NULL;
	mgCreateParser(&module->data.module.parser);
	module->data.module.filename = NULL;
	module->data.module.globals = mgCreateValueMap(1 << 4);
	module->data.module.isStatic = MG_FALSE;

	return module;
}


void mgModuleSet(MGValue *module, const char *name, MGValue *value)
{
	MG_ASSERT(module);
	MG_ASSERT(module->type == MG_TYPE_MODULE);
	MG_ASSERT(name);

	_mgMapSet(&module->data.module.globals->data.m, name, value);
}


MGValue* mgModuleGet(MGValue *module, const char *name)
{
	MG_ASSERT(module);
	MG_ASSERT(module->type == MG_TYPE_MODULE);
	MG_ASSERT(name);

	return _mgMapGet(&module->data.module.globals->data.m, name);
}


int mgModuleGetInteger(MGValue *module, const char *name, int defaultValue)
{
	MGValue *value = mgModuleGet(module, name);
	return (value && (value->type == MG_TYPE_INTEGER)) ? value->data.i : defaultValue;
}


float mgModuleGetFloat(MGValue *module, const char *name, float defaultValue)
{
	MGValue *value = mgModuleGet(module, name);
	return (value && (value->type == MG_TYPE_FLOAT)) ? value->data.f : defaultValue;
}


const char* mgModuleGetString(MGValue *module, const char *name, const char *defaultValue)
{
	MGValue *value = mgModuleGet(module, name);
	return (value && (value->type == MG_TYPE_STRING)) ? value->data.str.s : defaultValue;
}
