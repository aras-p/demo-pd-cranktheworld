#include "fx.h"

#include "pd_api.h"
#include "../mathlib.h"
#include "../util/pixel_ops.h"

#define TEST_PRETTY_HIP 0 // "Pretty Hip" by Fabrice Neyret https://www.shadertoy.com/view/XsBfRW - 24fps at 2x2t
#define TEST_XOR_TOWERS 1 // "XOR Towers" by Greg Rostami https://www.shadertoy.com/view/7lsXR2 simplified - 10fps at 2x2t

#define CURRENT_TEST TEST_XOR_TOWERS

typedef struct TraceState
{
	float t;
	float rotmx, rotmy;
	float camdist;
} TraceState;

static int trace_ray(TraceState* st, float x, float y)
{
#if CURRENT_TEST == TEST_PRETTY_HIP
	float ux = st->rotmy * x + st->rotmx * y;
	float uy = st->rotmx * x - st->rotmy * y;
	ux = ux * 10.0f + 5.0f;
	uy = uy * 10.0f + 5.0f;
	float fx = fract(ux);
	float fy = fract(uy);
	fx = MIN(fx, 1.0f - fx);
	fy = MIN(fy, 1.0f - fy);
	float cx = ceilf(ux) - 5.5f;
	float cy = ceilf(uy) - 5.5f;
	float s = sqrtf(cx * cx + cy * cy);
	float e = 2.0f * fract((st->t - s * 0.5f) * 0.25f) - 1.0f;
	float v = fract(4.0f * MIN(fx, fy));
	float b = 0.95f * (e < 0.0f ? v : 1.0f - v) - e * e;
	float a = smoothstep(-0.05f, 0.0f, b) + s * 0.1f;

	return MIN(255, a * 250.0f);
#endif // CURRENT_TEST == TEST_PRETTY_HIP

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

	return 128;
}



static int s_frame_count = 0;

static int fx_various_test_update(uint32_t buttons_cur, uint32_t buttons_pressed, float crank_angle, float time, uint8_t* framebuffer, int framebuffer_stride)
{
	TraceState st;
	st.t = time;
#if CURRENT_TEST == TEST_PRETTY_HIP
	st.rotmx = 0.7f;//sinf(st.t / 4.0f);
	st.rotmy = 0.7f;//cosf(st.t / 4.0f);
#endif
#if CURRENT_TEST == TEST_XOR_TOWERS
	st.t = time * 0.1f;
	float r_angle = 0.6f - 0.1f * st.t;
	st.rotmx = sinf(r_angle);
	st.rotmy = cosf(r_angle);
	st.camdist = 2.0f + cosf(st.t * 1.7f);
#endif

	float xsize = 1.0f;
	float ysize = 0.6f;
	float dx = xsize / LCD_COLUMNS;
	float dy = ysize / LCD_ROWS;

	float y = ysize / 2 - dy * 0.5f;
	int t_frame_index = s_frame_count & 3;
	for (int py = 0; py < LCD_ROWS; ++py, y -= dy)
	{
		int t_row_index = py & 1;
		int col_offset = g_order_pattern_2x2[t_frame_index][t_row_index] - 1;
		if (col_offset < 0)
			continue; // this row does not evaluate any pixels

		float x = -xsize / 2 + dx * 0.5f;
		int pix_idx = py * LCD_COLUMNS;

		x += dx * col_offset;
		pix_idx += col_offset;
		for (int px = col_offset; px < LCD_COLUMNS; px += 2, x += dx * 2, pix_idx += 2)
		{
			int val = trace_ray(&st, x, y);
			g_screen_buffer[pix_idx] = val;
		}
	}
	draw_dithered_screen(framebuffer, 0);
	++s_frame_count;
	return 0;
}

Effect fx_various_test_init(void* pd_api)
{
	return (Effect) { fx_various_test_update };
}
