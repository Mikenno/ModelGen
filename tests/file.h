#ifndef MODELGEN_TEST_FILE_H
#define MODELGEN_TEST_FILE_H

#include <windows.h>


typedef void (*MGWalkFilesCallback)(const char *filename);


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