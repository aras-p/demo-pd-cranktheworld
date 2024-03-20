#pragma once

#include "../mathlib.h"

void drawLine(uint8_t* bitmap, int rowstride, const float3* p1, const float3* p2, int thick, const uint8_t pattern[8]);
void fillTriangle(uint8_t* bitmap, int rowstride, const float3* p1, const float3* p2, const float3* p3, const uint8_t pattern[8]);
