#include "fx.h"
#include "../globals.h"
#include "../mathlib.h"

#define FADE_DURATION 1.0f

int get_fade_bias(float start_time, float end_time)
{
	int bias = 0;
	float t = G.time;
	if (G.ending)
	{
		if (t < FADE_DURATION)
			bias = (int)lerp(-250.0f, 0.0f, t);
	}
	else
	{
		if (t < start_time + FADE_DURATION)
			bias = (int)lerp(-250.0f, 0.0f, t - start_time);
		if (t > end_time - FADE_DURATION)
			bias = (int)lerp(0.0f, -250.0f, t - (end_time - FADE_DURATION));
	}

	bias = MIN(0, bias);
	return bias;
}