
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#   include <windows.h>
#endif

#include "modelgen.h"
#include "value.h"
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

	if (_mgNullValue == NULL)
		_mgNullValue = mgCreateValueNull();
	else
		mgReferenceValue(_mgNullValue);

	_mgListCreate(char*, instance->path, 1 << 2);

	instance->modules = mgCreateValueMap(1 << 3);
	instance->staticModules = mgCreateValueMap(1 << 3);

	_mgListCreate(MGVertex, instance->vertices, 1 << 9);

#ifdef _WIN32
	char path[MG_PATH_MAX + 1];

	if (GetCurrentDirectoryA(MG_PATH_MAX + 1, path))
		_mgListAdd(char*, instance->path, mgStringDuplicate(path));

	if (GetModuleFileNameA(NULL, path, MG_PATH_MAX + 1))
	{
		size_t dirnameEnd = mgDirnameEnd(path);

		if (dirnameEnd)
		{
			path[dirnameEnd] = '\0';
			dirnameEnd = mgDirnameEnd(path);

			_mgListAdd(char*, instance->path, mgStringDuplicate(path));

			if (dirnameEnd)
			{
				strcpy(path + dirnameEnd, "/modules");

				_mgListInsert(char*, instance->path, _mgListIndexRelativeToAbsolute(instance->path, -2), mgStringDuplicate(path));
			}
		}
	}
#endif

	for (int i = 0; _mgStaticModules[i].name; ++i)
	{
		MG_ASSERT(_mgStaticModules[i].create);

		MGValue *module = _mgStaticModules[i].create();
		MG_ASSERT(module);
		MG_ASSERT(module->type == MG_TYPE_MODULE);

		module->data.module.isStatic = MG_TRUE;
		MG_ASSERT(mgMapGet(instance->staticModules, _mgStaticModules[i].name) == NULL);

		mgMapSet(instance->staticModules, _mgStaticModules[i].name, module);
	}

	instance->base = mgMapGet(instance->staticModules, "base");
}


void mgDestroyInstance(MGInstance *instance)
{
	MG_ASSERT(instance);

	if (_mgNullValue->refCount == 1)
	{
		mgDestroyValue(_mgNullValue);
		_mgNullValue = NULL;
	}
	else
		mgDestroyValue(_mgNullValue);

	for (int i = 0; i < _mgListLength(instance->path); ++i)
		free(_mgListGet(instance->path, i));
	_mgListDestroy(instance->path);

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
	MG_ASSERT(module->type == MG_TYPE_MODULE);
	MG_ASSERT(locals);

	memset(frame, 0, sizeof(MGStackFrame));

	frame->state = MG_STACK_FRAME_STATE_ACTIVE;
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


static inline void _mgRunModule(MGInstance *instance, MGValue *module)
{
	MG_ASSERT(instance);
	MG_ASSERT(module);
	MG_ASSERT(module->type == MG_TYPE_MODULE);
	MG_ASSERT(module->data.module.instance == instance);
	MG_ASSERT(module->data.module.parser.root);

	MGStackFrame frame;
	mgCreateStackFrameEx(&frame, mgReferenceValue(module), mgReferenceValue(module->data.module.globals));

	mgPushStackFrame(instance, &frame);
	mgInterpret(module);
	mgPopStackFrame(instance, &frame);

	mgDestroyStackFrame(&frame);
}


static inline MGValue* _mgImportModuleFile(MGInstance *instance, const char *name)
{
	MG_ASSERT(instance);
	MG_ASSERT(name);

	MGValue *module = mgMapGet(instance->modules, name);

	if (module == NULL)
	{
		char filename[MG_PATH_MAX + 1];
		char _name[MG_PATH_MAX + 1];

		strcpy(_name, "/");
		strcat(_name, name);
		strcat(_name, ".mg");

		for (int i = 0; i <= _mgListLength(instance->path); ++i)
		{
			if (i < _mgListLength(instance->path))
				strcat(strcpy(filename, _mgListGet(instance->path, i)), _name);
			else
				strcpy(filename, _name + 1);

			if (mgFileExists(filename))
			{
				module = mgCreateValueModule();

				module->data.module.instance = instance;
				module->data.module.filename = mgStringDuplicate(filename);

				if (mgParseFile(&module->data.module.parser, filename))
				{
					mgMapSet(instance->modules, name, module);
					_mgRunModule(instance, module);
					return module;
				}
				else
				{
					fprintf(stderr, "Error: Failed loading module \"%s\"\n", name);
					mgDestroyValue(module);
					return NULL;
				}
			}
		}

		module = mgMapGet(instance->staticModules, name);

		if (module == NULL)
		{
			fprintf(stderr, "Error: Failed loading module \"%s\"\n", name);
			mgDestroyValue(module);
			return NULL;
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

	MGValue *module = _mgImportModuleFile(instance, name);
	MG_ASSERT(module);

	return module;
}
