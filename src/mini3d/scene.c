#include <stdlib.h>
#include <string.h>
#include "../mathlib.h"
#include "mini3d.h"
#include "scene.h"
#include "shape.h"
#include "render.h"

#define WIDTH 400
#define HEIGHT 240

struct FaceInstance
{
	float3 normal;
	float midz;
};
typedef struct FaceInstance FaceInstance;

static Scene3D* s_facesort_instance;
static int compareFaceZ(const void* a, const void* b)
{
	uint16_t idxa = *(const uint16_t*)a;
	uint16_t idxb = *(const uint16_t*)b;
	float za = s_facesort_instance->tmp_faces[idxa].midz;
	float zb = s_facesort_instance->tmp_faces[idxb].midz;
	if (za < zb)
		return +1;
	if (za > zb)
		return -1;
	return 0;
}

void Scene3D_init(Scene3D* scene)
{
	Scene3D_setCamera(scene, (float3){ 0, 0, 0 }, (float3){ 0, 0, 1 }, 1.0, (float3){ 0, 1, 0 });
	Scene3D_setGlobalLight(scene, (float3){ 0, -1, 0 });

	Scene3D_setCenter(scene, 0.5, 0.5);

	scene->tmp_points_cap = scene->tmp_faces_cap = 0;
	scene->tmp_points = NULL;
	scene->tmp_faces = NULL;
	scene->tmp_order_table = NULL;
}

void Scene3D_deinit(Scene3D* scene)
{
	if (scene->tmp_points != NULL)
		m3d_free(scene->tmp_points);
	if (scene->tmp_faces != NULL)
		m3d_free(scene->tmp_faces);
	if (scene->tmp_order_table != NULL)
		m3d_free(scene->tmp_order_table);
}

void
Scene3D_setGlobalLight(Scene3D* scene, float3 light)
{
	scene->light = light;
}

void
Scene3D_setCenter(Scene3D* scene, float x, float y)
{
	scene->centerx = x;
	scene->centery = y;
}

void
Scene3D_setCamera(Scene3D* scene, float3 origin, float3 lookAt, float scale, float3 up)
{
	xform camera = mtx_identity;

	camera.x = -origin.x;
	camera.y = -origin.y;
	camera.z = -origin.z;

	float3 dir = (float3){ lookAt.x - origin.x, lookAt.y - origin.y, lookAt.z - origin.z };
	
	float l = sqrtf(v3_lensq(&dir));
	
	dir.x /= l;
	dir.y /= l;
	dir.z /= l;
	
	scene->scale = 240 * scale;

	// first yaw around the y axis

	float h = 0;

	if ( dir.x != 0 || dir.z != 0 )
	{
		h = sqrtf(dir.x * dir.x + dir.z * dir.z);
		
		xform yaw = mtx_make(dir.z/h, 0, -dir.x/h, 0, 1, 0, dir.x/h, 0, dir.z/h);
		camera = mtx_multiply(&camera, &yaw);
	}

	// then pitch up/down to y elevation

	xform pitch = mtx_make(1, 0, 0, 0, h, -dir.y, 0, dir.y, h);
	camera = mtx_multiply(&camera, &pitch);

	// and roll to position the up vector

	if ( up.x != 0 || up.y != 0 )
	{
		l = sqrtf(up.x * up.x + up.y * up.y);
		xform roll = mtx_make(up.y/l, up.x/l, 0, -up.x/l, up.y/l, 0, 0, 0, 1);
	
		scene->camera = mtx_multiply(&camera, &roll);
	}
	else
		scene->camera = camera;
}

typedef uint8_t Pattern[8];

