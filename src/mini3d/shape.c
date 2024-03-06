#include "mini3d.h"
#include "shape.h"

void Shape3D_init(Shape3D* shape, int vtxCount, const float3* vb, int triCount, const uint16_t* ib)
{
	shape->nPoints = vtxCount;
	shape->points = vb;
	shape->nFaces = triCount;
	shape->faces = ib;
	float3 bmin = { 1.0e30f, 1.0e30f, 1.0e30f };
	float3 bmax = { -1.0e30f, -1.0e30f, -1.0e30f };
	shape->center.x = 0;
	shape->center.y = 0;
	shape->center.z = 0;
	const float3* vptr = vb;
	for (int i = 0; i < vtxCount; ++i) {
		bmin.x = MIN(bmin.x, vptr->x);
		bmin.y = MIN(bmin.y, vptr->y);
		bmin.z = MIN(bmin.z, vptr->z);
		bmax.x = MAX(bmax.x, vptr->x);
		bmax.y = MAX(bmax.y, vptr->y);
		bmax.z = MAX(bmax.z, vptr->z);
		++vptr;
	}
	shape->center.x = (bmin.x + bmax.x) * 0.5f;
	shape->center.y = (bmin.y + bmax.y) * 0.5f;
	shape->center.z = (bmin.z + bmax.z) * 0.5f;
	shape->extent.x = (bmax.x - bmin.x) * 0.5f;
	shape->extent.y = (bmax.y - bmin.y) * 0.5f;
	shape->extent.z = (bmax.z - bmin.z) * 0.5f;
}
