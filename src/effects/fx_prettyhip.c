// SPDX-License-Identifier: Unlicense

#include "../globals.h"

#include "../platform.h"
#include "fx.h"
#include "../mathlib.h"
#include "../util/pixel_ops.h"
#include <string.h>

// Background: loosely based on "Pretty Hip" by Fabrice Neyret https://www.shadertoy.com/view/XsBfRW
// Foreground: traditional "Kefren Bars"

#define MAX_LOOKUP_EXTENT (12)

static float s_uxuy_lookup[MAX_LOOKUP_EXTENT][MAX_LOOKUP_EXTENT];

void fx_prettyhip_init()
{
	for (int iy = 0; iy < MAX_LOOKUP_EXTENT; ++iy)
	{
		for (int ix = 0; ix < MAX_LOOKUP_EXTENT; ++ix)
		{
			float cx = (float)ix - 5.5f;
			float cy = (float)iy - 5.5f;
			float s = sqrtf(cx * cx + cy * cy);
			s_uxuy_lookup[iy][ix] = s;
		}
	}
}

typedef struct EvalState
{
	float t;
	float alpha;
	float rotmx, rotmy;
} EvalState;

static int eval_color(EvalState* st, float x, float y)
{
	float ux = st->rotmy * x + st->rotmx * y;
	float uy = st->rotmx * x - st->rotmy * y;
	ux = ux * 10.0f + 5.0f;
	uy = uy * 10.0f + 5.0f;
	float fx = fract(ux);
	float fy = fract(uy);
	fx = MIN(fx, 1.0f - fx);
	fy = MIN(fy, 1.0f - fy);

	// the calculation below is a small table, so lookup:
	//float cx = ceilf(ux) - 5.5f;
	//float cy = ceilf(uy) - 5.5f;
	//float s = sqrtf(cx * cx + cy * cy);
	int lcx = (int)ceilf(ux);
	int lcy = (int)ceilf(uy);
	float s = s_uxuy_lookup[lcy][lcx];

	float e = 2.0f * fract((st->t - s * 0.5f) * 0.25f) - 1.0f;
	float v = fract(4.0f * MIN(fx, fy));
	float b = 0.95f * (e < 0.0f ? v : 1.0f - v) - e * e;
	//float a = smoothstep(-0.05f, 0.0f, b) + s * 0.1f;
	float a = invlerp(-0.05f, 0.0f, b) + s * 0.1f;

	float res = st->alpha < 0.5f ? e : a;
	return MIN(255, (int)(res * 250.0f));
}

#define MAX_BARS (240)
static int s_bar_count = 120;
#define BAR_WIDTH (17)
static uint8_t kBarColors[BAR_WIDTH] = { 5, 50, 96, 134, 165, 189, 206, 216, 220, 216, 206, 189, 165, 134, 96, 50, 5 };


void fx_prettyhip_update(float start_time, float end_time, float alpha)
{
	// background
	EvalState st;
	st.t = G.time * 0.3f;
	st.alpha = alpha;
	float r_angle = M_PIf / 4.0f + st.t * 0.1f + G.crank_angle_rad;
	st.rotmx = sinf(r_angle);
	st.rotmy = cosf(r_angle);

	float xsize = 1.0f;
	float ysize = 0.6f;
	float dx = xsize / SCREEN_X;
	float dy = ysize / SCREEN_Y;

	float y = ysize / 2 - dy * 0.5f;
	int t_frame_index = G.frame_count & 3;
	for (int py = 0; py < SCREEN_Y; ++py, y -= dy)
	{
		int t_row_index = py & 1;
		int col_offset = g_order_pattern_2x2[t_frame_index][t_row_index] - 1;
		if (col_offset < 0)
			continue; // this row does not evaluate any pixels

		float x = -xsize / 2 + dx * 0.5f;
		int pix_idx = py * SCREEN_X;

		x += dx * col_offset;
		pix_idx += col_offset;
		for (int px = col_offset; px < SCREEN_X; px += 2, x += dx * 2, pix_idx += 2)
		{
			int val = eval_color(&st, x, y);
			g_screen_buffer[pix_idx] = val;
		}
	}

	// foreground: kefren bars
	uint8_t bar_line[SCREEN_X];
	memset(bar_line, 0xFF, sizeof(bar_line));

	const float kSinStep1 = 0.093f;
	const float kSinStep2 = -0.063f;

	float time = G.time * 0.3f;
	float bar_step = (float)s_bar_count / (float)SCREEN_Y;
	int prev_bar_idx = -1;
	float bar_idx = 0.0f;
	for (int py = 0; py < SCREEN_Y; ++py, bar_idx += bar_step) {
		int idx = (int)bar_idx;
		if (idx != prev_bar_idx) {
			prev_bar_idx = idx;

			// Draw a new bar into our scanline
			float bar_x = (sinf(time * 1.1f + kSinStep1 * idx) + sinf(time * 2.3f + kSinStep2 * idx)) * SCREEN_X * 0.1f + SCREEN_X / 2;
			int bar_ix = ((int)bar_x) - BAR_WIDTH / 2;
			for (int x = 0; x < BAR_WIDTH; ++x) {
				bar_line[bar_ix + x] = kBarColors[x];
			}
		}

		uint8_t* dst = g_screen_buffer + py * SCREEN_X;
		//int row_bias = 50 - (SCREEN_Y - py) / 2; // fade to white near top, more black at bottom
		for (int px = 0; px < SCREEN_X; ++px, ++dst)
		{
			uint8_t v = bar_line[px];
			if (v == 0xFF)
				continue;
			*dst = v;
		}
	}

	int bias = (G.beat ? 50 : 0) + get_fade_bias(start_time, end_time);
	draw_dithered_screen(G.framebuffer, bias);
}