static Pattern patterns[] =
{
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x80, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00 },
	{ 0x88, 0x00, 0x00, 0x00, 0x88, 0x00, 0x00, 0x00 },
	{ 0x88, 0x00, 0x20, 0x00, 0x88, 0x00, 0x02, 0x00 },
	{ 0x88, 0x00, 0x22, 0x00, 0x88, 0x00, 0x22, 0x00 },
	{ 0xa8, 0x00, 0x22, 0x00, 0x8a, 0x00, 0x22, 0x00 },
	{ 0xaa, 0x00, 0x22, 0x00, 0xaa, 0x00, 0x22, 0x00 },
	{ 0xaa, 0x00, 0xa2, 0x00, 0xaa, 0x00, 0x2a, 0x00 },
	{ 0xaa, 0x00, 0xaa, 0x00, 0xaa, 0x00, 0xaa, 0x00 },
	{ 0xaa, 0x40, 0xaa, 0x00, 0xaa, 0x04, 0xaa, 0x00 },
	{ 0xaa, 0x44, 0xaa, 0x00, 0xaa, 0x44, 0xaa, 0x00 },
	{ 0xaa, 0x44, 0xaa, 0x10, 0xaa, 0x44, 0xaa, 0x01 },
	{ 0xaa, 0x44, 0xaa, 0x11, 0xaa, 0x44, 0xaa, 0x11 },
	{ 0xaa, 0x54, 0xaa, 0x11, 0xaa, 0x45, 0xaa, 0x11 },
	{ 0xaa, 0x55, 0xaa, 0x11, 0xaa, 0x55, 0xaa, 0x11 },
	{ 0xaa, 0x55, 0xaa, 0x51, 0xaa, 0x55, 0xaa, 0x15 },
	{ 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55 },
	{ 0xba, 0x55, 0xaa, 0x55, 0xab, 0x55, 0xaa, 0x55 },
	{ 0xbb, 0x55, 0xaa, 0x55, 0xbb, 0x55, 0xaa, 0x55 },
	{ 0xbb, 0x55, 0xea, 0x55, 0xbb, 0x55, 0xae, 0x55 },
	{ 0xbb, 0x55, 0xee, 0x55, 0xbb, 0x55, 0xee, 0x55 },
	{ 0xfb, 0x55, 0xee, 0x55, 0xbf, 0x55, 0xee, 0x55 },
	{ 0xff, 0x55, 0xee, 0x55, 0xff, 0x55, 0xee, 0x55 },
	{ 0xff, 0x55, 0xfe, 0x55, 0xff, 0x55, 0xef, 0x55 },
	{ 0xff, 0x55, 0xff, 0x55, 0xff, 0x55, 0xff, 0x55 },
	{ 0xff, 0x55, 0xff, 0xd5, 0xff, 0x55, 0xff, 0x5d },
	{ 0xff, 0x55, 0xff, 0xdd, 0xff, 0x55, 0xff, 0xdd },
	{ 0xff, 0x75, 0xff, 0xdd, 0xff, 0x57, 0xff, 0xdd },
	{ 0xff, 0x77, 0xff, 0xdd, 0xff, 0x77, 0xff, 0xdd },
	{ 0xff, 0x77, 0xff, 0xfd, 0xff, 0x77, 0xff, 0xdf },
	{ 0xff, 0x77, 0xff, 0xff, 0xff, 0x77, 0xff, 0xff },
	{ 0xff, 0xf7, 0xff, 0xff, 0xff, 0x7f, 0xff, 0xff },
	{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }
};

static void drawShapeFace(const Scene3D* scene, uint8_t* bitmap, int rowstride, const float3 *p1, const float3 *p2, const float3 *p3, const FaceInstance* face, RenderStyle style, float colorBias)
{
	// If any vertex is behind the camera, skip it
	if (p1->z <= 0 || p2->z <= 0 || p3->z <= 0)
		return;

	float x1 = p1->x;
	float y1 = p1->y;
	float x2 = p2->x;
	float y2 = p2->y;
	float x3 = p3->x;
	float y3 = p3->y;

	// quick bounds check
	if ( (x1 < 0 && x2 < 0 && x3 < 0) ||
		 (x1 >= WIDTH && x2 >= WIDTH && x3 >= WIDTH) ||
		 (y1 < 0 && y2 < 0 && y3 < 0) ||
		 (y1 >= HEIGHT && y2 >= HEIGHT && y3 >= HEIGHT) )
		return;

	int inverted = 0; //@TODO?

	// only render front side of faces via winding order
	float d = (x2 - x1) * (y3 - y1) - (y2 - y1) * (x3 - x1);
	if ( (d >= 0) ^ (inverted ? 1 : 0) )
		return;

	// fill
	if (style & kRenderFilled)
	{
		// lighting

		float c = colorBias;
		float v;

		if (c <= -1)
			v = 0;
		else if (c >= 1)
			v = 1;
		else
		{
			if (inverted)
				v = (1.0f + v3_dot(face->normal, scene->light)) / 2;
			else
				v = (1.0f - v3_dot(face->normal, scene->light)) / 2;

			if (c > 0)
				v = c + (1 - c) * v; // map [0,1] to [c,1]
			else if (c < 0)
				v *= 1 + c; // map [0,1] to [0, 1+c]
		}

		// cheap gamma adjust
		// v = v * v;

		int vi = (int)(32.99f * v);

		if (vi > 32)
			vi = 32;
		else if (vi < 0)
			vi = 0;

		uint8_t* pattern = (uint8_t*)&patterns[vi];
		fillTriangle(bitmap, rowstride, p1, p2, p3, pattern);
	}

	// edges
	if (style & kRenderWireframe)
	{
		const uint8_t* color = patterns[0]; // 32: white, 0: black
		drawLine(bitmap, rowstride, p1, p2, 1, color);
		drawLine(bitmap, rowstride, p2, p3, 1, color);
		drawLine(bitmap, rowstride, p3, p1, 1, color);
	}

}

