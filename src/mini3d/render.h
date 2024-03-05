//
//  render.h
//  Extension
//
//  Created by Dave Hayden on 10/20/15.
//  Copyright © 2015 Panic, Inc. All rights reserved.
//

#pragma once

#include <stdio.h>
#include "../mathlib.h"
#include "mini3d.h"

void drawLine(uint8_t* bitmap, int rowstride, const Point3D* p1, const Point3D* p2, int thick, const uint8_t pattern[8]);
void fillTriangle(uint8_t* bitmap, int rowstride, const Point3D* p1, const Point3D* p2, const Point3D* p3, const uint8_t pattern[8]);
