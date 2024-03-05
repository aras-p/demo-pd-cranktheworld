//
//  shape.h
//  Extension
//
//  Created by Dave Hayden on 10/7/15.
//  Copyright © 2015 Panic, Inc. All rights reserved.
//

#pragma once

#include "mini3d.h"
#include "../mathlib.h"

typedef struct
{
	int nPoints;
	const Point3D* points;
	int nFaces;
	const uint16_t* faces;
	const float* faceColorBias;
	Point3D center; // used for z-sorting entire shapes at a time
} Shape3D;

void Shape3D_init(Shape3D* shape, int vtxCount, const Point3D* vb, int triCount, const uint16_t* ib, const float *triColorBias);
