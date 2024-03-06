#include "fx.h"

#include "pd_api.h"
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

static uint32_t s_rng = 1;

static uint16_t s_plasma_pos1;
static uint16_t s_plasma_pos2;
static uint16_t s_plasma_pos3;
static uint16_t s_plasma_pos4;

static int fx_plasma_update(uint32_t buttons_cur, uint32_t buttons_pressed, float crank_angle, float time, uint8_t* framebuffer, int framebuffer_stride)
{
	s_rng = 1;
	int tpos4 = s_plasma_pos4;
	int tpos3 = s_plasma_pos3;

	int pix_idx = 0;
	for (int py = 0; py < LCD_ROWS; ++py)
	{
		int tpos1 = s_plasma_pos1 + 5;
		int tpos2 = s_plasma_pos2 + 3;

		tpos3 &= TRIG_TABLE_MASK;
		tpos4 &= TRIG_TABLE_MASK;

		uint8_t* row = framebuffer + py * framebuffer_stride;

		for (int px = 0; px < LCD_COLUMNS; ++px, ++pix_idx)
		{
			tpos1 &= TRIG_TABLE_MASK;
			tpos2 &= TRIG_TABLE_MASK;

			int plasma = s_sin_table[tpos1] + s_sin_table[tpos2] + s_sin_table[tpos3] + s_sin_table[tpos4];

			int index = plasma >> 3;
			index &= 0xFF;
			//if (index < 0) index = 0;
			//if (index > 255) index = 255;

			int test = g_blue_noise[pix_idx];
			if (index < test) {
				put_pixel_black(row, px);
			}

			tpos1 += 5;
			tpos2 += 3;
		}

		tpos4 += 3;
		tpos3 += 1;
	}

	s_plasma_pos1 += 7;
	s_plasma_pos3 += 3;

	//s_plasma_pos2 += 17;
	//s_plasma_pos4 += 17;

	return 0;
}

Effect fx_plasma_init(void* pd_api)
{
	init_sin_table();

	return (Effect) {fx_plasma_update};
}
