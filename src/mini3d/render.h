//
//  render.h
//  Extension
//
//  Created by Dave Hayden on 10/20/15.
//  Copyright © 2015 Panic, Inc. All rights reserved.
//

#pragma once

#include <stdio.h>
#include "3dmath.h"
#include "mini3d.h"

typedef struct
{
	int16_t start;
	int16_t end;
} LCDRowRange;

LCDRowRange drawLine(uint8_t* bitmap, int rowstride, const Point3D* p1, const Point3D* p2, int thick, const uint8_t pattern[8]);
LCDRowRange fillTriangle(uint8_t* bitmap, int rowstride, const Point3D* p1, const Point3D* p2, const Point3D* p3, const uint8_t pattern[8]);
