#include "fx.h"

#include "pd_api.h"
#include "../mathlib.h"
#include "../util/pixel_ops.h"
#include "../external/aheasing/easing.h"

#include <stdlib.h>

#define kMinT (0.001f)
#define kMaxT (1.0e3f)

typedef struct Ray {
	float3 orig;
	float3 dir;
} Ray;

static float3 ray_pointat(const Ray* r, float t)
{
	return v3_add(r->orig, v3_mulfl(r->dir, t));
}

typedef struct Camera {
	float3 origin;
	float3 lowerLeftCorner;
	float3 horizontal;
	float3 vertical;
	float3 u, v, w;
} Camera;

static void camera_init(Camera* cam, float3 lookFrom, float3 lookAt, float3 vup, float vfov, float aspect, float aperture, float focusDist)
{
	float theta = vfov * M_PIf / 180.0f;
	float halfHeight = tanf(theta / 2);
	float halfWidth = aspect * halfHeight;
	cam->origin = lookFrom;
	cam->w = v3_normalize(v3_sub(lookFrom, lookAt));
	cam->u = v3_normalize(v3_cross(vup, cam->w));
	cam->v = v3_cross(cam->w, cam->u);
	cam->lowerLeftCorner = cam->origin;
	cam->lowerLeftCorner = v3_sub(cam->lowerLeftCorner, v3_mulfl(cam->u, halfWidth * focusDist));
	cam->lowerLeftCorner = v3_sub(cam->lowerLeftCorner, v3_mulfl(cam->v, halfHeight * focusDist));
	cam->lowerLeftCorner = v3_sub(cam->lowerLeftCorner, v3_mulfl(cam->w, focusDist));

	cam->horizontal = v3_mulfl(cam->u, 2 * halfWidth * focusDist);
	cam->vertical = v3_mulfl(cam->v, 2 * halfHeight * focusDist);
}

static int hit_unit_sphere(const Ray* r, const float3* pos, float tMax, float* outT)
{
	float3 oc = v3_sub(r->orig, *pos);
	float b = v3_dot(oc, r->dir);
	float c = v3_dot(oc, oc) - 1;
	float discr = b * b - c;
	if (discr > 0)
	{
		float discrSq = sqrtf(discr);

		float t = (-b - discrSq);
		if (t <= kMinT || t >= tMax)
		{
			t = (-b + discrSq);
			if (t <= kMinT || t >= tMax)
				return 0;
		}
		*outT = t;
		return 1;
	}
	return 0;
}

static int hit_ground(const Ray* r, float tMax, float3* outPos)
{
	// b = dot(plane->normal, r->dir), our normal is (0,1,0)
	float b = r->dir.y;
	if (b >= 0.0f)
		return 0;

	// a = dot(plane->normal, r->orig) + plane->distance
	float a = r->orig.y;
	float t = -a / b;
	if (t < tMax && t > kMinT)
	{
		float3 pos = ray_pointat(r, t);
		if (fabsf(pos.x) < 20.0f && fabsf(pos.z) < 20.0f)
		{
			*outPos = pos;
			return 1;
		}
	}
	return 0;
}

static Camera s_camera;

static float3 s_SpheresOrig[] =
{
	{2.5f,1,0},
	{0,1,0},
	{-2.5f,1,0},
};
#define kSphereCount (sizeof(s_SpheresOrig) / sizeof(s_SpheresOrig[0]))
static float3 s_SpheresPos[kSphereCount];
static int s_SphereCols[kSphereCount] =
{
	75, 150, 225,
};

typedef struct SphereOrder {
	float dist;
	int index;
} SphereOrder;
static SphereOrder s_SphereOrder[kSphereCount];
static char s_SphereVisible[kSphereCount];

// start time, bounce time, bounce height
static float3 kSphereBounces[kSphereCount] = {
	{6.0f, 3.0f, 3.0f},
	{3.0f, 3.0f, 3.0f},
	{9.0f, 3.0f, 3.0f},
};

static float3 s_LightDir;

static bool shadow_ray(const Ray* r)
{
	for (int i = 0; i < kSphereCount; ++i)
	{
		if (!s_SphereVisible[i])
			continue;
		float t;
		if (hit_unit_sphere(r, &s_SpheresPos[i], kMaxT, &t))
		{
			return true;
		}
	}
	return false;
}

static int hit_world_refl(const Ray* r, float* outSphereT, float3 *outGroundPos, int* outID, int skip_sphere)
{
	float t;
	int anything = 0;
	float closest = kMaxT;
	for (int i = 0; i < kSphereCount; ++i)
	{
		if (i == skip_sphere || !s_SphereVisible[i])
			continue;
		if (hit_unit_sphere(r, &s_SpheresPos[i], closest, &t))
		{
			anything = 1;
			closest = t;
			*outSphereT = t;
			*outID = i;
		}
	}
	if (!anything && hit_ground(r, closest, outGroundPos))
	{
		anything = 1;
		*outID = -1;
	}
	return anything;
}

