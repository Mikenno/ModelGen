
#include <stdio.h>
#include <string.h>

#include "modelgen.h"
#include "module.h"
#include "utilities.h"


extern MGValue* mgCreateBaseLib(void);
extern MGValue* mgCreateMathLib(void);


struct {
	const char *name;
	MGValue* (*create)(void);
} _mgStaticModules[] = {
	{ "base", mgCreateBaseLib },
	{ "math", mgCreateMathLib },
	{ NULL, NULL }
};


static inline char* _mgFilenameToImportName(const char *filename)
{
	MG_ASSERT(filename);

	char *name = NULL;
	const char *ext = strchr(filename, '.');

	if (ext)
		name = mgStringDuplicateFixed(filename, ext - filename);
	else
		name = mgStringDuplicate(filename);

	mgStringReplaceCharacter(name, '/', '.');

	return name;
}


static inline char* _mgImportNameToFilename(const char *name)
{
	MG_ASSERT(name);

	char *filename = (char*) malloc((strlen(name) + 3 + 1) * sizeof(char));

	strcpy(filename, name);
	mgStringReplaceCharacter(filename, '.', '/');
	strcat(filename, ".mg");

	return filename;
}


void mgCreateInstance(MGInstance *instance)
{
	MG_ASSERT(instance);

	memset(instance, 0, sizeof(MGInstance));

	instance->modules = mgCreateValueMap(1 << 3);
	instance->staticModules = mgCreateValueMap(1 << 3);

	_mgListCreate(MGVertex, instance->vertices, 1 << 9);

	for (int i = 0; _mgStaticModules[i].name; ++i)
	{
		MG_ASSERT(_mgStaticModules[i].create);

		MGValue *module = _mgStaticModules[i].create();
		MG_ASSERT(module);
		MG_ASSERT(module->type == MG_VALUE_MODULE);

		module->data.module.isStatic = MG_TRUE;
		MG_ASSERT(mgMapGet(instance->staticModules, _mgStaticModules[i].name) == NULL);

		mgMapSet(instance->staticModules, _mgStaticModules[i].name, module);
	}
}


void mgDestroyInstance(MGInstance *instance)
{
	MG_ASSERT(instance);

	mgDestroyValue(instance->modules);
	mgDestroyValue(instance->staticModules);

	_mgListDestroy(instance->vertices);
}


inline void mgCreateStackFrame(MGStackFrame *frame, MGValue *module)
{
	mgCreateStackFrameEx(frame, module, mgCreateValueMap(1 << 4));
}


void mgCreateStackFrameEx(MGStackFrame *frame, MGValue *module, MGValue *locals)
{
	MG_ASSERT(frame);
	MG_ASSERT(module);
	MG_ASSERT(module->type == MG_VALUE_MODULE);
	MG_ASSERT(locals);

	memset(frame, 0, sizeof(MGStackFrame));

	frame->module = module;
	frame->locals = locals;
}


void mgDestroyStackFrame(MGStackFrame *frame)
{
	MG_ASSERT(frame);

	if (frame->value)
		mgDestroyValue(frame->value);

	mgDestroyValue(frame->module);
	mgDestroyValue(frame->locals);
}


void mgPushStackFrame(MGInstance *instance, MGStackFrame *frame)
{
	MG_ASSERT(instance);
	MG_ASSERT(frame);
	MG_ASSERT(instance->callStackTop != frame);
	MG_ASSERT(frame->last == NULL);
	MG_ASSERT(frame->next == NULL);

	frame->last = instance->callStackTop;

	if (instance->callStackTop)
		instance->callStackTop->next = frame;

	instance->callStackTop = frame;
}


void mgPopStackFrame(MGInstance *instance, MGStackFrame *frame)
{
	MG_ASSERT(instance);
	MG_ASSERT(frame);
	MG_ASSERT(instance->callStackTop == frame);
	MG_ASSERT(frame->next == NULL);

	instance->callStackTop = frame->last;

	if (instance->callStackTop)
		instance->callStackTop->next = NULL;

	frame->last = NULL;
}


static inline MGValue* _mgModuleLoadFile(MGInstance *instance, const char *filename, const char *name)
{
	MG_ASSERT(instance);
	MG_ASSERT(filename);
	MG_ASSERT(name);

	MGValue *module = mgMapGet(instance->modules, name);

	if (module == NULL)
	{
		module = mgCreateValueModule();

		module->data.module.instance = instance;
		module->data.module.filename = mgStringDuplicate(filename);

		if (mgParseFile(&module->data.module.parser, filename))
			mgMapSet(instance->modules, name, module);
		else
		{
			fprintf(stderr, "Error: Failed loading module \"%s\"\n", name);
			mgDestroyValue(module);
			module = NULL;
		}
	}

	MG_ASSERT(module);

	return mgReferenceValue(module);
}


