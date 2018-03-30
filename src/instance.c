
#include <stdio.h>
#include <string.h>

#include "modelgen.h"
#include "module.h"
#include "utilities.h"
#include "baselib.h"


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
	mgDestroyModule(&pair->value);
}


void mgCreateInstance(MGInstance *instance)
{
	MG_ASSERT(instance);

	memset(instance, 0, sizeof(MGInstance));
}


void mgDestroyInstance(MGInstance *instance)
{
	MG_ASSERT(instance);

	for (size_t i = 0; i < _mgListLength(instance->modules); ++i)
		_mgDestroyNameModule(&_mgListItems(instance->modules)[i]);
	_mgListDestroy(instance->modules);
}


static MGModule* _mgInstanceSet(MGInstance *instance, const char *name, MGModule *module)
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
		pair->value = *module;

		return &pair->value;
	}
	else if (pair)
	{
		_mgDestroyNameModule(pair);

		if ((_mgListLength(instance->modules) > 1) && (pair != &_mgListGet(instance->modules, _mgListLength(instance->modules) - 1)))
			*pair = _mgListGet(instance->modules, _mgListLength(instance->modules) - 1);

		--_mgListLength(instance->modules);
	}

	return NULL;
}


static inline MGModule* _mgInstanceGet(MGInstance *instance, const char *name)
{
	MG_ASSERT(instance);
	MG_ASSERT(name);

	for (size_t i = 0; i < _mgListLength(instance->modules); ++i)
		if (strcmp(_mgListGet(instance->modules, i).key, name) == 0)
			return &_mgListGet(instance->modules, i).value;

	return NULL;
}


static inline MGModule* _mgModuleLoadFile(MGInstance *instance, const char *filename, const char *name)
{
	MG_ASSERT(instance);
	MG_ASSERT(filename);
	MG_ASSERT(name);

	MGModule module;
	mgCreateModule(&module);

	module.instance = instance;
	module.filename = mgStringDuplicate(filename);

	mgParseFile(&module.parser, filename);
	MG_ASSERT(module.parser.root);

	return _mgInstanceSet(instance, name, &module);
}


static inline MGModule* _mgModuleLoadFileHandle(MGInstance *instance, FILE *file, const char *name)
{
	MG_ASSERT(instance);
	MG_ASSERT(file);
	MG_ASSERT(name);

	MGModule module;
	mgCreateModule(&module);

	module.instance = instance;

	mgParseFileHandle(&module.parser, file);
	MG_ASSERT(module.parser.root);

	return _mgInstanceSet(instance, name, &module);
}


static inline MGModule* _mgModuleLoadString(MGInstance *instance, const char *string, const char *name)
{
	MG_ASSERT(instance);
	MG_ASSERT(string);
	MG_ASSERT(name);

	MGModule module;
	mgCreateModule(&module);

	module.instance = instance;

	mgParseString(&module.parser, string);
	MG_ASSERT(module.parser.root);

	return _mgInstanceSet(instance, name, &module);
}


static inline void _mgRunModule(MGModule *module)
{
	MG_ASSERT(module);

	mgLoadBaseLib(module);
	mgInterpret(module);
}


static inline void _mgRunFile(MGInstance *instance, const char *filename, const char *name)
{
	MGModule *module = _mgInstanceGet(instance, name);

	if (module == NULL)
		module = _mgModuleLoadFile(instance, filename, name);

	_mgRunModule(module);
}


void mgRunFile(MGInstance *instance, const char *filename, const char *name)
{
	MG_ASSERT(instance);
	MG_ASSERT(filename);

	char *_name = NULL;
	_mgRunFile(instance, filename, name ? name : (_name = _mgFilenameToImportName(filename)));
	free(_name);
}


void mgRunFileHandle(MGInstance *instance, FILE *file, const char *name)
{
	MG_ASSERT(instance);
	MG_ASSERT(file);
	MG_ASSERT(name);

	_mgRunModule(_mgModuleLoadFileHandle(instance, file, name));
}


void mgRunString(MGInstance *instance, const char *string, const char *name)
{
	MG_ASSERT(instance);
	MG_ASSERT(string);
	MG_ASSERT(name);

	_mgRunModule(_mgModuleLoadString(instance, string, name));
}


void mgImportModule(MGInstance *instance, const char *name)
{
	MG_ASSERT(instance);
	MG_ASSERT(name);

	char *filename = _mgImportNameToFilename(name);
	_mgRunFile(instance, filename, name);
	free(filename);
}