static int hit_world_primary(const Ray* r, float* outSphereT, float3* outGroundPos, int* outID)
{
	for (int ii = 0; ii < kSphereCount; ++ii)
	{
		int si = s_SphereOrder[ii].index;
		if (!s_SphereVisible[si])
			continue;
		float t;
		if (hit_unit_sphere(r, &s_SpheresPos[si], kMaxT, &t))
		{
			float hitY = r->orig.y + r->dir.y * t;
			if (hitY > 0.0f) // otherwise it would be below ground
			{
				*outID = si;
				*outSphereT = t;
				return 1;
			}
		}
	}
	if (hit_ground(r, kMaxT, outGroundPos))
	{
		*outID = -1;
		return 1;
	}
	return 0;
}

static int trace_refl_ray(const Ray* ray, int parent_id)
{
	float sphereT;
	float3 groundPos;
	int id = 0;
	if (hit_world_refl(ray, &sphereT, &groundPos, &id, parent_id))
	{
		if (id < 0) {
			int gx = (int)groundPos.x;
			int gy = (int)groundPos.z;
			int val = (gx ^ gy) >> 1;
			return val & 1 ? 25 : 240;
		}
		return s_SphereCols[id];
	}
	else
	{
		return 255;
	}
}

static int trace_ray(const Ray* ray)
{
	float sphereT;
	float3 groundPos;
	int id = 0;
	if (hit_world_primary(ray, &sphereT, &groundPos, &id))
	{
		if (id < 0) {
			int gx = (int)groundPos.x;
			int gy = (int)groundPos.z;
			int val = ((gx ^ gy) >> 1) & 1;
			int baseCol = val ? 50 : 240;

			Ray sray;
			sray.orig = groundPos;
			sray.dir = s_LightDir;
			if (shadow_ray(&sray))
			{
				baseCol /= 4;
			}

			return baseCol;
		}
		Ray refl_ray;
		float3 pos = ray_pointat(ray, sphereT);
		float3 nor = v3_sub(pos, s_SpheresPos[id]);
		refl_ray.orig = pos;
		refl_ray.dir = v3_reflect(ray->dir, nor);
		return (trace_refl_ray(&refl_ray, id) * s_SphereCols[id]) >> 8;
	}
	else
	{
		return 255;
	}
}

static int CompareSphereDist(const void* a, const void* b)
{
	const SphereOrder* oa = (const SphereOrder*)a;
	const SphereOrder* ob = (const SphereOrder*)b;
	if (oa->dist < ob->dist)
		return -1;
	if (oa->dist > ob->dist)
		return 1;
	return 0;
}

static int s_frame_count = 0;
static int s_temporal_mode = 1;
#define TEMPORAL_MODE_COUNT 3