static inline MGValue* _mgModuleLoadFileHandle(MGInstance *instance, FILE *file, const char *name)
{
	MG_ASSERT(instance);
	MG_ASSERT(file);
	MG_ASSERT(name);

	MGValue *module = mgMapGet(instance->modules, name);

	if (module == NULL)
	{
		module = mgCreateValueModule();

		module->data.module.instance = instance;

		if (mgParseFileHandle(&module->data.module.parser, file))
			mgMapSet(instance->modules, name, module);
		else
		{
			fprintf(stderr, "Error: Failed loading module \"%s\"\n", name);
			mgDestroyValue(module);
			module = NULL;
		}
	}

	MG_ASSERT(module);

	return mgReferenceValue(module);
}


static inline MGValue* _mgModuleLoadString(MGInstance *instance, const char *string, const char *name)
{
	MG_ASSERT(instance);
	MG_ASSERT(string);
	MG_ASSERT(name);

	MGValue *module = mgMapGet(instance->modules, name);

	if (module == NULL)
	{
		module = mgCreateValueModule();

		module->data.module.instance = instance;

		if (mgParseString(&module->data.module.parser, string))
			mgMapSet(instance->modules, name, module);
		else
		{
			fprintf(stderr, "Error: Failed loading module \"%s\"\n", name);
			mgDestroyValue(module);
			module = NULL;
		}
	}

	MG_ASSERT(module);

	return mgReferenceValue(module);
}


static inline void _mgImportDefaultInto(MGInstance *instance, MGValue *module)
{
	MGValue *base = mgMapGet(instance->staticModules, "base");
	MG_ASSERT(base);
	MG_ASSERT(base->type == MG_VALUE_MODULE);

	for (size_t i = 0; i < mgMapSize(base->data.module.globals); ++i)
	{
		MGValueMapPair *pair = &_mgListGet(base->data.module.globals->data.m, i);

		mgModuleSet(module, pair->key, mgReferenceValue(pair->value));
	}
}


static inline void _mgRunModule(MGInstance *instance, MGValue *module)
{
	MG_ASSERT(instance);
	MG_ASSERT(module);
	MG_ASSERT(module->type == MG_VALUE_MODULE);
	MG_ASSERT(module->data.module.instance == instance);

	MGStackFrame frame;
	mgCreateStackFrameEx(&frame, mgReferenceValue(module), mgReferenceValue(module->data.module.globals));

	frame.state = MG_STACK_FRAME_STATE_ACTIVE;

	mgPushStackFrame(instance, &frame);
	_mgImportDefaultInto(instance, module);
	mgInterpret(module);
	mgPopStackFrame(instance, &frame);

	mgDestroyStackFrame(&frame);
}


static inline MGValue* _mgImportModuleFile(MGInstance *instance, const char *filename, const char *name)
{
	MG_ASSERT(instance);
	MG_ASSERT(filename);
	MG_ASSERT(name);

	MGValue *module = mgMapGet(instance->modules, name);

	if (module == NULL)
		module = mgMapGet(instance->staticModules, name);

	if (module == NULL)
	{
		module = mgCreateValueModule();

		module->data.module.instance = instance;
		module->data.module.filename = mgStringDuplicate(filename);

		if (mgParseFile(&module->data.module.parser, filename))
		{
			mgMapSet(instance->modules, name, module);
			_mgRunModule(instance, module);
		}
		else
		{
			fprintf(stderr, "Error: Failed loading module \"%s\"\n", name);
			mgDestroyValue(module);
			module = NULL;
		}
	}

	MG_ASSERT(module);

	return mgReferenceValue(module);
}


void mgRunFile(MGInstance *instance, const char *filename, const char *name)
{
	MG_ASSERT(instance);
	MG_ASSERT(filename);

	char *_name = NULL;
	MGValue *module = _mgModuleLoadFile(instance, filename, name ? name : (_name = _mgFilenameToImportName(filename)));
	_mgRunModule(instance, module);
	mgDestroyValue(module);
	free(_name);
}


void mgRunFileHandle(MGInstance *instance, FILE *file, const char *name)
{
	MG_ASSERT(instance);
	MG_ASSERT(file);
	MG_ASSERT(name);

	MGValue *module = _mgModuleLoadFileHandle(instance, file, name);
	_mgRunModule(instance, module);
	mgDestroyValue(module);
}


void mgRunString(MGInstance *instance, const char *string, const char *name)
{
	MG_ASSERT(instance);
	MG_ASSERT(string);
	MG_ASSERT(name);

	MGValue *module = _mgModuleLoadString(instance, string, name);
	_mgRunModule(instance, module);
	mgDestroyValue(module);
}


MGValue* mgImportModule(MGInstance *instance, const char *name)
{
	MG_ASSERT(instance);
	MG_ASSERT(name);

	char *filename = _mgImportNameToFilename(name);
	MGValue *module = _mgImportModuleFile(instance, filename, name);
	free(filename);

	MG_ASSERT(module);

	return module;
}
