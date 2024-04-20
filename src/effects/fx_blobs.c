#include "../globals.h"

#include "pd_api.h"
#include "fx.h"
#include "../mathlib.h"
#include "../util/pixel_ops.h"

typedef struct Blob {
	float x, y;
	float sx, sy;
	float speed;
} Blob;

#define MIN_BLOBS 3
#define MAX_BLOBS 6
static Blob s_blobs[MAX_BLOBS];
static int s_blob_count = 5;

static void blob_init(Blob* b)
{
	b->sx = RandomFloat01(&G.rng) * 0.5f + 0.1f;
	b->sy = RandomFloat01(&G.rng) * 0.5f + 0.1f;
	b->speed = (RandomFloat01(&G.rng) * 2.0f - 1.0f);
}

// Very simplified "Ring Twister" by Flyguy https://www.shadertoy.com/view/Xt23z3

typedef struct EvalState
{
	float t;
	float sint;
} EvalState;

static int eval_ring_twister(const EvalState* st, float x, float y)
{
	float rad = (x * x + y * y) * 4.0f - 1.8f;
	if (fabsf(rad) > 1.0f)
		return 0;
	float r = rad;
	float a = atan2f(y, x) - st->t * 0.6f;

	float b1 = fract((a + st->t + sinf(a) * st->sint) * (2.0f/M_PIf)) * (M_PIf * 0.5f) - 2.0f;
	float b2 = b1 + (r > cosf(b1) ? 1.6f : 0.0f);

	float t2 = sinf(b2);
	r -= t2;
	if (r < 0.0f)
		return 0;

	float b3 = cosf(b2) - t2;
	if (r > b3)
		return 0;

	return (int)((0.7f * b3 - 0.5f * r) * 255.0f);
}

void fx_blobs_update(float start_time, float end_time, float alpha)
{
	if (G.buttons_pressed & kButtonLeft)
	{
		s_blob_count--;
		if (s_blob_count < MIN_BLOBS)
			s_blob_count = MIN_BLOBS;
	}
	if (G.buttons_pressed & kButtonRight)
	{
		s_blob_count++;
		if (s_blob_count > MAX_BLOBS)
			s_blob_count = MAX_BLOBS;
	}

	// update blobs
	float time = G.time * (0.05f + 0.4f * (cosf(G.crank_angle_rad) * 0.5f + 0.5f));
	float offset = 0.0f;
	for (int i = 0; i < s_blob_count; ++i) {
		Blob* b = &s_blobs[i];
		if (b->sx == 0)
			blob_init(b);

		b->x = sinf((time + offset) * M_PIf * b->speed) * LCD_COLUMNS * b->sx + LCD_COLUMNS / 2;
		b->y = cosf((time + offset) * M_PIf * b->speed) * LCD_ROWS * b->sy + LCD_ROWS / 2;

		offset += 0.5f;
	}

	float scale = 1.0f / (1.0f * powf(10.0f, (float)(3 + s_blob_count * 3)));

	EvalState st;
	float t = G.time * 0.5f;
	st.t = t;
	st.sint = sinf(t) * M_PIf;

	float xsize = 3.333f;
	float ysize = 2.0f;
	float dx = xsize / LCD_COLUMNS;
	float dy = ysize / LCD_ROWS;
	
	int t_frame_index = G.frame_count & 3;
	float y = ysize / 2 - dy;
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
			float dist = 1.0f;
			for (int i = 0; i < s_blob_count; ++i) {
				const Blob* b = &s_blobs[i];
				float dx = b->x - px;
				float dy = b->y - py;
				dist *= (dx * dx + dy * dy);
			}
			int val = (int)(280 - dist * scale);
			val = MAX(-250, val);

			int val_ring = eval_ring_twister(&st, x, y);
			if (val_ring > 0)
				val = val_ring;

			g_screen_buffer[pix_idx] = val;
		}
	}
	int bias = (G.beat ? 50 : 0) + get_fade_bias(start_time, end_time);
	draw_dithered_screen(G.framebuffer, bias);
}

void fx_blobs_init()
{
	for (int i = 0; i < s_blob_count; ++i) {
		blob_init(&s_blobs[0]);
	}
}
