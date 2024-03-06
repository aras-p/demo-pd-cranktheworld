#pragma once

#include <stdint.h>

extern uint8_t* g_blue_noise;

void init_blue_noise(void* pd_api);

static inline void put_pixel_black(uint8_t* row, int x)
{
	uint8_t mask = ~(1 << (7 - (x & 7)));
	row[x >> 3] &= mask;
}
