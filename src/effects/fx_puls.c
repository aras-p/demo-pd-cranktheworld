#include "fx.h"

#include "pd_api.h"
#include "../mathlib.h"
#include "../util/pixel_ops.h"
#include <stdbool.h>

// Puls by Rrrola "tribute", I guess?
// somewhat based on https://wakaba.c3.cx/w/puls.html

const float BLOWUP = 38.0f;
const int MAXITERS = 14;
const int MAXSTEP = 24;
#define MAXSTEPSHIFT 8

static float kBlowupTable[MAXSTEPSHIFT + 1]; // BLOWUP / pow(2, stepshift + 8)
static float kStepTable[MAXSTEPSHIFT + 1]; // 1.0f / pow(2, stepshift)

static bool s_Inited = false;
static void init_puls()
{
	if (s_Inited)
		return;
	s_Inited = true;
	for (int i = 0; i < MAXSTEPSHIFT + 1; ++i)
	{
		kBlowupTable[i] = BLOWUP / powf(2.0f, i + 8.0f);
		kStepTable[i] = 1.0f / powf(2.0f, (float)i);
	}
}

static int func(float timeParam, float3 pos, int stepshift)
{
	float v2x = fabsf(fract(pos.x) - 0.5f) / 2.0f;
	float v2y = fabsf(fract(pos.y) - 0.5f) / 2.0f;
	float v2z = fabsf(fract(pos.z) - 0.5f) / 2.0f;
	float r = timeParam;
	float blowup = kBlowupTable[stepshift];

	float sum = v2x + v2y + v2z;
	if (sum - 0.1445f + r < blowup) return 1;

	v2x = 0.25f - v2x;
	v2y = 0.25f - v2y;
	v2z = 0.25f - v2z;
	sum = v2x + v2y + v2z;
	if (sum - 0.1445 - r < blowup) return 2;

	int hue;
	float width;
	if (fabsf(sum - 3.0f * r - 0.375f) < 0.03846f + blowup)
	{
		width = 0.1445f;
		hue = 4;
	}
	else
	{
		width = 0.0676f;
		hue = 3;
	}

	float dx = fabsf(v2z - v2x);
	float dy = fabsf(v2x - v2y);
	float dz = fabsf(v2y - v2z);
	sum = dx + dy + dz;
	if (sum - width < blowup) return hue;

	return 0;
}

typedef struct PulsState
{
	float t;
	float t_param;
	float sin_a, cos_a;
	float3 pos;
} PulsState;

static int trace_ray(const PulsState* st, float x, float y)
{
	float3 pos = st->pos;
	float3 dir = { x, -y, 0.33594f - x * x - y * y };
	dir = (float3){ dir.y, dir.z * st->cos_a - dir.x * st->sin_a, dir.x * st->cos_a + dir.z * st->sin_a };
	dir = (float3){ dir.y, dir.z * st->cos_a - dir.x * st->sin_a, dir.x * st->cos_a + dir.z * st->sin_a };
	dir = (float3){ dir.y, dir.z * st->cos_a - dir.x * st->sin_a, dir.x * st->cos_a + dir.z * st->sin_a };

	int stepshift = MAXSTEPSHIFT;

	int i = 0;
	int c;
	int j;

	for (j = 0; j < MAXSTEP; j++)
	{
		c = func(st->t_param, pos, stepshift);
		if (c > 0)
		{
			if (stepshift < MAXSTEPSHIFT) ++stepshift;
			float mul = kStepTable[stepshift];
			pos = v3_sub(pos, v3_mulfl(dir, mul));
		}
		else
		{
			if (stepshift >= 1) --stepshift;
			float mul = kStepTable[stepshift];
			pos = v3_add(pos, v3_mulfl(dir, mul));
			i++;
		}

		if (stepshift >= MAXSTEPSHIFT) break;
		if (i >= MAXITERS) break;
	}

	return stepshift * 31;
}

static void do_render(float crank_angle, float time, uint8_t* framebuffer, int framebuffer_stride)
{
	init_puls();

	PulsState st;
	st.t = time * 30.0f;
	st.t_param = 0.0769f * sinf(st.t * -0.0708f);
	st.sin_a = sinf(st.t * 0.00564f);
	st.cos_a = cosf(st.t * 0.00564f);
	st.pos.x = 0.5f + 0.0134f * st.t;
	st.pos.y = 1.1875f + 0.0134f * st.t;
	st.pos.z = 0.875f + 0.0134f * st.t;

	// trace one ray per 2x2 pixel block
	// x: -0.5 .. +0.5
	// y: -0.3 .. +0.3
	float dx = 2.0f / LCD_COLUMNS;
	float dy = 1.2f / LCD_ROWS;

	float y = 0.3f - dy * 0.5f;
	int pix_idx = 0;
	for (int py = 0; py < LCD_ROWS / 2; ++py, y -= dy)
	{
		float x = -0.5f + dx * 0.5f;

		for (int px = 0; px < LCD_COLUMNS / 2; ++px, x += dx, ++pix_idx)
		{
			int val = trace_ray(&st, x, y);
			g_screen_buffer[pix_idx] = val;
		}
	}

	draw_dithered_screen_2x2(framebuffer, 1);
}


static int fx_puls_update(uint32_t buttons_cur, uint32_t buttons_pressed, float crank_angle, float time, uint8_t* framebuffer, int framebuffer_stride)
{
	do_render(crank_angle, time, framebuffer, framebuffer_stride);
	return MAXITERS;
}

Effect fx_puls_init(void* pd_api)
{
	return (Effect) { fx_puls_update };
}
