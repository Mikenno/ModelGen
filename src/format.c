
#include "format.h"


void mgExportOBJ(MGInstance *instance, FILE *file)
{
	const size_t vertexCount = _mgListLength(instance->vertices);

	MGVertex *const vertices = _mgListItems(instance->vertices);

	for (size_t j = 0; j < vertexCount; ++j)
		fprintf(file, "v %f %f %f\n", vertices[j][0], vertices[j][1], vertices[j][2]);

	for (size_t j = 0; j < vertexCount / 3; ++j)
		fprintf(file, "f %zu %zu %zu\n", j * 3 + 1, j * 3 + 2, j * 3 + 3);
}
