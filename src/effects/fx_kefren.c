#include "fx.h"

#include "pd_api.h"
#include "../mathlib.h"
#include "../util/pixel_ops.h"

#define MAX_BARS (240)

static int s_bar_count = 120;

#define BAR_WIDTH (17)
static uint8_t kBarColors[BAR_WIDTH] = {5, 50, 96, 134, 165, 189, 206, 216, 220, 216, 206, 189, 165, 134, 96, 50, 5};

static int fx_kefren_update(uint32_t buttons_cur, uint32_t buttons_pressed, float crank_angle, float time, uint8_t* framebuffer, int framebuffer_stride)
{
	if (buttons_cur & kButtonLeft)
	{
		s_bar_count--;
		if (s_bar_count < 50)
			s_bar_count = 50;
	}
	if (buttons_cur & kButtonRight)
	{
		s_bar_count++;
		if (s_bar_count > MAX_BARS)
			s_bar_count = MAX_BARS;
	}

	uint8_t bar_line[LCD_ROWSIZE * 8];
	memset(bar_line, 0xFF, sizeof(bar_line));

	uint8_t scanline[LCD_ROWSIZE];

	const float kSinStep1 = 0.093f;
	const float kSinStep2 = -0.063f;

	float bar_step = (float)s_bar_count / (float)LCD_ROWS;
	int prev_bar_idx = -1;
	float bar_idx = 0.0f;
	for (int py = 0; py < LCD_ROWS; ++py, bar_idx += bar_step) {
		int idx = (int)bar_idx;
		if (idx != prev_bar_idx) {
			prev_bar_idx = idx;

			// Draw a new bar into our scanline
			float bar_x = (sinf(time * 1.1f + kSinStep1 * idx) + sinf(time * 2.3f + kSinStep2 * idx)) * LCD_COLUMNS * 0.2f + LCD_COLUMNS / 2;
			int bar_ix = ((int)bar_x) - BAR_WIDTH/2;
			for (int x = 0; x < BAR_WIDTH; ++x) {
				bar_line[bar_ix + x] = kBarColors[x];
			}
		}

		// dither the scanline
		const uint8_t* noise_row = g_blue_noise + py * LCD_COLUMNS;
		int px = 0;
		for (int bx = 0; bx < LCD_COLUMNS/8; ++bx) {
			uint8_t pixbyte = 0xFF;
			for (int ib = 0; ib < 8; ++ib, ++px) {
				if (bar_line[px] < noise_row[px] + 50 - (LCD_ROWS-py)/2) {
					pixbyte &= ~(1<<(7-ib));
				}
			}
			scanline[bx] = pixbyte;
		}

		uint8_t* row = framebuffer + py * framebuffer_stride;
		memcpy(row, scanline, sizeof(scanline));
	}
	return s_bar_count;
}

Effect fx_kefren_init(void* pd_api)
{
	return (Effect) {fx_kefren_update};
}
