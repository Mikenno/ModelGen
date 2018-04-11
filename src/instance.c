
#include <stdio.h>
#include <string.h>

#include "modelgen.h"
#include "module.h"
#include "utilities.h"

#include "libs/baselib.h"
#include "libs/mathlib.h"


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


static inline void _mgDestroyNameModule(MGNameModule *pair)
{
	MG_ASSERT(pair);
	MG_ASSERT(pair->key);

	free(pair->key);
	mgDestroyModule(pair->value);
}


void mgCreateInstance(MGInstance *instance)
{
	MG_ASSERT(instance);

	memset(instance, 0, sizeof(MGInstance));

	_mgListCreate(MGVertex, instance->vertices, 1 << 9);
}


void mgDestroyInstance(MGInstance *instance)
{
	MG_ASSERT(instance);

	for (size_t i = 0; i < _mgListLength(instance->modules); ++i)
		_mgDestroyNameModule(&_mgListGet(instance->modules, i));
	_mgListDestroy(instance->modules);

	_mgListDestroy(instance->vertices);
}


inline void mgCreateStackFrame(MGStackFrame *frame)
{
	mgCreateStackFrameEx(frame, mgCreateValueMap(1 << 4));
}


void mgCreateStackFrameEx(MGStackFrame *frame, MGValue *locals)
{
	MG_ASSERT(frame);
	MG_ASSERT(locals);

	memset(frame, 0, sizeof(MGStackFrame));

	frame->locals = locals;
}


