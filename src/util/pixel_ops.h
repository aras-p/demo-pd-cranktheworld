#pragma once

#include <stdint.h>

extern uint8_t* g_blue_noise;
extern uint8_t g_screen_buffer[];

void init_blue_noise(void* pd_api);

static inline void put_pixel_black(uint8_t* row, int x)
{
	uint8_t mask = ~(1 << (7 - (x & 7)));
	row[x >> 3] &= mask;
}

void draw_dithered_scanline(const uint8_t* values, int y, int bias, uint8_t* framebuffer);

void draw_dithered_screen_2x2(uint8_t* framebuffer, int filter);
