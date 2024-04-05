#include "pixel_ops.h"
#include "image_loader.h"

#include "../allocator.h"

#include "pd_api.h"

#include <string.h>

uint8_t* g_blue_noise;

uint8_t g_screen_buffer[LCD_COLUMNS * LCD_ROWS];
uint8_t g_screen_buffer_2x2sml[LCD_COLUMNS/2 * LCD_ROWS/2];

void init_blue_noise(void* pd_api)
{
	int bn_w, bn_h;
	g_blue_noise = read_tga_file_grayscale("BlueNoise.tga", pd_api, &bn_w, &bn_h);
	if (bn_w != LCD_COLUMNS || bn_h != LCD_ROWS) {
		pd_free(g_blue_noise);
		g_blue_noise = NULL;
	}
}


void draw_dithered_scanline(const uint8_t* values, int y, int bias, uint8_t* framebuffer)
{
	uint8_t scanline[LCD_ROWSIZE];
	const uint8_t* noise_row = g_blue_noise + y * LCD_COLUMNS;
	int px = 0;
	for (int bx = 0; bx < LCD_COLUMNS / 8; ++bx) {
		uint8_t pixbyte = 0xFF;
		for (int ib = 0; ib < 8; ++ib, ++px) {
			if (values[px] <= noise_row[px]) {
				pixbyte &= ~(1 << (7 - ib));
			}
		}
		scanline[bx] = pixbyte;
	}

	uint8_t* row = framebuffer + y * LCD_ROWSIZE;
	memcpy(row, scanline, sizeof(scanline));
}

void draw_dithered_screen_2x2(uint8_t* framebuffer, int filter)
{
	uint8_t rowvalues[LCD_COLUMNS];
	if (filter == 0)
	{
		// use same source value for each 2x2 block
		int src_idx = 0;
		for (int y = 0; y < LCD_ROWS / 2; ++y) {
			for (int x = 0; x < LCD_COLUMNS / 2; ++x, ++src_idx) {
				uint8_t val = g_screen_buffer[src_idx];
				rowvalues[x * 2 + 0] = val;
				rowvalues[x * 2 + 1] = val;
			}
			draw_dithered_scanline(rowvalues, y * 2 + 0, 0, framebuffer);
			draw_dithered_scanline(rowvalues, y * 2 + 1, 0, framebuffer);
		}
	}
	else if (filter == 1)
	{
		// filter values horizontally: bottom to top, right to left.
		// this will expand image to be full width, but half height.
		for (int y = LCD_ROWS/2 - 1; y >= 0; --y) {
			int src_idx = y * LCD_COLUMNS / 2;
			int dst_idx = y * LCD_COLUMNS;
			for (int x = LCD_COLUMNS / 2 - 1; x >= 0; --x) {
				int val_prev = g_screen_buffer[src_idx + (x > 0 ? x - 1 : 0)];
				int val_curr = g_screen_buffer[src_idx + x];
				g_screen_buffer[dst_idx + x * 2 + 0] = (val_prev + val_curr) >> 1;
				g_screen_buffer[dst_idx + x * 2 + 1] = val_curr;
			}
		}
		// draw scanlines, filtering odd ones
		for (int y = 0; y < LCD_ROWS / 2; ++y)
		{
			const uint8_t* row = &g_screen_buffer[y * LCD_COLUMNS];
			// draw raw scanline
			draw_dithered_scanline(row, y * 2 + 0, 0, framebuffer);
			int next_y = y * 2 + 2;
			if (next_y >= LCD_ROWS)
			{
				// nothing to filter with below, just draw previous
				draw_dithered_scanline(row, y * 2 + 1, 0, framebuffer);
			}
			else
			{
				// compute filtered scanline
				for (int x = 0; x < LCD_COLUMNS; ++x)
				{
					rowvalues[x] = ((int)row[x] + (int)row[x + LCD_COLUMNS]) >> 1;
				}
				draw_dithered_scanline(rowvalues, y * 2 + 1, 0, framebuffer);
			}
		}
	}
}
