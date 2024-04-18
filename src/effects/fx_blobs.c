#include "../globals.h"

#include "pd_api.h"
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
	
	int t_frame_index = G.frame_count & 3;
	for (int py = 0; py < LCD_ROWS; ++py)
	{
		int t_row_index = py & 1;
		int col_offset = g_order_pattern_2x2[t_frame_index][t_row_index] - 1;
		if (col_offset < 0)
			continue; // this row does not evaluate any pixels

		int pix_idx = py * LCD_COLUMNS;
		pix_idx += col_offset;
		for (int px = col_offset; px < LCD_COLUMNS; px += 2, pix_idx += 2)
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
			g_screen_buffer[pix_idx] = val;
		}
	}
	draw_dithered_screen(G.framebuffer, G.beat ? 50 : 0);
}

void fx_blobs_init()
{
	for (int i = 0; i < s_blob_count; ++i) {
		blob_init(&s_blobs[0]);
	}
}