void Scene3D_drawShape(Scene3D* scene, uint8_t* buffer, int rowstride, const Shape3D* shape, const xform* matrix, RenderStyle style, float colorBias)
{
	// temporary buffers
	if (scene->tmp_points_cap < shape->nPoints) {
		scene->tmp_points_cap = shape->nPoints;
		scene->tmp_points = m3d_realloc(scene->tmp_points, scene->tmp_points_cap * sizeof(scene->tmp_points[0]));
	}
	if (scene->tmp_faces_cap < shape->nFaces) {
		scene->tmp_faces_cap = shape->nFaces;
		scene->tmp_faces = m3d_realloc(scene->tmp_faces, scene->tmp_faces_cap * sizeof(scene->tmp_faces[0]));
		scene->tmp_order_table = m3d_realloc(scene->tmp_order_table, scene->tmp_faces_cap * sizeof(scene->tmp_order_table[0]));
	}

	// transform points
	for (int i = 0; i < shape->nPoints; ++i)
		scene->tmp_points[i] = mtx_transform_pt(&scene->camera, mtx_transform_pt(matrix, shape->points[i]));

	// compute face normals and midpoints
	const uint16_t* ibPtr = shape->faces;
	for (int i = 0; i < shape->nFaces; ++i, ibPtr += 3)
	{
		uint16_t idx0 = ibPtr[0] - 1;
		uint16_t idx1 = ibPtr[1] - 1;
		uint16_t idx2 = ibPtr[2] - 1;
		FaceInstance* face = &scene->tmp_faces[i];
		scene->tmp_order_table[i] = i;
		face->normal = v3_tri_normal(&scene->tmp_points[idx0], &scene->tmp_points[idx1], &scene->tmp_points[idx2]);

		float z = scene->tmp_points[idx0].z + scene->tmp_points[idx1].z + scene->tmp_points[idx2].z;
		face->midz = z;
	}

	// project points to screen
	for (int i = 0; i < shape->nPoints; ++i)
	{
		float3* p = &scene->tmp_points[i];
		if (p->z > 0)
		{
			p->x = scene->scale * (p->x / p->z + 1.6666666f * scene->centerx);
			p->y = scene->scale * (p->y / p->z + scene->centery);
		}
	}

	// sort faces by z
	s_facesort_instance = scene;
	qsort(scene->tmp_order_table, shape->nFaces, sizeof(scene->tmp_order_table[0]), compareFaceZ);

	// draw faces
	for (int i = 0; i < shape->nFaces; ++i)
	{
		uint16_t fi = scene->tmp_order_table[i];
		uint16_t idx0 = shape->faces[fi * 3 + 0] - 1;
		uint16_t idx1 = shape->faces[fi * 3 + 1] - 1;
		uint16_t idx2 = shape->faces[fi * 3 + 2] - 1;
		drawShapeFace(scene, buffer, rowstride, &scene->tmp_points[idx0], &scene->tmp_points[idx1], &scene->tmp_points[idx2], &scene->tmp_faces[fi], style, colorBias); //@TODO: face bias
	}
}
