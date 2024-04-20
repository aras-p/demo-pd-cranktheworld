#include "../globals.h"

#include "pd_api.h"
#include "fx.h"
#include "../mathlib.h"
#include "../util/pixel_ops.h"

#define TRIG_TABLE_SIZE 512
#define TRIG_TABLE_MASK (TRIG_TABLE_SIZE-1)
#define TRIG_TABLE_SCALE 1024
static int s_sin_table[TRIG_TABLE_SIZE];
static float s_sin_table_f[TRIG_TABLE_SIZE];

static void init_sin_table()
{
	const float idx_to_rad = 2.0f * M_PIf / TRIG_TABLE_SIZE;
	for (int i = 0; i < TRIG_TABLE_SIZE; ++i)
	{
		float rad = i * idx_to_rad;
		float v = sinf(rad);
		s_sin_table_f[i] = v;
		s_sin_table[i] = (int)(v * TRIG_TABLE_SCALE);
	}
}

static float sinf_tbl(float x)
{
	x *= TRIG_TABLE_SIZE / (2.0f * M_PIf);
	int idx = ((int)x) & TRIG_TABLE_MASK;
	return s_sin_table_f[idx];
}

static float cosf_tbl(float x)
{
	return sinf_tbl(x + M_PIf*0.5f);
}

static uint16_t s_plasma_pos1;
static uint16_t s_plasma_pos2;
static uint16_t s_plasma_pos3;
static uint16_t s_plasma_pos4;
static int s_plasma_bias = 0;


// raymarching a very simplified version of "twisty cuby" by DJDoomz
// https://www.shadertoy.com/view/MtdyWj

typedef struct EvalState
{
	float rotm_tx, rotm_ty;
	float rotm_tx6, rotm_ty6;
	float3 pos;

	float t;
	float sint;
} EvalState;

static float sdf_twisty_cuby(const EvalState* st, float3 p)
{
	p.z -= 6.0f;

	float n1 = st->rotm_ty * p.x + st->rotm_tx * p.z;
	float n2 = st->rotm_tx * p.x - st->rotm_ty * p.z;
	p.x = n1; p.z = n2;
	n1 = st->rotm_ty6 * p.y + st->rotm_tx6 * p.x;
	n2 = st->rotm_tx6 * p.y - st->rotm_ty6 * p.x;
	p.y = n1; p.x = n2;

	p = v3_abs(p);
	float d = MAX(p.x, MAX(p.y, p.z));
	return d - 2.7f;
}

static int trace_twisty_cuby(EvalState* st, float x, float y)
{
	float3 pos = st->pos;
	float3 dir = { x, y, 0.8f }; // do not normalize on purpose lol
	pos = v3_add(pos, v3_mulfl(dir, 4.0f));

	float d;
	for (int i = 0; i < 3; ++i)
	{
		d = sdf_twisty_cuby(st, pos);
		pos = v3_add(pos, v3_mulfl(dir, d * 2.5f));
	}

	float val = saturate(d * 4.0f);
	return 255 - (int)(val * 255.0f);
}


// Very simplified "Ring Twister" by Flyguy https://www.shadertoy.com/view/Xt23z3

static int eval_ring_twister(const EvalState* st, float x, float y)
{
	float rad = (x * x + y * y) * 4.0f - 1.8f;
	if (fabsf(rad) > 1.0f)
		return 0;
	float r = rad;
	float a = atan2f_approx(y, x) - st->t * 0.6f;

	float b1 = fract((a + st->t + sinf_tbl(a) * st->sint) * (2.0f / M_PIf)) * (M_PIf * 0.5f) - 2.0f;
	float b2 = b1 + (r > cosf_tbl(b1) ? 1.6f : 0.0f);

	float t2 = sinf_tbl(b2);
	r -= t2;
	if (r < 0.0f)
		return 0;

	float b3 = cosf_tbl(b2) - t2;
	if (r > b3)
		return 0;

	return (int)((0.7f * b3 - 0.5f * r) * 255.0f);
}


void fx_plasma_update(float start_time, float end_time, float alpha)
{
	int tpos4 = s_plasma_pos4;
	int tpos3 = s_plasma_pos3;

	EvalState st;
	float t = G.time * 0.5f;
	float sint = sinf(t);
	st.pos.x = 0.0f;
	st.pos.y = 0.6f * sint;
	st.pos.z = 0.0f;
	st.t = t;
	st.sint = sint * M_PIf;
	bool twisty_cube = alpha < 0.5f;

	float xsize = 3.333f;
	float ysize = 2.0f;
	float dx = xsize / LCD_COLUMNS;
	float dy = ysize / LCD_ROWS;

	int t_frame_index = G.frame_count & 3;

	float y = ysize / 2 - dy;
	for (int py = 0; py < LCD_ROWS; ++py, tpos4 += 3, tpos3 += 1, y -= dy)
	{
		tpos3 &= TRIG_TABLE_MASK;
		tpos4 &= TRIG_TABLE_MASK;

		int t_row_index = py & 1;
		int col_offset = g_order_pattern_2x2[t_frame_index][t_row_index] - 1;
		if (col_offset < 0)
			continue; // this row does not evaluate any pixels

		float x = -xsize / 2 + dx * 0.5f;
		int pix_idx = py * LCD_COLUMNS;
		x += dx * col_offset;
		pix_idx += col_offset;

		if (twisty_cube)
		{
			float tt = t + 0.2f * sinf(y * 1.5f + t);
			st.rotm_tx = cosf(tt); st.rotm_ty = sinf(tt);
			st.rotm_tx6 = cosf(tt * 0.6f); st.rotm_ty6 = sinf(tt * 0.6f);
		}

		for (int px = col_offset; px < LCD_COLUMNS; px += 2, x += dx * 2, pix_idx += 2)
		{
			int tpos1 = s_plasma_pos1 + 5 + px * 5;
			int tpos2 = s_plasma_pos2 + 3 + px * 3;
			tpos1 &= TRIG_TABLE_MASK;
			tpos2 &= TRIG_TABLE_MASK;

			int plasma = s_sin_table[tpos1] + s_sin_table[tpos2] + s_sin_table[tpos3] + s_sin_table[tpos4];

			int val = plasma >> 3;

			if (twisty_cube)
			{
				if (fabsf(x) < 1.0f)
				{
					int cube_val = trace_twisty_cuby(&st, x, y);
					if (cube_val > 0)
						val = cube_val;
				}
			}
			else
			{
				int ring_val = eval_ring_twister(&st, x, y);
				if (ring_val > 0)
					val = ring_val;
			}
			g_screen_buffer[pix_idx] = val;
		}
	}

	s_plasma_pos1 += 7;
	s_plasma_pos3 += 3;

	if (G.beat)
	{
		int r = XorShift32(&G.rng);
		s_plasma_bias = (r & 128) - 63;
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
