// SPDX-License-Identifier: Unlicense

#include "../globals.h"

#include "pd_api.h"
#include "../external/aheasing/easing.h"
#include "../mathlib.h"
#include "../util/pixel_ops.h"

typedef struct Star {
	float3 pos;
	float speed;
} Star;

#define STARS_START (100)
#define STARS_MID (3000)
#define STARS_END (8000)

static Star s_stars[STARS_END];

static void star_init(Star* s, uint32_t* rng)
{
	s->pos.x = RandomFloat01(rng) * 30000.0f - 15000.0f;
	s->pos.y = RandomFloat01(rng) * 30000.0f - 15000.0f;
	s->pos.z = RandomFloat01(rng) * 100 + 1;
	s->speed = RandomFloat01(rng) * 70 + 30;
}

void fx_starfield_update(float start_time, float end_time, float alpha)
{
	float dt = G.time - G.prev_time;

	if (G.ending)
		alpha = 1.0f - (cosf(G.crank_angle_rad) * 0.5f + 0.5f);

	float speed_a = CubicEaseIn(alpha) * 0.8f + 0.2f;
	dt *= speed_a;

	int draw_count;
	if (alpha < 0.5f)
	{
		draw_count = (int)lerp(STARS_START, STARS_MID, alpha * 2.0f);
		G.pd->graphics->clear(kColorWhite);
	}
	else
	{
		draw_count = (int)lerp(STARS_MID, STARS_END, (alpha - 0.5f) * 2.0f);
		if (G.beat || G.ending)
			G.pd->graphics->clear(kColorWhite);
	}

	for (int i = 0; i < draw_count; ++i)
	{
		Star* s = &s_stars[i];
		if (s->speed == 0) {
			star_init(s, &G.rng);
		}
		s->pos.z -= s->speed * dt;
		if (s->pos.z < 0) {
			star_init(s, &G.rng);
		}

		// project to screen
		int px = (int)(s->pos.x / s->pos.z + LCD_COLUMNS / 2 + 0.5f);
		int py = (int)(s->pos.y / s->pos.z + LCD_ROWS / 2 + 0.5f);
		if (px < 0 || px >= LCD_COLUMNS || py < 0 || py >= LCD_ROWS) {
			star_init(s, &G.rng);
			continue;
		}

		uint8_t* row = G.framebuffer + py * G.framebuffer_stride;
		put_pixel_black(row, px);
	}
}

void fx_starfield_init()
{
	for (int i = 0; i < STARS_END; ++i) {
		star_init(&s_stars[i], &G.rng);
	}
}
