#pragma once

#include <stdio.h>
#include <stdint.h>
#include <math.h>

#if TARGET_PLAYDATE
static inline float _lib3d_sqrtf(float x)
{
	float result;
	asm ("vsqrt.f32 %0, %1" : "=t"(result) : "t"(x));
	return result;
}
#define sqrtf(f) _lib3d_sqrtf(f)
#endif

#if !defined(MIN)
#define MIN(a, b) (((a)<(b))?(a):(b))
#endif

#if !defined(MAX)
#define MAX(a, b) (((a)>(b))?(a):(b))
#endif

#define M_PIf (3.14159265f)

typedef struct float3
{
	float x;
	float y;
	float z;
} float3;

static inline float3 v3_cross(float3 a, float3 b)
{
	return (float3){ .x = a.y * b.z - a.z * b.y, .y = a.z * b.x - a.x * b.z, .z = a.x * b.y - a.y * b.x };
}

static inline float v3_dot(float3 a, float3 b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

float3 v3_tri_normal(const float3* p1, const float3* p2, const float3* p3);

static inline float v3_lensq(float3* v)
{
	return v->x * v->x + v->y * v->y + v->z * v->z;
}

static inline float v3_len(float3* v)
{
	return sqrtf(v3_lensq(v));
}

static inline float3 v3_add(float3 a, float3 v)
{
	return (float3) { a.x + v.x, a.y + v.y, a.z + v.z };
}

typedef struct xform
{
	float m[3][3];
	float x;
	float y;
	float z;
} xform;

extern xform mtx_identity;

xform mtx_make(float m11, float m12, float m13, float m21, float m22, float m23, float m31, float m32, float m33);
xform mtx_make_translate(float dx, float dy, float dz);
xform mtx_make_axis_angle(float angle, float3 axis);
xform mtx_multiply(const xform* l, const xform* r);
float3 mtx_transform_pt(const xform *m, float3 p);

float mtx_get_determinant(xform* m);
