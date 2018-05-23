
#include "format.h"
#include "debug.h"


void mgExportOBJ(MGInstance *instance, FILE *file)
{
	const size_t vertexCount = _mgListLength(instance->vertices);
	MGVertex *const vertices = _mgListItems(instance->vertices);

	MG_ASSERT(instance->vertexSize.position == 3);
	MG_ASSERT(instance->vertexSize.uv == 0);
	MG_ASSERT(instance->vertexSize.normal == 3);
	MG_ASSERT(instance->vertexSize.color == 0);

	for (size_t j = 0; j < vertexCount; ++j)
		fprintf(file, "v %f %f %f\n", vertices[j][0], vertices[j][1], vertices[j][2]);

	for (size_t j = 0; j < vertexCount; ++j)
		fprintf(file, "vn %f %f %f\n", vertices[j][3], vertices[j][4], vertices[j][5]);

	for (size_t j = 0; j < vertexCount / 3; ++j)
		fprintf(file, "f %zu//%zu %zu//%zu %zu//%zu\n",
		        j * 3 + 1, j * 3 + 1,
		        j * 3 + 2, j * 3 + 2,
		        j * 3 + 3, j * 3 + 3);
}


void mgExportTriangles(MGInstance *instance, FILE *file)
{
	const size_t vertexCount = _mgListLength(instance->vertices);
	MGVertex *const vertices = _mgListItems(instance->vertices);

	fwrite(vertices, vertexCount * sizeof(MGVertex), 1, file);
}
