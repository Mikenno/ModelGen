
#include <string.h>

#include "primitive.h"
#include "utilities.h"
#include "debug.h"


MGValue* mgCreateValueInteger(int i)
{
	MGValue *value = mgCreateValue(MG_TYPE_INTEGER);
	value->data.i = i;
	return value;
}


MGValue* mgCreateValueFloat(float f)
{
	MGValue *value = mgCreateValue(MG_TYPE_FLOAT);
	value->data.f = f;
	return value;
}


MGValue* mgCreateValueStringEx(const char *s, MGStringUsage usage)
{
	MGValue *value = mgCreateValue(MG_TYPE_STRING);
	value->data.str.s = NULL;
	mgStringSetEx(value, s, usage);
	return value;
}


void mgStringSetEx(MGValue *value, const char *s, MGStringUsage usage)
{
	MG_ASSERT(s);

	if (value->data.str.usage != MG_STRING_USAGE_STATIC)
		free(value->data.str.s);

	value->data.str.s = (usage == MG_STRING_USAGE_COPY) ? mgStringDuplicate(s) : (char*) s;
	value->data.str.length = strlen(value->data.str.s);
	value->data.str.usage = usage;
}
