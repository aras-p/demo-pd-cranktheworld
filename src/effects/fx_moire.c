#include "fx.h"

#include "pd_api.h"
#include "../mathlib.h"
#include "../util/pixel_ops.h"

static int fx_moire_update()
{
	float t = G.fx_local_time;
	float cx1 = sinf(t / 2) * LCD_COLUMNS / 3 + LCD_COLUMNS / 2;
	float cy1 = sinf(t / 4) * LCD_ROWS / 3 + LCD_ROWS / 2;
	float cx2 = cosf(t / 3) * LCD_COLUMNS / 3 + LCD_COLUMNS / 2;
	float cy2 = cosf(t) * LCD_ROWS / 3 + LCD_ROWS / 2;

	int pix_idx = 0;
	for (int py = 0; py < LCD_ROWS; ++py)
	{
		uint8_t* row = G.framebuffer + py * G.framebuffer_stride;

		float dy1 = py - cy1; dy1 *= dy1;
		float dy2 = py - cy2; dy2 *= dy2;
		for (int px = 0; px < LCD_COLUMNS; ++px, ++pix_idx)
		{
			float dx1 = px - cx1; dx1 *= dx1;
			float dx2 = px - cx2; dx2 *= dx2;
			int dist1 = (int)sqrtf(dx1 + dy1);
			int dist2 = (int)sqrtf(dx2 + dy2);
			int val = (dist1 ^ dist2) >> 4;
			if (val & 1) {
				put_pixel_black(row, px);
			}
		}
	}

	return 0;
}

Effect fx_moire_init()
{
	return (Effect) {fx_moire_update};
}
