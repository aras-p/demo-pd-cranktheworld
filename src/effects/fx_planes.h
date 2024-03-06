#pragma once

#include <stdint.h>

void fx_planes_init();
int fx_planes_update(uint32_t buttons_cur, float crank_angle, float time, uint8_t* framebuffer, int framebuffer_stride);
