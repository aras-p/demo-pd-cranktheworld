#include "fx.h"

#include "pd_api.h"
#include "../mathlib.h"
#include "../util/pixel_ops.h"

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

typedef struct Hit
{
	float3 pos;
	float3 normal;
	float t;
} Hit;

static int hit_unit_sphere(const Ray* r, const float3* pos, float tMin, float tMax, Hit* outHit)
{
	float3 oc = v3_sub(r->orig, *pos);
	float b = v3_dot(oc, r->dir);
	float c = v3_dot(oc, oc) - 1;
	float discr = b * b - c;
	if (discr > 0)
	{
		float discrSq = sqrtf(discr);

		float t = (-b - discrSq);
		if (t <= tMin || t >= tMax)
		{
			t = (-b + discrSq);
			if (t <= tMin || t >= tMax)
				return 0;
		}
		outHit->pos = ray_pointat(r, t);
		outHit->normal = v3_sub(outHit->pos, *pos);
		outHit->t = t;
		return 1;
	}
	return 0;
}

static int hit_ground(const Ray* r, float tMin, float tMax, float3* outPos)
{
	// b = dot(plane->normal, r->dir), our normal is (0,1,0)
	float b = r->dir.y;
	if (b == 0.0f)
		return 0;

	// a = dot(plane->normal, r->orig) + plane->distance
	float a = r->orig.y;
	float t = -a / b;
	if (t < tMax && t > tMin)
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

static float3 s_Spheres[] =
{
	{2.5f,1,0},
	{0,1,0},
	{-2.5f,1,0},
};
static int s_SphereCols[] =
{
	75, 150, 225,
};
static const int kSphereCount = sizeof(s_Spheres) / sizeof(s_Spheres[0]);

const float kMinT = 0.001f;
const float kMaxT = 1.0e7f;
const int kMaxDepth = 10;

static int hit_world(const Ray* r, float tMin, float tMax, Hit* outHit, int* outID)
{
	Hit tmpHit;
	int anything = 0;
	float closest = tMax;
	for (int i = 0; i < kSphereCount; ++i)
	{
		if (hit_unit_sphere(r, &s_Spheres[i], tMin, closest, &tmpHit))
		{
			anything = 1;
			closest = tmpHit.t;
			*outHit = tmpHit;
			*outID = i;
		}
	}
	if (!anything && hit_ground(r, tMin, closest, &outHit->pos))
	{
		anything = 1;
		*outID = -1;
	}
	return anything;
}

static int trace_refl_ray(const Ray* ray)
{
	Hit rec;
	int id = 0;
	if (hit_world(ray, kMinT, kMaxT, &rec, &id))
	{
		if (id < 0) {
			int gx = (int)rec.pos.x;
			int gy = (int)rec.pos.z;
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
	Hit rec;
	int id = 0;
	if (hit_world(ray, kMinT, kMaxT, &rec, &id))
	{
		if (id < 0) {
			int gx = (int)rec.pos.x;
			int gy = (int)rec.pos.z;
			int val = ((gx ^ gy) >> 1) & 1;
			return val ? 25 : 240;
		}
		Ray refl_ray;
		refl_ray.orig = rec.pos;
		refl_ray.dir = v3_reflect(ray->dir, rec.normal);
		return (trace_refl_ray(&refl_ray) * s_SphereCols[id]) >> 8;
	}
	else
	{
		return 255;
	}
}

static int fx_raytrace_update(uint32_t buttons_cur, uint32_t buttons_pressed, float crank_angle, float time, uint8_t* framebuffer, int framebuffer_stride)
{
	float cangle = (crank_angle + 68) * M_PIf / 180.0f;
	float cs = cosf(cangle);
	float ss = sinf(cangle);
	float dist = 4.0f;
	camera_init(&s_camera, (float3) { ss * dist, 2.3f, cs * dist }, (float3) { 0, 1, 0 }, (float3) { 0, 1, 0 }, 60.0f, (float)LCD_COLUMNS / (float)LCD_ROWS, 0.1f, 3.0f);

	// trace one ray per 2x2 pixel block
	float dv = 2.0f / LCD_ROWS;
	float du = 2.0f / LCD_COLUMNS;
	Ray camRay;
	camRay.orig = s_camera.origin;

	uint8_t scanline[LCD_COLUMNS];
	float vv = 1.0f - dv * 0.5f;
	for (int py = 0; py < LCD_ROWS; py += 2, vv -= dv)
	{
		float uu = du * 0.5f;
		int prev_val = -1;

		float3 rdir_rowstart = v3_add(s_camera.lowerLeftCorner, v3_mulfl(s_camera.vertical, vv));
		rdir_rowstart = v3_sub(rdir_rowstart, s_camera.origin);

		for (int px = 0; px < LCD_COLUMNS; px += 2, uu += du)
		{
			float3 rdir = v3_add(rdir_rowstart, v3_mulfl(s_camera.horizontal, uu));
			camRay.dir = v3_normalize(rdir);

			int val = trace_ray(&camRay);

			// for the horizontal two pixels, put (average of cur+prev, cur)
			if (prev_val < 0)
				prev_val = val;
			scanline[px] = (prev_val + val) >> 1;
			scanline[px + 1] = val;
			prev_val = val;
		}

		// dither and draw two scanlines
		draw_dithered_scanline(scanline, py, 0, framebuffer);
		draw_dithered_scanline(scanline, py + 1, 0, framebuffer);
	}

	return kSphereCount;
}

Effect fx_raytrace_init(void* pd_api)
{
	return (Effect) {fx_raytrace_update};
}
