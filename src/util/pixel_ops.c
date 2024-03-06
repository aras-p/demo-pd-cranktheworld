#include "pixel_ops.h"
#include "image_loader.h"

#include "../allocator.h"

#include "pd_api.h"

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
