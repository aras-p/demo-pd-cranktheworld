#pragma once

#include <stdint.h>

typedef struct Effect {
	int (*update)(uint32_t buttons_cur, uint32_t buttons_pressed, float crank_angle, float time, uint8_t* framebuffer, int framebuffer_stride);
} Effect;
