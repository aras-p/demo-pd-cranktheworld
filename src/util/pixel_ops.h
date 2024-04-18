#pragma once

#include <stdint.h>

extern uint8_t g_screen_buffer[];
extern uint8_t g_screen_buffer_2x2sml[];

void init_pixel_ops();

static inline void put_pixel_black(uint8_t* row, int x)
{
	uint8_t mask = ~(1 << (7 - (x & 7)));
	row[x >> 3] &= mask;
}

// negative bias lightens the image, positive darkens
void draw_dithered_scanline(const uint8_t* values, int y, int bias, uint8_t* framebuffer);
void draw_dithered_screen(uint8_t* framebuffer, int bias);
void draw_dithered_screen_2x2(uint8_t* framebuffer, int filter);

extern int g_order_pattern_2x2[4][2];
extern int g_order_pattern_3x2[6][2];
extern int g_order_pattern_4x2[8][2];
extern int g_order_pattern_4x3[12][3];
extern int g_order_pattern_4x4[16][4];

void draw_line(uint8_t* framebuffer, int width, int height, int x1, int y1, int x2, int y2, uint8_t color);
