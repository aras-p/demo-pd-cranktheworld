#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct PlaydateAPI PlaydateAPI;

#define TIME_UNIT_LENGTH_SECONDS (1.875f)

typedef struct Globals
{
	PlaydateAPI* pd;

	// time: all units in music bars (1.0 == 1 bar == 1.875s)
	float global_time; // current global time
	float fx_start_time; // global time when current effect started
	float fx_local_time; // time since start of current effect/scene

	// screen
	uint8_t* framebuffer;
	int framebuffer_stride;

	// input
	uint32_t buttons_cur; // buttons currently pressed, bitmask of kButton*
	uint32_t buttons_pressed; // buttons that were pressed down this frame, bitmask of kButton*
	float crank_angle_rad;
} Globals;

extern Globals G;
