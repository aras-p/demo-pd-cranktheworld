//
//  pd3d.c
//  Playdate Simulator
//
//  Created by Dave Hayden on 8/25/15.
//  Copyright (c) 2015 Panic, Inc. All rights reserved.
//

#include <string.h>
#include "mathlib.h"

Matrix3D identityMatrix = { .m = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}, .dx = 0, .dy = 0, .dz = 0 };

Matrix3D Matrix3DMake(float m11, float m12, float m13, float m21, float m22, float m23, float m31, float m32, float m33)
{
	return (Matrix3D){ .m = {{m11, m12, m13}, {m21, m22, m23}, {m31, m32, m33}}, .dx = 0, .dy = 0, .dz = 0 };
}

Matrix3D Matrix3DMakeTranslate(float dx, float dy, float dz)
{
	return (Matrix3D){ .m = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}, .dx = dx, .dy = dy, .dz = dz };
}


Matrix3D Matrix3DMakeRotation(float angle, Vector3D axis)
{
	Matrix3D p;

#undef M_PI
#define M_PI 3.14159265358979323846f

	float c = cosf(angle * M_PI / 180);
	float s = sinf(angle * M_PI / 180);
	float x = axis.dx;
	float y = axis.dy;
	float z = axis.dz;

	float d = sqrtf(x * x + y * y + z * z);

	x /= d;
	y /= d;
	z /= d;

	p.m[0][0] = c + x * x * (1 - c);
	p.m[0][1] = x * y * (1 - c) - z * s;
	p.m[0][2] = x * z * (1 - c) + y * s;
	p.m[1][0] = y * x * (1 - c) + z * s;
	p.m[1][1] = c + y * y * (1 - c);
	p.m[1][2] = y * z * (1 - c) - x * s;
	p.m[2][0] = z * x * (1 - c) - y * s;
	p.m[2][1] = z * y * (1 - c) + x * s;
	p.m[2][2] = c + z * z * (1 - c);
	p.dx = p.dy = p.dz = 0;
	return p;
}


Matrix3D Matrix3D_multiply(Matrix3D l, Matrix3D r)
{
	Matrix3D m;
	
	m.m[0][0] = l.m[0][0] * r.m[0][0] + l.m[1][0] * r.m[0][1] + l.m[2][0] * r.m[0][2];
	m.m[1][0] = l.m[0][0] * r.m[1][0] + l.m[1][0] * r.m[1][1] + l.m[2][0] * r.m[1][2];
	m.m[2][0] = l.m[0][0] * r.m[2][0] + l.m[1][0] * r.m[2][1] + l.m[2][0] * r.m[2][2];

	m.m[0][1] = l.m[0][1] * r.m[0][0] + l.m[1][1] * r.m[0][1] + l.m[2][1] * r.m[0][2];
	m.m[1][1] = l.m[0][1] * r.m[1][0] + l.m[1][1] * r.m[1][1] + l.m[2][1] * r.m[1][2];
	m.m[2][1] = l.m[0][1] * r.m[2][0] + l.m[1][1] * r.m[2][1] + l.m[2][1] * r.m[2][2];

	m.m[0][2] = l.m[0][2] * r.m[0][0] + l.m[1][2] * r.m[0][1] + l.m[2][2] * r.m[0][2];
	m.m[1][2] = l.m[0][2] * r.m[1][0] + l.m[1][2] * r.m[1][1] + l.m[2][2] * r.m[1][2];
	m.m[2][2] = l.m[0][2] * r.m[2][0] + l.m[1][2] * r.m[2][1] + l.m[2][2] * r.m[2][2];

	m.dx = l.dx * r.m[0][0] + l.dy * r.m[0][1] + l.dz * r.m[0][2] + r.dx;
	m.dy = l.dx * r.m[1][0] + l.dy * r.m[1][1] + l.dz * r.m[1][2] + r.dy;
	m.dz = l.dx * r.m[2][0] + l.dy * r.m[2][1] + l.dz * r.m[2][2] + r.dz;
	
	return m;
}

Point3D Matrix3D_apply(const Matrix3D *m, Point3D p)
{
	float x = p.x * m->m[0][0] + p.y * m->m[0][1] + p.z * m->m[0][2] + m->dx;
	float y = p.x * m->m[1][0] + p.y * m->m[1][1] + p.z * m->m[1][2] + m->dy;
	float z = p.x * m->m[2][0] + p.y * m->m[2][1] + p.z * m->m[2][2] + m->dz;
	
	return (Point3D) { x, y, z };
}

Vector3D Vector3D_normalize(Vector3D v)
{
	float d = sqrtf(v.dx * v.dx + v.dy * v.dy + v.dz * v.dz);
	
	return (Vector3D) { v.dx / d, v.dy / d, v.dz / d };
}

Vector3D pnormal(const Point3D* p1, const Point3D* p2, const Point3D* p3)
{
	Vector3D v = Vector3DCross(
		(Vector3D) { p2->x - p1->x, p2->y - p1->y, p2->z - p1->z },
		(Vector3D) { p3->x - p1->x, p3->y - p1->y, p3->z - p1->z });

	return Vector3D_normalize(v);
}

float Matrix3D_getDeterminant(Matrix3D* m)
{
	return m->m[0][0] * m->m[1][1] * m->m[2][2]
		 + m->m[0][1] * m->m[1][2] * m->m[2][0]
		 + m->m[0][2] * m->m[1][0] * m->m[2][1]
		 - m->m[2][0] * m->m[1][1] * m->m[0][2]
		 - m->m[1][0] * m->m[0][1] * m->m[2][2]
		 - m->m[0][0] * m->m[2][1] * m->m[1][2];
}
