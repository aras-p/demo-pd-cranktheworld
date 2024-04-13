#include "fx.h"

#include "pd_api.h"
#include "../mathlib.h"
#include "../util/pixel_ops.h"

typedef struct Blob {
	float x, y;
	float sx, sy;
	float speed;
} Blob;

#define MAX_BLOBS 9 // 9 blobs can do 30FPS
static Blob s_blobs[MAX_BLOBS];
static int s_blob_count = 5;

static uint32_t s_rng = 1;

static void blob_init(Blob* b)
{
	b->sx = RandomFloat01(&s_rng) * 0.5f + 0.1f;
	b->sy = RandomFloat01(&s_rng) * 0.5f + 0.1f;
	b->speed = (RandomFloat01(&s_rng) * 2.0f - 1.0f);
}

static int fx_blobs_update()
{
	if (G.buttons_pressed & kButtonLeft)
	{
		s_blob_count--;
		if (s_blob_count < 1)
			s_blob_count = 1;
	}
	if (G.buttons_pressed & kButtonRight)
	{
		s_blob_count++;
		if (s_blob_count > MAX_BLOBS)
			s_blob_count = MAX_BLOBS;
	}

	// update blobs
	float time = G.time;
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
	for (int py = 0; py < LCD_ROWS; py += 2)
	{
		uint8_t* row1 = G.framebuffer + py * G.framebuffer_stride;
		uint8_t* row2 = row1 + G.framebuffer_stride;
		int pix_idx = py * LCD_COLUMNS;

		for (int px = 0; px < LCD_COLUMNS; px += 2, pix_idx += 2)
		{
			float dist = 1.0f;
			for (int i = 0; i < s_blob_count; ++i) {
				const Blob* b = &s_blobs[i];
				float dx = b->x - px;
				float dy = b->y - py;
				dist *= (dx * dx + dy * dy);
			}
			int val = (int)(280 - dist * scale);

			// output 2x2 block of pixels
			int test = g_blue_noise[pix_idx];
			if (val < test) {
				put_pixel_black(row1, px);
			}
			test = g_blue_noise[pix_idx + 1];
			if (val < test) {
				put_pixel_black(row1, px + 1);
			}
			test = g_blue_noise[pix_idx + LCD_COLUMNS];
			if (val < test) {
				put_pixel_black(row2, px);
			}
			test = g_blue_noise[pix_idx + LCD_COLUMNS + 1];
			if (val < test) {
				put_pixel_black(row2, px + 1);
			}
		}
	}

	return s_blob_count;
}

Effect fx_blobs_init()
{
	for (int i = 0; i < s_blob_count; ++i) {
		blob_init(&s_blobs[0]);
	}

	return (Effect) {fx_blobs_update};
}
