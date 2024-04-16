#include "../globals.h"

#include "pd_api.h"
#include "../mathlib.h"
#include "../util/pixel_ops.h"
#include <stdbool.h>

typedef struct TraceState
{
	float t;
	float rotmx, rotmy;
	float3 pos;
} TraceState;

static int trace_ray(const TraceState* st, float x, float y)
{
	float rx = st->rotmy * x + st->rotmx * y;
	float ry = st->rotmx * x - st->rotmy * y;
	float rax = fabsf(rx);
	float ray = fabsf(ry);
	if (rax < 0.6f && ray < 0.3f)
		return 0;
	if (rax < 0.9f && ray < 0.45f)
		return 255;
	return (int)((sinf(rx * 5) + cosf(ry * 5) + cosf(rx + ry * 2) + sinf(rx * 3 - ry * 1.2f )) * 255);
}

// For each 4x4 pixel block, only two pixels are evaluated each frame.
// So that the whole block is fully evaluated across 8 frames. The pixels
// and the frames they are evaluated in:
//
// 0 5 2 4
// 7 3 1 6
// 2 4 7 0
// 6 1 5 3

static int s_pattern_4x4_8[8][4] = {
	{1, 0, 4, 0},
	{0, 3, 0, 2},
	{3, 0, 1, 0},
	{0, 2, 0, 4},
	{4, 0, 2, 0},
	{2, 0, 0, 3},
	{0, 4, 0, 1},
	{0, 1, 3, 0},
};

static int s_temporal_mode = 2;
#define TEMPORAL_MODE_COUNT 8