static void do_render(float crank_angle, float time, uint8_t* framebuffer, int framebuffer_stride)
{
	time = 24; // debug

	float cangle = (crank_angle + 68 + time * 20) * M_PIf / 180.0f;
	float cs = cosf(cangle);
	float ss = sinf(cangle);
	float dist = 4.0f;
	camera_init(&s_camera, (float3) { ss* dist, 2.3f, cs* dist }, (float3) { 0, 1, 0 }, (float3) { 0, 1, 0 }, 60.0f, (float)LCD_COLUMNS / (float)LCD_ROWS, 0.1f, 3.0f);


	// sort spheres by distance from camera, for primary rays
	for (int i = 0; i < kSphereCount; ++i)
	{
		// animate spheres
		float3 sp = s_SpheresOrig[i];
		s_SphereVisible[i] = time > kSphereBounces[i].x;
		sp.y = 1.0f + kSphereBounces[i].z - BounceEaseOut((time - kSphereBounces[i].x) / kSphereBounces[i].y) * kSphereBounces[i].z;
		s_SpheresPos[i] = sp;

		s_SphereOrder[i].index = i;
		float3 vec = v3_sub(sp, s_camera.origin);
		s_SphereOrder[i].dist = v3_lensq(&vec);
	}
	qsort(s_SphereOrder, kSphereCount, sizeof(s_SphereOrder[0]), CompareSphereDist);

	Ray camRay;
	camRay.orig = s_camera.origin;
	if (s_temporal_mode == 0)
	{
		// trace one ray per 2x2 pixel block
		float dv = 2.0f / LCD_ROWS;
		float du = 2.0f / LCD_COLUMNS;

		float vv = 1.0f - dv * 0.5f;
		int pix_idx = 0;
		for (int py = 0; py < LCD_ROWS / 2; ++py, vv -= dv)
		{
			float uu = du * 0.5f;

			float3 rdir_rowstart = v3_add(s_camera.lowerLeftCorner, v3_mulfl(s_camera.vertical, vv));
			rdir_rowstart = v3_sub(rdir_rowstart, s_camera.origin);

			for (int px = 0; px < LCD_COLUMNS / 2; ++px, uu += du, ++pix_idx)
			{
				float3 rdir = v3_add(rdir_rowstart, v3_mulfl(s_camera.horizontal, uu));
				camRay.dir = v3_normalize(rdir);

				int val = trace_ray(&camRay);
				g_screen_buffer[pix_idx] = val;
			}
		}
		draw_dithered_screen_2x2(framebuffer, 1);
	}
	if (s_temporal_mode == 1)
	{
		// 2x2 block temporal update one pixel per frame
		float dv = 1.0f / LCD_ROWS;
		float du = 1.0f / LCD_COLUMNS;
		float vv = 1.0f - dv * 0.5f;
		int t_frame_index = s_frame_count & 3;
		for (int py = 0; py < LCD_ROWS; ++py, vv -= dv)
		{
			int t_row_index = py & 1;
			int col_offset = g_order_pattern_2x2[t_frame_index][t_row_index] - 1;
			if (col_offset < 0)
				continue; // this row does not evaluate any pixels

			float uu = du * 0.5f;
			float3 rdir_rowstart = v3_add(s_camera.lowerLeftCorner, v3_mulfl(s_camera.vertical, vv));
			rdir_rowstart = v3_sub(rdir_rowstart, s_camera.origin);

			int pix_idx = py * LCD_COLUMNS;

			uu += du * col_offset;
			pix_idx += col_offset;
			for (int px = col_offset; px < LCD_COLUMNS; px += 2, uu += du * 2, pix_idx += 2)
			{
				float3 rdir = v3_add(rdir_rowstart, v3_mulfl(s_camera.horizontal, uu));
				camRay.dir = v3_normalize(rdir);

				int val = trace_ray(&camRay);
				g_screen_buffer[pix_idx] = val;
			}
		}
		draw_dithered_screen(framebuffer, 0);
	}
	if (s_temporal_mode == 2)
	{
		// 4x2 block temporal update one pixel per frame
		float dv = 1.0f / LCD_ROWS;
		float du = 1.0f / LCD_COLUMNS;
		float vv = 1.0f - dv * 0.5f;
		int t_frame_index = s_frame_count & 7;
		for (int py = 0; py < LCD_ROWS; ++py, vv -= dv)
		{
			int t_row_index = py & 1;
			int col_offset = g_order_pattern_4x2[t_frame_index][t_row_index] - 1;
			if (col_offset < 0)
				continue; // this row does not evaluate any pixels

			float uu = du * 0.5f;
			float3 rdir_rowstart = v3_add(s_camera.lowerLeftCorner, v3_mulfl(s_camera.vertical, vv));
			rdir_rowstart = v3_sub(rdir_rowstart, s_camera.origin);

			int pix_idx = py * LCD_COLUMNS;

			uu += du * col_offset;
			pix_idx += col_offset;
			for (int px = col_offset; px < LCD_COLUMNS; px += 4, uu += du * 4, pix_idx += 4)
			{
				float3 rdir = v3_add(rdir_rowstart, v3_mulfl(s_camera.horizontal, uu));
				camRay.dir = v3_normalize(rdir);

				int val = trace_ray(&camRay);
				g_screen_buffer[pix_idx] = val;
			}
		}
		draw_dithered_screen(framebuffer, 0);
	}

	++s_frame_count;
}


static int fx_raytrace_update(uint32_t buttons_cur, uint32_t buttons_pressed, float crank_angle, float time, uint8_t* framebuffer, int framebuffer_stride)
{
	if (buttons_pressed & kButtonLeft)
	{
		s_temporal_mode--;
		if (s_temporal_mode < 0)
			s_temporal_mode = TEMPORAL_MODE_COUNT - 1;
	}
	if (buttons_pressed & kButtonRight)
	{
		s_temporal_mode++;
		if (s_temporal_mode >= TEMPORAL_MODE_COUNT)
			s_temporal_mode = 0;
	}

	do_render(crank_angle, time, framebuffer, framebuffer_stride);

	return s_temporal_mode;
}

Effect fx_raytrace_init(void* pd_api)
{
	s_LightDir = v3_normalize((float3) { 0.8f, 1.0f, 0.6f });
	return (Effect) {fx_raytrace_update};
}
