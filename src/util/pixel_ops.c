#include "pixel_ops.h"
#include "image_loader.h"

#include "../allocator.h"

#include "pd_api.h"

#include <string.h>

uint8_t* g_blue_noise;

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