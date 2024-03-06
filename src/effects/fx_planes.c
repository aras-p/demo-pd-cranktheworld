#include "fx.h"

#include <stdlib.h>

#include "pd_api.h"
#include "../mini3d/scene.h"

float3 g_mesh_Cube_vb[] = { // 28 verts
  {1.000000f, -1.000000f, 1.000000f},
  {-1.000000f, -1.000000f, -1.000000f},
  {-0.754643f, 0.700373f, 1.000000f},
  {-0.654210f, -0.654210f, 2.619101f},
  {-0.480073f, 0.111070f, 2.879520f},
  {0.480073f, 0.111070f, 2.879520f},
  {0.654210f, -0.654210f, 2.619101f},
  {-1.000000f, -1.000000f, 1.000000f},
  {0.754643f, 0.408254f, -1.529500f},
  {1.000000f, -1.000000f, -1.000000f},
  {0.754643f, 0.700373f, 1.000000f},
  {-0.754643f, 0.408254f, -1.529500f},
  {-1.283430f, -0.567080f, -1.000000f},
  {-1.283430f, -0.567080f, 1.000000f},
  {1.283430f, 0.050230f, -1.000000f},
  {1.283430f, -0.567080f, -1.000000f},
  {-1.283430f, 0.050230f, 1.000000f},
  {1.283430f, 0.050230f, 1.000000f},
  {1.283430f, -0.567080f, 1.000000f},
  {-1.283430f, 0.050230f, -1.000000f},
  {-3.291110f, -0.774914f, -1.192131f},
  {-3.291110f, -0.774914f, -0.118306f},
  {3.291110f, -0.447044f, -1.192131f},
  {3.291110f, -0.774914f, -1.192131f},
  {-3.291110f, -0.447044f, -0.118306f},
  {3.291110f, -0.447044f, -0.118306f},
  {3.291110f, -0.774914f, -0.118306f},
  {-3.291110f, -0.447044f, -1.192131f},
};
uint16_t g_mesh_Cube_ib[] = { // 52 tris
  8, 11, 2,
  8, 2, 10,
  1, 9, 0,
  1, 0, 7,
  1, 11, 8,
  1, 8, 9,
  6, 5, 4,
  6, 4, 3,
  2, 16, 13,
  2, 13, 4,
  7, 0, 6,
  7, 6, 3,
  10, 2, 4,
  10, 4, 5,
  2, 11, 19,
  2, 19, 16,
  9, 8, 14,
  9, 14, 15,
  4, 13, 7,
  4, 7, 3,
  14, 17, 25,
  14, 25, 22,
  18, 15, 23,
  18, 23, 26,
  11, 1, 12,
  11, 12, 19,
  0, 18, 5,
  0, 5, 6,
  1, 7, 13,
  1, 13, 12,
  0, 9, 15,
  0, 15, 18,
  8, 10, 17,
  8, 17, 14,
  18, 17, 10,
  18, 10, 5,
  21, 24, 27,
  21, 27, 20,
  23, 22, 25,
  23, 25, 26,
  17, 18, 26,
  17, 26, 25,
  15, 14, 22,
  15, 22, 23,
  16, 19, 27,
  16, 27, 24,
  19, 12, 20,
  19, 20, 27,
  13, 16, 24,
  13, 24, 21,
  12, 13, 21,
  12, 21, 20,
};

static Scene3D s_scene;
static Shape3D s_shape_plane;

#define MAX_PLANES 500
static int planeCount = 100;
static xform planeXforms[MAX_PLANES];
static float planeDistances[MAX_PLANES];
static int planeOrder[MAX_PLANES];

static int CompareZ(const void* a, const void* b)
{
	int ia = *(const int*)a;
	int ib = *(const int*)b;
	float za = planeDistances[ia];
	float zb = planeDistances[ib];
	if (za < zb)
		return +1;
	if (za > zb)
		return -1;
	return 0;
}


static int fx_planes_update(uint32_t buttons_cur, float crank_angle, float time, uint8_t* framebuffer, int framebuffer_stride)
{
	if (buttons_cur & kButtonLeft)
	{
		planeCount -= 1;
		if (planeCount < 1)
			planeCount = 1;
	}
	if (buttons_cur & kButtonRight)
	{
		planeCount += 1;
		if (planeCount > MAX_PLANES)
			planeCount = MAX_PLANES;
	}

	uint32_t rng = 1;

	float cangle = crank_angle * M_PIf / 180.0f;
	float cs = cosf(cangle);
	float ss = sinf(cangle);
	Scene3D_setCamera(&s_scene, (float3) { cs * 8.0f, 3.0f, ss * 8.0f }, (float3) { 0, 0, 0 }, 1.0f, (float3) { 0, -1, 0 });

	// position and sort planes
	for (int i = 0; i < planeCount; ++i)
	{
		float px = ((XorShift32(&rng) & 63) - 31.5f);
		float py = ((XorShift32(&rng) & 15) - 7.5f);
		float pz = ((XorShift32(&rng) & 63) - 31.5f);
		float rot = (XorShift32(&rng) % 360) * 1.0f;
		float rx = ((XorShift32(&rng) & 63) - 31.5f);
		float ry = 60;
		float rz = ((XorShift32(&rng) & 63) - 31.5f);
		if (i == 0) {
			px = py = pz = 0.0f;
			rot = 0;
			rx = 0;
			ry = 1;
			rz = 0;
		}
		planeXforms[i] = mtx_make_axis_angle(rot, (float3) { rx, ry, rz });
		planeXforms[i].x = px;
		planeXforms[i].y = py;
		planeXforms[i].z = pz;
		//planeXforms[i] = mtx_make_translate(px, py, pz);
		float3 center = mtx_transform_pt(&s_scene.camera, mtx_transform_pt(&planeXforms[i], s_shape_plane.center));
		planeDistances[i] = center.z;
		planeOrder[i] = i;
	}
	qsort(planeOrder, planeCount, sizeof(planeOrder[0]), CompareZ);

	// draw
	for (int i = 0; i < planeCount; ++i)
	{
		int idx = planeOrder[i];
		Scene3D_drawShape(&s_scene, framebuffer, framebuffer_stride, &s_shape_plane, &planeXforms[idx], kRenderFilled | kRenderWireframe);
	}

	return planeCount;
}

Effect fx_planes_init()
{
	Scene3D_init(&s_scene);
	Shape3D_init(&s_shape_plane, sizeof(g_mesh_Cube_vb) / sizeof(g_mesh_Cube_vb[0]), g_mesh_Cube_vb, sizeof(g_mesh_Cube_ib) / sizeof(g_mesh_Cube_ib[0]) / 3, g_mesh_Cube_ib);
	Scene3D_setGlobalLight(&s_scene, (float3) { 0.3f, 1.0f, 0.3f });
	return (Effect) {fx_planes_update};
}
