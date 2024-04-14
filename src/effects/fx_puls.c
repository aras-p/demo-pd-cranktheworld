#include "../globals.h"

#include "pd_api.h"
#include "../mathlib.h"
#include "../util/pixel_ops.h"
#include <stdbool.h>

// Puls by Rrrola "tribute", I guess?
// somewhat based on https://wakaba.c3.cx/w/puls.html

#define MAX_TRACE_STEPS 24

static float puls_sdf(float timeParam, float3 pos)
{
	float v2x = fabsf(fract(pos.x) - 0.5f) / 2.0f;
	float v2y = fabsf(fract(pos.y) - 0.5f) / 2.0f;
	float v2z = fabsf(fract(pos.z) - 0.5f) / 2.0f;
	float r = timeParam;

	float d1 = v2x + v2y + v2z - 0.1445f + r;

	v2x = 0.25f - v2x;
	v2y = 0.25f - v2y;
	v2z = 0.25f - v2z;
	float width = (0.08f - r*2.0f) * 0.3f;
	float dx = fabsf(v2z - v2x);
	float dy = fabsf(v2x - v2y);
	float dz = fabsf(v2y - v2z);
	float d2 = dx + dy + dz - width;

	return MIN(d1, d2);
}

typedef struct PulsState
{
	float t;
	float t_param;
	float sin_a, cos_a;
	float3 pos;
} PulsState;

static int trace_ray(const PulsState* st, float x, float y)
{
	float3 pos = st->pos;
	float3 dir = { x, -y, 0.33594f - x * x - y * y };
	dir = (float3){ dir.y, dir.z * st->cos_a - dir.x * st->sin_a, dir.x * st->cos_a + dir.z * st->sin_a };
	dir = (float3){ dir.y, dir.z * st->cos_a - dir.x * st->sin_a, dir.x * st->cos_a + dir.z * st->sin_a };
	dir = (float3){ dir.y, dir.z * st->cos_a - dir.x * st->sin_a, dir.x * st->cos_a + dir.z * st->sin_a };

	float t = 0.0f;
	int i;
	for (i = 0; i < MAX_TRACE_STEPS; ++i)
	{
		float3 q = v3_add(pos, v3_mulfl(dir, t));
		float d = puls_sdf(st->t_param, q);
		if (d < t * 0.07f)
			break;
		t += d * 1.7f;
	}

	//return (int)(((float)j) / (float)MAXSTEP * 255.0f);
	return MIN((int)(t * 0.25f * 255.0f), 255);
}

static int s_frame_count = 0;
static int s_temporal_mode = 1;
#define TEMPORAL_MODE_COUNT 6

static void do_render(float crank_angle, float time, uint8_t* framebuffer, int framebuffer_stride)
{
	PulsState st;
	st.t = time * 10.0f;
	st.t_param = 0.0769f * sinf(st.t * 2.0f * -0.0708f);
	st.sin_a = sinf(st.t * 0.00564f);
	st.cos_a = cosf(st.t * 0.00564f);
	st.pos.x = 0.5f + 0.0134f * st.t;
	st.pos.y = 1.1875f + 0.0134f * st.t;
	st.pos.z = 0.875f + 0.0134f * st.t;

	float xsize = 1.0f;
	float ysize = 0.6f;
	float dx = xsize / LCD_COLUMNS;
	float dy = ysize / LCD_ROWS;

	if (s_temporal_mode == 0)
	{
		// trace one ray per 2x2 pixel block
		float y = ysize / 2 - dy;
		for (int py = 0; py < LCD_ROWS / 2; ++py, y -= dy * 2)
		{
			float x = -xsize / 2 + dx;
			int pix_idx = py * LCD_COLUMNS / 2;
			for (int px = 0; px < LCD_COLUMNS / 2; ++px, x += dx * 2, ++pix_idx)
			{
				int val = trace_ray(&st, x, y);
				g_screen_buffer[pix_idx] = val;
			}
		}
		draw_dithered_screen_2x2(framebuffer, 1);
	}
	if (s_temporal_mode == 1)
	{
		// temporal: one ray for each 2x2 block, and also update one pixel within each 2x2 macroblock (16x fewer rays): 28fps (35ms)
		float y = ysize / 2 - dy;
		for (int py = 0; py < LCD_ROWS / 2; ++py, y -= dy * 2)
		{
			// Temporal: every frame update just one out of every 2x2 pixel blocks.
			// Which means every other frame we skip every other row.
			if ((s_frame_count & 1) == (py & 1))
				continue;

			float x = -xsize / 2 + dx;
			int pix_idx = py * LCD_COLUMNS / 2;
			// And for each row we step at 2 pixels, but shift location by one every
			// other frame.
			if ((s_frame_count & 2)) {
				x += dx;
				pix_idx++;
			}
			for (int px = 0; px < LCD_COLUMNS / 2; px += 2, x += dx * 4, pix_idx += 2)
			{
				int val = trace_ray(&st, x, y);
				g_screen_buffer_2x2sml[pix_idx] = val;
			}
		}

		memcpy(g_screen_buffer, g_screen_buffer_2x2sml, LCD_COLUMNS / 2 * LCD_ROWS / 2);
		draw_dithered_screen_2x2(framebuffer, 1);
	}
	if (s_temporal_mode == 2)
	{
		// 2x2 block temporal update one pixel per frame
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
	}
	if (s_temporal_mode == 3)
	{
		// 4x2 block temporal update one pixel per frame
		float y = ysize / 2 - dy * 0.5f;
		int t_frame_index = s_frame_count & 7;
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
		draw_dithered_screen(framebuffer, 0);
	}
	if (s_temporal_mode == 4)
	{
		// 4x3 block temporal update one pixel per frame
		float y = ysize / 2 - dy * 0.5f;
		int t_frame_index = s_frame_count % 12;
		for (int py = 0; py < LCD_ROWS; ++py, y -= dy)
		{
			int t_row_index = py % 3;
			int col_offset = g_order_pattern_4x3[t_frame_index][t_row_index] - 1;
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
		draw_dithered_screen(framebuffer, 0);
	}
	if (s_temporal_mode == 5)
	{
		// 4x4 block temporal update one pixel per frame
		float y = ysize / 2 - dy * 0.5f;
		int t_frame_index = s_frame_count & 15;
		for (int py = 0; py < LCD_ROWS; ++py, y -= dy)
		{
			int t_row_index = py & 3;
			int col_offset = g_order_pattern_4x4[t_frame_index][t_row_index] - 1;
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
		draw_dithered_screen(framebuffer, 0);
	}
	++s_frame_count;
}


int fx_puls_update()
{
	if (G.buttons_pressed & kButtonLeft)
	{
		s_temporal_mode--;
		if (s_temporal_mode < 0)
			s_temporal_mode = TEMPORAL_MODE_COUNT - 1;
	}
	if (G.buttons_pressed & kButtonRight)
	{
		s_temporal_mode++;
		if (s_temporal_mode >= TEMPORAL_MODE_COUNT)
			s_temporal_mode = 0;
	}

	do_render(G.crank_angle_rad, G.time, G.framebuffer, G.framebuffer_stride);
	return s_temporal_mode;
}
