#include "fx.h"

#include "pd_api.h"
#include "../mathlib.h"
#include "../util/pixel_ops.h"

typedef struct Blob {
	float x, y;
	float sx, sy;
	float speed;
} Blob;

#define MAX_BLOBS 20
static Blob s_blobs[MAX_BLOBS];
static int s_blob_count = 5;

static uint32_t s_rng = 1;

static void blob_init(Blob* b)
{
	b->sx = RandomFloat01(&s_rng) * 0.5f + 0.1f;
	b->sy = RandomFloat01(&s_rng) * 0.5f + 0.1f;
	b->speed = (RandomFloat01(&s_rng) * 2.0f - 1.0f);
}

static int fx_blobs_update(uint32_t buttons_cur, uint32_t buttons_pressed, float crank_angle, float time, uint8_t* framebuffer, int framebuffer_stride)
{
	if (buttons_pressed & kButtonLeft)
	{
		s_blob_count--;
		if (s_blob_count < 1)
			s_blob_count = 1;
	}
	if (buttons_pressed & kButtonRight)
	{
		s_blob_count++;
		if (s_blob_count > MAX_BLOBS)
			s_blob_count = MAX_BLOBS;
	}

	// update blobs
	float offset = 0.0f;
	for (int i = 0; i < s_blob_count; ++i) {
		Blob* b = &s_blobs[i];
		if (b->sx == 0)
			blob_init(b);

		b->x = sinf((time + offset) * M_PIf * b->speed) * LCD_COLUMNS * b->sx + LCD_COLUMNS / 2;
		b->y = cosf((time + offset) * M_PIf * b->speed) * LCD_ROWS * b->sy + LCD_ROWS / 2;

		offset += 0.5f;
	}


	int pix_idx = 0;
	float scale = 1.0f / (3.0f * powf(10.0f, (2 + s_blob_count))); // 1.0f / 3.0e8f
	for (int py = 0; py < LCD_ROWS; ++py)
	{
		uint8_t* row = framebuffer + py * framebuffer_stride;

		for (int px = 0; px < LCD_COLUMNS; ++px, ++pix_idx)
		{
			float dist = 1.0f;
			for (int i = 0; i < s_blob_count; ++i) {
				const Blob* b = &s_blobs[i];
				float dx = b->x - px;
				float dy = b->y - py;
				dist *= sqrtf(dx * dx + dy * dy);
			}
			int val = (int)(1024 - dist * scale);
			int test = g_blue_noise[pix_idx];
			if (val < test) {
				put_pixel_black(row, px);
			}
		}
	}

	return s_blob_count;
}

Effect fx_blobs_init(void* pd_api)
{
	for (int i = 0; i < s_blob_count; ++i) {
		blob_init(&s_blobs[0]);
	}

	return (Effect) {fx_blobs_update};
}
