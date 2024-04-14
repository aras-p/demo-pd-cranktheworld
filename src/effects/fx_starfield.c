#include "../globals.h"

#include "pd_api.h"
#include "../mathlib.h"
#include "../util/pixel_ops.h"

typedef struct Star {
	float3 pos;
	float speed;
} Star;

#define MAX_STARS (20000) // 20k is still 30fps

static Star s_stars[MAX_STARS];
static int s_star_count = 3000;
static uint32_t s_rng = 1;
static float s_prev_time = -1;

static void star_init(Star* s, uint32_t* rng)
{
	s->pos.x = RandomFloat01(rng) * 30000.0f - 15000.0f;
	s->pos.y = RandomFloat01(rng) * 30000.0f - 15000.0f;
	s->pos.z = RandomFloat01(rng) * 100 + 1;
	s->speed = RandomFloat01(rng) * 70 + 30;
}

int fx_starfield_update()
{
	const int kStep = 251;
	if (G.buttons_cur & kButtonLeft)
	{
		s_star_count -= kStep;
		if (s_star_count < 50)
			s_star_count = 50;
	}
	if (G.buttons_cur & kButtonRight)
	{
		s_star_count += kStep;
		if (s_star_count > MAX_STARS)
			s_star_count = MAX_STARS;
	}

	float dt = G.time - s_prev_time;
	if (s_prev_time < 0 || s_prev_time >= G.time)
		dt = 0;
	s_prev_time = G.time;

	float fx_alpha = G.time / 32.0f * 0.7f + 0.3f;
	dt *= fx_alpha;
	int draw_star_count = (int)(s_star_count * fx_alpha);

	for (int i = 0; i < draw_star_count; ++i)
	{
		Star* s = &s_stars[i];
		if (s->speed == 0) {
			star_init(s, &s_rng);
		}
		s->pos.z -= s->speed * dt;
		if (s->pos.z < 0) {
			star_init(s, &s_rng);
		}

		// project to screen
		int px = (int)(s->pos.x / s->pos.z + LCD_COLUMNS / 2 + 0.5f);
		int py = (int)(s->pos.y / s->pos.z + LCD_ROWS / 2 + 0.5f);
		if (px < 0 || px >= LCD_COLUMNS || py < 0 || py >= LCD_ROWS) {
			star_init(s, &s_rng);
			continue;
		}

		uint8_t* row = G.framebuffer + py * G.framebuffer_stride;
		put_pixel_black(row, px);
	}

	return draw_star_count;
}

void fx_starfield_init()
{
	for (int i = 0; i < s_star_count; ++i) {
		star_init(&s_stars[i], &s_rng);
	}
}