int fx_temporal_test_update()
{
	if (G.buttons_pressed & kButtonLeft)
	{
		s_temporal_mode--;
		if (s_temporal_mode < 0)
			s_temporal_mode = TEMPORAL_MODE_COUNT-1;
	}
	if (G.buttons_pressed & kButtonRight)
	{
		s_temporal_mode++;
		if (s_temporal_mode >= TEMPORAL_MODE_COUNT)
			s_temporal_mode = 0;
	}

	TraceState st;
	st.t = G.time;
	float rot_angle = st.t / 4.0f;
	rot_angle = G.crank_angle_rad;
	st.rotmx = sinf(rot_angle);
	st.rotmy = cosf(rot_angle);

	float xsize = 3.3333f;
	float ysize = 2.0f;

	float dx = xsize / LCD_COLUMNS;
	float dy = ysize / LCD_ROWS;
	if (s_temporal_mode == 0)
	{
		// full screen trace: 3fps (333ms)
		float y = ysize/2 - dy * 0.5f;
		for (int py = 0; py < LCD_ROWS; ++py, y -= dy)
		{
			float x = -xsize/2 + dx * 0.5f;
			int pix_idx = py * LCD_COLUMNS;
			for (int px = 0; px < LCD_COLUMNS; px += 1, x += dx, pix_idx += 1)
			{
				int val = trace_ray(&st, x, y);
				g_screen_buffer[pix_idx] = val;
			}
		}
		draw_dithered_screen(G.framebuffer, 0);
	}
	if (s_temporal_mode == 1)
	{
		// trace once per 2x2 block, interpolate spatially: 11fps (90ms)
		float y = ysize/2 - dy;
		for (int py = 0; py < LCD_ROWS / 2; ++py, y -= dy * 2)
		{
			float x = -xsize/2 + dx;
			int pix_idx = py * LCD_COLUMNS / 2;
			for (int px = 0; px < LCD_COLUMNS / 2; px += 1, x += dx * 2, pix_idx += 1)
			{
				int val = trace_ray(&st, x, y);
				g_screen_buffer[pix_idx] = val;
			}
		}
		draw_dithered_screen_2x2(G.framebuffer, 1);
	}
	if (s_temporal_mode == 2)
	{
		// temporal: one ray for each 2x2 block, and also update one pixel within each 2x2 macroblock (16x fewer rays): 28fps (35ms)
		float y = ysize / 2 - dy;
		for (int py = 0; py < LCD_ROWS / 2; ++py, y -= dy * 2)
		{
			// Temporal: every frame update just one out of every 2x2 pixel blocks.
			// Which means every other frame we skip every other row.
			if ((G.frame_count & 1) == (py & 1))
				continue;

			float x = -xsize / 2 + dx;
			int pix_idx = py * LCD_COLUMNS / 2;
			// And for each row we step at 2 pixels, but shift location by one every
			// other frame.
			if ((G.frame_count & 2)) {
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
		draw_dithered_screen_2x2(G.framebuffer, 1);
	}
	if (s_temporal_mode == 3)
	{
		// Temporal: for each 4x4 block, trace only two pixels each frame (8x fewer rays): 18fps (55ms)
		float y = ysize / 2 - dy * 0.5f;
		int t_frame_index = G.frame_count & 7;
		for (int py = 0; py < LCD_ROWS; ++py, y -= dy)
		{
			int t_row_index = py & 3;
			int col_offset = s_pattern_4x4_8[t_frame_index][t_row_index] - 1;
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
		draw_dithered_screen(G.framebuffer, 0);
	}
	if (s_temporal_mode == 4)
	{
		// Temporal: 4x2 pixel block, ordered dither matrix (8x fewer rays): 18fps (55ms). Somewhat more vertical stripe artifacts compared to above in motion.
		// 0 4 2 6
		// 3 7 1 5
		static int s_pattern[8][2] = {
			{1, 0},
			{0, 3},
			{3, 0},
			{0, 1},
			{2, 0},
			{0, 4},
			{4, 0},
			{0, 2},
		};
		float y = ysize / 2 - dy * 0.5f;
		int t_frame_index = G.frame_count & 7;
		for (int py = 0; py < LCD_ROWS; ++py, y -= dy)
		{
			int t_row_index = py & 1;
			int col_offset = s_pattern[t_frame_index][t_row_index] - 1;
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
		draw_dithered_screen(G.framebuffer, 0);
	}
	if (s_temporal_mode == 5)
	{
		// Temporal: 3x3 pixel block, ordered dither matrix (9x fewer rays): 20fps (50ms)
		// 0 5 2
		// 3 8 7
		// 6 1 4
		static int s_pattern[9][3] = {
			{1, 0, 0},
			{0, 0, 2},
			{3, 0, 0},
			{0, 1, 0},
			{0, 0, 3},
			{2, 0, 0},
			{0, 0, 1},
			{0, 3, 0},
			{0, 2, 0},
		};
		float y = ysize / 2 - dy * 0.5f;
		int t_frame_index = G.frame_count % 9;
		for (int py = 0; py < LCD_ROWS; ++py, y -= dy)
		{
			int t_row_index = py % 3;
			int col_offset = s_pattern[t_frame_index][t_row_index] - 1;
			if (col_offset < 0)
				continue; // this row does not evaluate any pixels

			float x = -xsize / 2 + dx * 0.5f;
			int pix_idx = py * LCD_COLUMNS;
			x += dx * col_offset;
			pix_idx += col_offset;
			for (int px = col_offset; px < LCD_COLUMNS; px += 3, x += dx * 3, pix_idx += 3)
			{
				int val = trace_ray(&st, x, y);
				g_screen_buffer[pix_idx] = val;
			}
		}
		draw_dithered_screen(G.framebuffer, 0);
	}
	if (s_temporal_mode == 6)
	{
		// Temporal: 4x3 pixel block, ordered dither matrix (12x fewer rays): 25 fps (40ms)
		// 0 9 6 3
		// 7 4 8 B
		// 2 A 1 5
		static int s_pattern[12][3] = {
			{1, 0, 0},
			{0, 0, 3},
			{0, 0, 1},
			{4, 0, 0},
			{0, 2, 0},
			{0, 0, 4},
			{3, 0, 0},
			{0, 1, 0},
			{0, 3, 0},
			{2, 0, 0},
			{0, 0, 2},
			{0, 4, 0},
		};
		float y = ysize / 2 - dy * 0.5f;
		int t_frame_index = G.frame_count % 12;
		for (int py = 0; py < LCD_ROWS; ++py, y -= dy)
		{
			int t_row_index = py % 3;
			int col_offset = s_pattern[t_frame_index][t_row_index] - 1;
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
		draw_dithered_screen(G.framebuffer, 0);
	}
	if (s_temporal_mode == 7)
	{
		// Temporal: 4x4 pixel block, ordered dither matrix (16x fewer rays): 30fps (33ms)
		//  0 12  3 15
		//  8  4 11  7
		//  2 14  1 13
		// 10  6  9  5
		static int s_pattern[16][4] = {
			{1, 0, 0, 0},
			{0, 0, 3, 0},
			{0, 0, 1, 0},
			{3, 0, 0, 0},
			{0, 2, 0, 0},
			{0, 0, 0, 4},
			{0, 0, 0, 2},
			{0, 4, 0, 0},
			{0, 1, 0, 0},
			{0, 0, 0, 3},
			{0, 0, 0, 1},
			{0, 3, 0, 0},
			{2, 0, 0, 0},
			{0, 0, 4, 0},
			{0, 0, 2, 0},
			{4, 0, 0, 0},
		};
		float y = ysize / 2 - dy * 0.5f;
		int t_frame_index = G.frame_count & 15;
		for (int py = 0; py < LCD_ROWS; ++py, y -= dy)
		{
			int t_row_index = py & 3;
			int col_offset = s_pattern[t_frame_index][t_row_index] - 1;
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
		draw_dithered_screen(G.framebuffer, 0);
	}

	return s_temporal_mode;
}
