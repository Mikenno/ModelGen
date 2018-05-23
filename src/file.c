
#include "file.h"
#include "utilities.h"


#define _MG_STREAM_BUFFER_LENGTH 1024


int mgFileExists(const char *filename)
{
#ifdef _WIN32
	DWORD dwAttrib = GetFileAttributesA(filename);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES)
	       && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
#else
	return access(filename, F_OK) != -1;
#endif
}


const char* mgBasename(const char *filename)
{
	const char* basename1 = strrchr(filename, '/');
	const char* basename2 = strrchr(filename, '\\');

	return ((basename1 != basename2) ? ((basename1 > basename2) ? (basename1 + 1) : (basename2 + 1)) : filename);
}


size_t mgDirnameEnd(const char *filename)
{
	const char *slash = strrchr(filename, '/');
	const char *backslash = strrchr(filename, '\\');

	if (slash && backslash && (backslash > slash))
		slash = backslash;
	else if (!slash)
		slash = backslash;

	return slash ? (slash - filename) : 0;
}


char* mgDirname(char *dirname, const char *filename)
{
	const size_t end = mgDirnameEnd(filename);

	if (end)
		strncpy(dirname, filename, end);

	dirname[end] = '\0';

	return dirname;
}


char* mgReadFile(const char *filename, size_t *length)
{
	FILE *f = fopen(filename, "r");

	if (!f)
		return NULL;

	char *str = mgReadFileHandle(f, length);

	fclose(f);

	return str;
}


char* mgReadFileHandle(FILE *file, size_t *length)
{
	fseek(file, 0, SEEK_END);
	const size_t len = (size_t) ftell(file);
	fseek(file, 0, SEEK_SET);

	char *str = NULL;

	if (len >= 0)
	{
		str = (char*) malloc((len + 1) * sizeof(char));

		if (!str)
			return NULL;

		size_t read = 0;
		if (len > 0)
			read = fread(str, sizeof(char), len, file);

		str[read] = '\0';

		if (length)
			*length = read;
	}
	else if (len < 0)
	{
		char buffer[_MG_STREAM_BUFFER_LENGTH];

		size_t strLen = 0;
		str = (char*) malloc(sizeof(char));

		if (!str)
			return NULL;

		char *strTemp;

		// TODO: To avoid a lot of reallocating, allocate a bigger buffer and realloc it smaller after

		while (!feof(file) && !ferror(file))
		{
			size_t read = fread(buffer, sizeof(char), _MG_STREAM_BUFFER_LENGTH, file);

			strLen += read;
			strTemp = (char*) realloc(str, (strLen + 1) * sizeof(char));

			if (!strTemp)
			{
				free(str);
				return NULL;
			}

			str = strTemp;

			strncpy(str + strLen - read, buffer, read);
		}

		str[strLen] = '\0';

		if (length)
			*length = strLen;
	}

	return str;
}


#ifdef _WIN32

int mgWalkFiles(const char *directory, MGWalkFilesCallback callback)
{
	char dir[MG_PATH_MAX + 1], filename[MG_PATH_MAX + 1];
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
