#include "fx.h"

#include "pd_api.h"
#include "../mathlib.h"
#include "../util/pixel_ops.h"

#define MAX_BARS (240)

static int s_bar_count = 120;

#define BAR_WIDTH (17)
static uint8_t kBarColors[BAR_WIDTH] = {5, 50, 96, 134, 165, 189, 206, 216, 220, 216, 206, 189, 165, 134, 96, 50, 5};

static int fx_kefren_update()
{
	if (G.buttons_cur & kButtonLeft)
	{
		s_bar_count--;
		if (s_bar_count < 50)
			s_bar_count = 50;
	}
	if (G.buttons_cur & kButtonRight)
	{
		s_bar_count++;
		if (s_bar_count > MAX_BARS)
			s_bar_count = MAX_BARS;
	}

	uint8_t bar_line[LCD_ROWSIZE * 8];
	memset(bar_line, 0xFF, sizeof(bar_line));

	const float kSinStep1 = 0.093f;
	const float kSinStep2 = -0.063f;

	float time = G.fx_local_time;
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

		int dither_bias = 50 - (LCD_ROWS - py) / 2; // fade to white near top, more black at bottom
		draw_dithered_scanline(bar_line, py, dither_bias, G.framebuffer);
	}
	return s_bar_count;
}

Effect fx_kefren_init()
{
	return (Effect) {fx_kefren_update};
}
