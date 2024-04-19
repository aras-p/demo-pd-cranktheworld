#include "../globals.h"

#include "pd_api.h"
#include "fx.h"
#include "../mathlib.h"
#include "../util/pixel_ops.h"

#define TRIG_TABLE_SIZE 512
#define TRIG_TABLE_MASK (TRIG_TABLE_SIZE-1)
#define TRIG_TABLE_SCALE 1024
static int s_sin_table[TRIG_TABLE_SIZE];

static void init_sin_table()
{
	const float idx_to_rad = 2.0f * M_PIf / TRIG_TABLE_SIZE;
	for (int i = 0; i < TRIG_TABLE_SIZE; ++i)
	{
		float rad = i * idx_to_rad;
		s_sin_table[i] = (int)(sinf(rad) * TRIG_TABLE_SCALE);
	}
}

static uint16_t s_plasma_pos1;
static uint16_t s_plasma_pos2;
static uint16_t s_plasma_pos3;
static uint16_t s_plasma_pos4;
static int s_plasma_bias = 0;

void fx_plasma_update(float start_time, float end_time, float alpha)
{
	int tpos4 = s_plasma_pos4;
	int tpos3 = s_plasma_pos3;

	int pix_idx = 0;
	uint8_t* pix = g_screen_buffer;
	for (int py = 0; py < LCD_ROWS; ++py)
	{
		int tpos1 = s_plasma_pos1 + 5;
		int tpos2 = s_plasma_pos2 + 3;

		tpos3 &= TRIG_TABLE_MASK;
		tpos4 &= TRIG_TABLE_MASK;

		for (int px = 0; px < LCD_COLUMNS; ++px, ++pix_idx)
		{
			tpos1 &= TRIG_TABLE_MASK;
			tpos2 &= TRIG_TABLE_MASK;

			int plasma = s_sin_table[tpos1] + s_sin_table[tpos2] + s_sin_table[tpos3] + s_sin_table[tpos4];

			int val = plasma >> 3;
			*pix++ = val;

			tpos1 += 5;
			tpos2 += 3;
		}

		tpos4 += 3;
		tpos3 += 1;
	}

	s_plasma_pos1 += 7;
	s_plasma_pos3 += 3;

	if (G.beat)
	{
		int r = XorShift32(&G.rng);
		s_plasma_bias = (r & 255) - 128;
	}
	if (G.ending)
	{
		s_plasma_bias = 0;
		s_plasma_pos2 = (int)(sinf(G.crank_angle_rad) * 764);
		s_plasma_pos4 = (int)(sinf(G.crank_angle_rad) * 431);
	}

	int bias = s_plasma_bias + get_fade_bias(start_time, end_time);
	draw_dithered_screen(G.framebuffer, bias);
}

void fx_plasma_init()
{
	init_sin_table();
}
