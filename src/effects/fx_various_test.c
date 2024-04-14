#include "../globals.h"

#include "pd_api.h"
#include "../mathlib.h"
#include "../util/pixel_ops.h"

#define TEST_XOR_TOWERS 1 // "XOR Towers" by Greg Rostami https://www.shadertoy.com/view/7lsXR2 simplified - 10fps at 2x2t
#define TEST_SPHERE_FIELD 2 // Somewhat based on "Raymarch 180 chars" by coyote https://www.shadertoy.com/view/llfSzH simplified - 12fps at 2x2t, 21fps at 4x2t

#define CURRENT_TEST TEST_SPHERE_FIELD

typedef struct TraceState
{
	float t;
	float rotmx, rotmy;
	float camx, camy;
	float camdist;
} TraceState;

static int trace_ray(TraceState* st, float x, float y)
{
#if CURRENT_TEST == TEST_XOR_TOWERS
	float ux = st->rotmy * x + st->rotmx * y;
	float uy = st->rotmx * x - st->rotmy * y;
	ux *= 1.666f;
	uy *= 1.666f;

	// camera motion
	float cx = 7.0f * st->t;
	float cy = 30.0f * st->t;

	int bx = 0, by = 0, bz = 0;

#define MAXSTEP 35
	float height = 0.0f;
	int it = 0;
	float heightstep = 0.3f;
	for (; it < MAXSTEP; ++it)
	{
		int tst = bx ^ by ^ bz;
		tst &= 63;
		if (tst < bz - 8)
			break;
		bx = (int)(ux * height + cx);
		by = (int)(uy * height + cy);
		bz = (int)height;
		height += heightstep;
		heightstep += 0.07f;
	}
	return (int)(height * (255.0f / 70.0f));

#endif // CURRENT_TEST == TEST_XOR_TOWERS

#if CURRENT_TEST == TEST_SPHERE_FIELD
	float ux = st->rotmy * x + st->rotmx * y;
	float uy = st->rotmx * x - st->rotmy * y;
	float3 pos = { st->camx, st->camy, st->camdist };
	float3 dir = { ux * 1.666f * 0.6f, uy * 1.666f * 0.6f, 2.0f * 0.6f };

	pos = v3_add(pos, dir);

#define MAXSTEP 15
	int it = 0;
	for (; it < MAXSTEP; ++it)
	{
		float3 rf = v3_subfl(v3_fract(pos), 0.5f);
		float d = v3_dot(rf, rf) - 0.1f;
		if (d < 0.01f)
			break;
		pos = v3_add(pos, v3_mulfl(dir, d * 1.5f));
	}
	return (int)(it * (255.0f / MAXSTEP));

#endif // CURRENT_TEST == TEST_SPHERE_FIELD


	return 128;
}



static int s_frame_count = 0;

int fx_various_test_update()
{
	TraceState st;
	st.t = G.time;
#if CURRENT_TEST == TEST_XOR_TOWERS
	st.t = G.time * 0.1f;
	float r_angle = 0.6f - 0.1f * st.t + G.crank_angle_rad;
	st.rotmx = sinf(r_angle);
	st.rotmy = cosf(r_angle);
	st.camdist = 2.0f + cosf(st.t * 1.7f);
#endif
#if CURRENT_TEST == TEST_SPHERE_FIELD
	float r_angle = 0.6f - 0.1f * st.t + G.crank_angle_rad;
	st.rotmx = sinf(r_angle);
	st.rotmy = cosf(r_angle);
	st.camx = cosf(st.t * 0.3f + 1.66f);
	st.camy = cosf(st.t * 0.23f + 1.0f);
	st.camdist = cosf(st.t * 0.11f);
#endif

	float xsize = 1.0f;
	float ysize = 0.6f;
	float dx = xsize / LCD_COLUMNS;
	float dy = ysize / LCD_ROWS;

	float y = ysize / 2 - dy * 0.5f;
	int t_frame_index = s_frame_count % 8;
	for (int py = 0; py < LCD_ROWS; ++py, y -= dy)
	{
		int t_row_index = py & 1;
		int col_offset = g_order_pattern_4x2[t_frame_index][t_row_index] - 1;
		if (col_offset < 0)
			continue; // this row does not evaluate any pixels

		float x = -xsize / 2 + dx * 0.5f;
		int pix_idx = py * LCD_COLUMNS;

		x += dx * col_offset;
		pix_idx += col_offset;
		for (int px = col_offset; px < LCD_COLUMNS; px += 4, x += dx * 4, pix_idx += 4)
		{
			int val = trace_ray(&st, x, y);
			g_screen_buffer[pix_idx] = val;
		}
	}
	draw_dithered_screen(G.framebuffer, G.beat ? 50 : 0);
	++s_frame_count;
	return 0;
}
