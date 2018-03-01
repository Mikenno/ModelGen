#ifndef MODELGEN_UTILITIES_H
#define MODELGEN_UTILITIES_H

const char* mgBasename(const char *filename);

unsigned int mgInlineRepresentationLength(const char *str, const char *end);
char* mgInlineRepresentation(char *destination, const char *source, const char *end);

#endif