void mgDestroyStackFrame(MGStackFrame *frame)
{
	MG_ASSERT(frame);

	if (frame->value)
		mgDestroyValue(frame->value);

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


static void _mgInstanceSet(MGInstance *instance, const char *name, MGModule *module)
{
	MG_ASSERT(instance);
	MG_ASSERT(name);

	MGNameModule *pair = NULL;
	for (size_t i = 0; (pair == NULL) && (i < _mgListLength(instance->modules)); ++i)
		if (strcmp(_mgListGet(instance->modules, i).key, name) == 0)
			pair = &_mgListGet(instance->modules, i);

	if (module)
	{
		if (pair)
			_mgDestroyNameModule(pair);
		else
		{
			_mgListAddUninitialized(MGNameModule, instance->modules);
			pair = &_mgListGet(instance->modules, _mgListLength(instance->modules)++);
		}

		pair->key = mgStringDuplicate(name);
		pair->value = module;
	}
	else if (pair)
	{
		_mgDestroyNameModule(pair);

		if ((_mgListLength(instance->modules) > 1) && (pair != &_mgListGet(instance->modules, _mgListLength(instance->modules) - 1)))
			*pair = _mgListGet(instance->modules, _mgListLength(instance->modules) - 1);

		--_mgListLength(instance->modules);
	}
}


static inline MGModule* _mgInstanceGet(MGInstance *instance, const char *name)
{
	MG_ASSERT(instance);
	MG_ASSERT(name);

	for (size_t i = 0; i < _mgListLength(instance->modules); ++i)
		if (strcmp(_mgListGet(instance->modules, i).key, name) == 0)
			return _mgListGet(instance->modules, i).value;

	return NULL;
}


static inline MGModule* _mgModuleLoadFile(MGInstance *instance, const char *filename, const char *name)
{
	MG_ASSERT(instance);
	MG_ASSERT(filename);
	MG_ASSERT(name);

	MGModule *module = _mgInstanceGet(instance, name);

	if (module == NULL)
	{
		module = mgCreateModule();

		module->instance = instance;
		module->filename = mgStringDuplicate(filename);

		if (mgParseFile(&module->parser, filename))
			_mgInstanceSet(instance, name, module);
		else
		{
			fprintf(stderr, "Error: Failed loading module \"%s\"\n", name);
			mgDestroyModule(module);
			module = NULL;
		}
	}

	MG_ASSERT(module);

	return module;
}


static inline MGModule* _mgModuleLoadFileHandle(MGInstance *instance, FILE *file, const char *name)
{
	MG_ASSERT(instance);
	MG_ASSERT(file);
	MG_ASSERT(name);

	MGModule *module = _mgInstanceGet(instance, name);

	if (module == NULL)
	{
		module = mgCreateModule();

		module->instance = instance;

		if (mgParseFileHandle(&module->parser, file))
			_mgInstanceSet(instance, name, module);
		else
		{
			fprintf(stderr, "Error: Failed loading module \"%s\"\n", name);
			mgDestroyModule(module);
			module = NULL;
		}
	}

	MG_ASSERT(module);

	return module;
}


static inline MGModule* _mgModuleLoadString(MGInstance *instance, const char *string, const char *name)
{
	MG_ASSERT(instance);
	MG_ASSERT(string);
	MG_ASSERT(name);

	MGModule *module = _mgInstanceGet(instance, name);

	if (module == NULL)
	{
		module = mgCreateModule();

		module->instance = instance;

		if (mgParseString(&module->parser, string))
			_mgInstanceSet(instance, name, module);
		else
		{
			fprintf(stderr, "Error: Failed loading module \"%s\"\n", name);
			mgDestroyModule(module);
			module = NULL;
		}
	}

	MG_ASSERT(module);

	return module;
}


static inline void _mgRunModule(MGInstance *instance, MGModule *module)
{
	MG_ASSERT(instance);
	MG_ASSERT(module);
	MG_ASSERT(module->instance == instance);

	MGStackFrame frame;
	mgCreateStackFrameEx(&frame, mgReferenceValue(module->globals));

	frame.state = MG_STACK_FRAME_STATE_ACTIVE;
	frame.module = module;

	mgPushStackFrame(instance, &frame);
	mgLoadBaseLib(module);
	mgLoadMathLib(module);
	mgInterpret(module);
	mgPopStackFrame(instance, &frame);

	mgDestroyStackFrame(&frame);
}


static inline MGModule* _mgImportModuleFile(MGInstance *instance, const char *filename, const char *name)
{
	MG_ASSERT(instance);
	MG_ASSERT(filename);
	MG_ASSERT(name);

	MGModule *module = _mgInstanceGet(instance, name);

	if (module == NULL)
	{
		module = mgCreateModule();

		module->instance = instance;
		module->filename = mgStringDuplicate(filename);

		if (mgParseFile(&module->parser, filename))
		{
			_mgInstanceSet(instance, name, module);
			_mgRunModule(instance, module);
		}
		else
		{
			fprintf(stderr, "Error: Failed loading module \"%s\"\n", name);
			mgDestroyModule(module);
			module = NULL;
		}
	}

	MG_ASSERT(module);

	return module;
}


void mgRunFile(MGInstance *instance, const char *filename, const char *name)
{
	MG_ASSERT(instance);
	MG_ASSERT(filename);

	char *_name = NULL;
	_mgRunModule(instance, _mgModuleLoadFile(instance, filename, name ? name : (_name = _mgFilenameToImportName(filename))));
	free(_name);
}


void mgRunFileHandle(MGInstance *instance, FILE *file, const char *name)
{
	MG_ASSERT(instance);
	MG_ASSERT(file);
	MG_ASSERT(name);

	_mgRunModule(instance, _mgModuleLoadFileHandle(instance, file, name));
}


void mgRunString(MGInstance *instance, const char *string, const char *name)
{
	MG_ASSERT(instance);
	MG_ASSERT(string);
	MG_ASSERT(name);

	_mgRunModule(instance, _mgModuleLoadString(instance, string, name));
}


MGModule* mgImportModule(MGInstance *instance, const char *name)
{
	MG_ASSERT(instance);
	MG_ASSERT(name);

	char *filename = _mgImportNameToFilename(name);
	MGModule *module = _mgImportModuleFile(instance, filename, name);
	free(filename);

	MG_ASSERT(module);

	return module;
}
