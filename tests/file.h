#ifndef MODELGEN_TESTS_FILE_H
#define MODELGEN_TESTS_FILE_H

#include <windows.h>


typedef void (*MGWalkFilesCallback)(const char *filename);


static inline int mgStringEndsWith(const char *string, const char *suffix)
{
	size_t stringLength = strlen(string);
	size_t suffixLength = strlen(suffix);

	return (stringLength >= suffixLength)
	    && (memcmp(suffix, string + (stringLength - suffixLength), suffixLength) == 0);
}


static inline int mgFileExists(const char *filename)
{
	DWORD dwAttrib = GetFileAttributesA(filename);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES)
	   && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
}


static size_t mgDirnameEnd(const char *filename)
{
	const char *slash = strrchr(filename, '/');
	const char *backslash = strrchr(filename, '\\');

	if (slash && backslash && (backslash > slash))
		slash = backslash;
	else if (!slash)
		slash = backslash;

	return slash ? (slash - filename) : 0;
}


static char* mgDirname(char *dirname, const char *filename)
{
	const size_t end = mgDirnameEnd(filename);

	if (end)
		strncpy(dirname, filename, end);

	dirname[end] = '\0';

	return dirname;
}


static int mgWalkFiles(const char *directory, MGWalkFilesCallback callback)
{
	char dir[MAX_PATH + 1], filename[MAX_PATH + 1];
	WIN32_FIND_DATAA find;
	HANDLE hFind;

	const int addSlash = !mgStringEndsWith(directory, "/") && !mgStringEndsWith(directory, "\\");

	strcpy(dir, directory);
	if (addSlash)
		strcat(dir, "/*");
	else
		strcat(dir, "*");

	if ((hFind = FindFirstFileA(dir, &find)) == INVALID_HANDLE_VALUE)
	{
		printf("No such directory \"%s\"\n", directory);
		return 1;
	}

	do
	{
		if (!strcmp(find.cFileName, ".") || !strcmp(find.cFileName, ".."))
			continue;

		strcpy(filename, directory);
		if (addSlash)
			strcat(filename, "/");
		strcat(filename, find.cFileName);

		if (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			mgWalkFiles(filename, callback);
		else
			callback(filename);
	}
	while (FindNextFileA(hFind, &find));

	FindClose(hFind);

	return 0;
}

#endif