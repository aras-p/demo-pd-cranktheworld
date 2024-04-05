#include "fx.h"

#include "pd_api.h"
#include "../mathlib.h"
#include "../util/pixel_ops.h"
#include <stdbool.h>

// somewhat based on https://www.shadertoy.com/view/ldyGWm

#define MAX_TRACE_STEPS 8
#define FAR_DIST 5.0f

typedef struct TraceState
{
	float t;
	float3 pos;
} TraceState;

static float scene_sdf(float3 q)
{
	// Layer one. The ".05" on the end varies the hole size.
	// p = abs(fract(q / 3) * 3 - 1.5)
	float3 p = v3_fract(v3_mulfl(q, 0.333333f));
	p = v3_abs(v3_subfl(v3_mulfl(p, 3.0f), 1.5f));
	float d = MIN(MAX(p.x, p.y), MIN(MAX(p.y, p.z), MAX(p.x, p.z))) - 1.0f + 0.05f;

	// Layer two
	p = v3_abs(v3_subfl(v3_fract(q), 0.5f));
	d = MAX(d, MIN(MAX(p.x, p.y), MIN(MAX(p.y, p.z), MAX(p.x, p.z))) - 1.0f / 3.0f + 0.05f);

	return d;
}

static int trace_ray(const TraceState* st, float x, float y)
{
	float3 pos = st->pos;
	float3 dir = { x, y, 1.0f };
	dir = v3_normalize(dir);
	//dir = (float3){ dir.y, dir.z * st->cos_a - dir.x * st->sin_a, dir.x * st->cos_a + dir.z * st->sin_a };
	//dir = (float3){ dir.y, dir.z * st->cos_a - dir.x * st->sin_a, dir.x * st->cos_a + dir.z * st->sin_a };
	//dir = (float3){ dir.y, dir.z * st->cos_a - dir.x * st->sin_a, dir.x * st->cos_a + dir.z * st->sin_a };

	// Rotate the ray
	//vec2 m = sin(vec2(0, 1.57079632) + iTime / 4.);
	//rd.xy = mat2(m.y, -m.x, m)*rd.xy;
	//rd.xz = mat2(m.y, -m.x, m)*rd.xz;

	float t = 0.0f;
	for (int i = 0; i < MAX_TRACE_STEPS; ++i)
	{
		float3 q = v3_add(pos, v3_mulfl(dir, t));
		float d = scene_sdf(q);
		if (d < t * 0.0025f || d > FAR_DIST)
			break;
		t += d;
	}
	//return (x) * 255;
	return MIN((int)(t * 0.3f * 255.0f), 255);
}

static void do_render(float crank_angle, float time, uint8_t* framebuffer, int framebuffer_stride)
{
	TraceState st;
	st.t = time * 30.0f;
	st.pos.x = 0.0f;
	st.pos.y = 0.0f;
	st.pos.z = time;

	// trace one ray per 2x2 pixel block
	// x: -1.67 .. +1.67
	// y: -1.0 .. +1.0
	float xext = 1.6667f;
	float yext = 1.0f;
	float dx = xext * 4 / LCD_COLUMNS;
	float dy = yext * 4 / LCD_ROWS;

	float y = yext - dy * 0.5f;
	int pix_idx = 0;
	for (int py = 0; py < LCD_ROWS / 2; ++py, y -= dy)
	{
		float x = -xext + dx * 0.5f;

		for (int px = 0; px < LCD_COLUMNS / 2; ++px, x += dx, ++pix_idx)
		{
			int val = trace_ray(&st, x, y);
			g_screen_buffer[pix_idx] = val;
		}
	}

	draw_dithered_screen_2x2(framebuffer, 1);
}


static int fx_sponge_update(uint32_t buttons_cur, uint32_t buttons_pressed, float crank_angle, float time, uint8_t* framebuffer, int framebuffer_stride)
{
	do_render(crank_angle, time, framebuffer, framebuffer_stride);
	return MAX_TRACE_STEPS;
}

Effect fx_sponge_init(void* pd_api)
{
	return (Effect) { fx_sponge_update };
}
