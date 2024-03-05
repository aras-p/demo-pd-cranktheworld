#pragma once

#include <stdint.h>
#include "../mathlib.h"
#include "shape.h"

typedef enum
{
	kRenderFilled			= (1<<0),
	kRenderWireframe		= (1<<1),
} RenderStyle;

typedef struct
{
	Matrix3D camera;
	Vector3D light;
	
	// location of the Z vanishing point on the screen. (0,0) is top left corner, (1,1) is bottom right. Defaults to (0.5,0.5)
	float centerx;
	float centery;
	
	// display scaling factor. Default is 120, so that the screen extents are (-1.66,1.66)x(-1,1)
	float scale;

	int tmp_points_cap;
	int tmp_faces_cap;
	Point3D* tmp_points;
	struct FaceInstance* tmp_faces;
	uint16_t* tmp_order_table;
} Scene3D;

void Scene3D_init(Scene3D* scene);
void Scene3D_deinit(Scene3D* scene);
void Scene3D_setCamera(Scene3D* scene, Point3D origin, Point3D lookAt, float scale, Vector3D up);
void Scene3D_setGlobalLight(Scene3D* scene, Vector3D light);
void Scene3D_setCenter(Scene3D* scene, float x, float y);

void Scene3D_drawShape(Scene3D* scene, uint8_t* buffer, int rowstride, const Shape3D* shape, const Matrix3D* matrix, RenderStyle style, float colorBias);
