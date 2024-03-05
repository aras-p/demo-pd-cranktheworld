//
//  shape.c
//  Extension
//
//  Created by Dave Hayden on 10/7/15.
//  Copyright © 2015 Panic, Inc. All rights reserved.
//

#include "mini3d.h"
#include "shape.h"

void Shape3D_init(Shape3D* shape, int vtxCount, const Point3D* vb, int triCount, const uint16_t* ib, const float* triColorBias)
{
	shape->nPoints = vtxCount;
	shape->points = vb;
	shape->nFaces = triCount;
	shape->faces = ib;
	shape->faceColorBias = triColorBias;
	shape->center.x = 0;
	shape->center.y = 0;
	shape->center.z = 0;
	const Point3D* vptr = vb;
	for (int i = 0; i < vtxCount; ++i) {
		shape->center.x += vptr->x;
		shape->center.y += vptr->y;
		shape->center.z += vptr->z;
		++vptr;
	}
	shape->center.x /= vtxCount;
	shape->center.y /= vtxCount;
	shape->center.z /= vtxCount;
	shape->colorBias = 0;
}
