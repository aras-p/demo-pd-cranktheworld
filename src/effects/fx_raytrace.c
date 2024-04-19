#include "../globals.h"

#include "pd_api.h"
#include "fx.h"
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

static bool hit_unit_sphere(const Ray* r, const float3* pos, float tMax, float* outT)
{
	float3 oc = v3_sub(r->orig, *pos);
	float b = v3_dot(oc, r->dir);
	float c = v3_dot(oc, oc) - 1.0f;
	float discr = b * b - c;
	if (discr > 0)
	{
		float discrSq = sqrtf(discr);

		float t = (-b - discrSq);
		if (t <= kMinT || t >= tMax)
		{
			// do not check the other intersection, as the ray never starts inside the sphere
			//t = (-b + discrSq);
			//if (t <= kMinT || t >= tMax)
				return false;
		}
		*outT = t;
		return true;
	}
	return false;
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
	75, 225, 150,
};

typedef struct SphereOrder {
	float dist;
	int index;
} SphereOrder;
static SphereOrder s_SphereOrder[kSphereCount];
static bool s_SphereVisible[kSphereCount];

// start time, duration, start height
static float3 kSphereBounces[kSphereCount] = {
	{6.0f, 6.0f, 8.0f},
	{22.0f, 6.0f, 8.0f},
	{14.0f, 6.0f, 8.0f},
};
// start time, duration, end distance
static float3 kSphereRoll[kSphereCount] = {
	{48.0f, 16.0f, 8.0f},
	{48.0f, 16.0f, 0.0f},
	{48.0f, 16.0f, 8.0f},
};

static float ease_roll(float x)
{
	x = saturate(x);
	return 2.7f * x * x * x - 0.3f * x * x - 0.5f * x;
}

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
		if (id < 0)
		{
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
		if (id != 1)
		{
			return s_SphereCols[id];
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

static void do_render(float crank_angle, float time, float start_time, float end_time, float alpha, uint8_t* framebuffer, int framebuffer_stride)
{
	float cangle = crank_angle + ((68 + time * 6.0f) * (G.ending ? 0.2f : 1.0f)) * (M_PIf / 180.0f);
	float cs = cosf(cangle);
	float ss = sinf(cangle);
	float cam_dist = 4.0f;
	camera_init(&s_camera, (float3) { ss * cam_dist, 2.3f, cs * cam_dist },
		(float3) { 0, 1, 0 }, (float3) { 0, 1, 0 }, 60.0f, (float)LCD_COLUMNS / (float)LCD_ROWS, 0.1f, 3.0f);

	// animate spheres
	for (int i = 0; i < kSphereCount; ++i)
	{
		float3 sp = s_SpheresOrig[i];
		if (G.ending)
		{
			s_SphereVisible[i] = true;
			if (i == 0)
			{
				float sphangle = time * 0.15f;
				float sph_cs = cosf(sphangle);
				float sph_ss = sinf(sphangle);
				float dist = 2.1f;
				sp.x = sph_cs * dist;
				sp.z = sph_ss * dist;
			}
			if (i == 2)
			{
				float sphangle = time * (-0.07f);
				float sph_cs = cosf(sphangle);
				float sph_ss = sinf(sphangle);
				float dist = 4.2f;
				sp.x = sph_cs * dist;
				sp.z = sph_ss * dist;
			}
		}
		else
		{
			s_SphereVisible[i] = time > kSphereBounces[i].x;
			float a_bounce = 1.0f - BounceEaseOut((time - kSphereBounces[i].x) / kSphereBounces[i].y);
			float a_roll = ease_roll((time - kSphereRoll[i].x) / kSphereRoll[i].y);
			sp.x = sp.x + a_roll * kSphereRoll[i].z * (signbit(sp.x) ? -1.0f : 1.0f);
			sp.y = 1.0f + a_bounce * kSphereBounces[i].z;
		}
		s_SpheresPos[i] = sp;
	}
	// sort spheres by distance from camera, for primary rays
	for (int i = 0; i < kSphereCount; ++i)
	{
		s_SphereOrder[i].index = i;
		float3 vec = v3_sub(s_SpheresPos[i], s_camera.origin);
		s_SphereOrder[i].dist = v3_lensq(&vec);
	}
	qsort(s_SphereOrder, kSphereCount, sizeof(s_SphereOrder[0]), CompareSphereDist);

	Ray camRay;
	camRay.orig = s_camera.origin;
	// 3x2 block temporal update one pixel per frame
	float dv = 1.0f / LCD_ROWS;
	float du = 1.0f / LCD_COLUMNS;
	float vv = 1.0f - dv * 0.5f;
	int t_frame_index = G.frame_count % 6;
	for (int py = 0; py < LCD_ROWS; ++py, vv -= dv)
	{
		int t_row_index = py & 1;
		int col_offset = g_order_pattern_3x2[t_frame_index][t_row_index] - 1;
		if (col_offset < 0)
			continue; // this row does not evaluate any pixels

		float uu = du * 0.5f;
		float3 rdir_rowstart = v3_add(s_camera.lowerLeftCorner, v3_mulfl(s_camera.vertical, vv));
		rdir_rowstart = v3_sub(rdir_rowstart, s_camera.origin);

		int pix_idx = py * LCD_COLUMNS;

		uu += du * col_offset;
		pix_idx += col_offset;
		for (int px = col_offset; px < LCD_COLUMNS; px += 3, uu += du * 3, pix_idx += 3)
		{
			float3 rdir = v3_add(rdir_rowstart, v3_mulfl(s_camera.horizontal, uu));
			camRay.dir = v3_normalize(rdir);

			int val = trace_ray(&camRay);
			g_screen_buffer[pix_idx] = val;
		}
	}
	draw_dithered_screen(framebuffer, get_fade_bias(start_time, end_time));
}

void fx_raytrace_update(float start_time, float end_time, float alpha)
{
	do_render(G.crank_angle_rad, G.ending ? G.time : G.time - start_time, start_time, end_time, alpha, G.framebuffer, G.framebuffer_stride);
}

void fx_raytrace_init()
{
	s_LightDir = v3_normalize((float3) { 0.8f, 1.0f, 0.6f });
}
