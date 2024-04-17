#include "../globals.h"

#include "pd_api.h"
#include "../mathlib.h"
#include "../util/pixel_ops.h"
#include "../external/aheasing/easing.h"
#include "../mini3d/render.h"

typedef struct TraceState
{
	float t;
	float rotmx, rotmy;

	float xor_rotmx, xor_rotmy;
	float xor_scale;
	float xor_camx, xor_camy;

	float sph_camx, sph_camy;
	float sph_camdist;
	float3 sponge_pos;
	float puls_t_param;
	float puls_cosa, puls_sina;
	float3 puls_pos;
} TraceState;


// ------------------------------------------
// "XOR Towers" by Greg Rostami https://www.shadertoy.com/view/7lsXR2 simplified - 10fps at 2x2t

static int trace_xor_towers(TraceState* st, float x, float y)
{
	x *= st->xor_scale;
	y *= st->xor_scale;
	float ux = st->xor_rotmy * x + st->xor_rotmx * y;
	float uy = st->xor_rotmx * x - st->xor_rotmy * y;

	float cx = st->xor_camx;
	float cy = st->xor_camy;

	int bx = 0, by = 0, bz = 0;

#define MAXSTEP 35
	float height = 0.0f;
	int it = 0;
	float heightstep = 0.3f;
	for (; it < MAXSTEP; ++it)
	{
		int tst = bx ^ by ^ bz;
		tst &= 63;
		if (tst < bz - 8)
			break;
		bx = (int)(ux * height + cx);
		by = (int)(uy * height + cy);
		bz = (int)height;
		height += heightstep;
		heightstep += 0.07f;
	}
	//return (int)(height * (255.0f / 70.0f));

	int res = (it - 10) * 10;
	res = MAX(0, res);
	res = MIN(255, res);
	return res;
#undef MAXSTEP
}


// ------------------------------------------
// Somewhat based on "Raymarch 180 chars" by coyote https://www.shadertoy.com/view/llfSzH simplified - 12fps at 2x2t, 21fps at 4x2t

static int trace_sphere_field(TraceState* st, float x, float y)
{
	float ux = st->rotmy * x + st->rotmx * y;
	float uy = st->rotmx * x - st->rotmy * y;
	float3 pos = { st->sph_camx, st->sph_camy, st->sph_camdist };
	float3 dir = { ux * 1.666f * 0.6f, uy * 1.666f * 0.6f, 2.0f * 0.6f };

	pos = v3_add(pos, dir);

#define MAXSTEP 15
	int it = 0;
	for (; it < MAXSTEP; ++it)
	{
		float3 rf = v3_subfl(v3_fract(pos), 0.5f);
		float d = v3_dot(rf, rf) - 0.1f;
		if (d < 0.01f)
			break;
		pos = v3_add(pos, v3_mulfl(dir, d * 1.5f));
	}
	return (int)(it * (255.0f / MAXSTEP));

#undef MAXSTEP
}
static int trace_octa_field(TraceState* st, float x, float y)
{
	float ux = st->rotmy * x + st->rotmx * y;
	float uy = st->rotmx * x - st->rotmy * y;
	float3 pos = { st->sph_camx, st->sph_camy, st->sph_camdist };
	float3 dir = { ux * 1.666f * 0.6f, uy * 1.666f * 0.6f, 2.0f * 0.6f };

	pos = v3_add(pos, dir);

#define MAXSTEP 15
	int it = 0;
	for (; it < MAXSTEP; ++it)
	{
		float3 rf = v3_subfl(v3_fract(pos), 0.5f);
		float d = fabsf(rf.x) + fabsf(rf.y) - 0.4f;
		if (d < 0.01f)
			break;
		pos = v3_add(pos, v3_mulfl(dir, d * 1.5f));
	}
	return (int)(it * (255.0f / MAXSTEP));

#undef MAXSTEP
}


// ------------------------------------------
// somewhat based on https://www.shadertoy.com/view/ldyGWm

#define SPONGE_MAX_TRACE_STEPS 8
#define SPONGE_FAR_DIST 5.0f

static float sponge_sdf(float3 q)
{
	// Layer one. The ".05" on the end varies the hole size.
	// p = abs(fract(q / 3) * 3 - 1.5)
	float3 p = v3_fract(v3_mulfl(q, 0.333333f));
	p = v3_abs(v3_subfl(v3_mulfl(p, 3.0f), 1.5f));
	float d = MIN(MAX(p.x, p.y), MIN(MAX(p.y, p.z), MAX(p.x, p.z))) - 1.0f + 0.05f;

	// Layer two
	p = v3_abs(v3_subfl(v3_fract(q), 0.5f));
	d = MAX(d, MIN(MAX(p.x, p.y), MIN(MAX(p.y, p.z), MAX(p.x, p.z))) - (1.0f / 3.0f) + 0.05f);

	return d;
}

static int trace_sponge(TraceState* st, float x, float y)
{
	x *= 3.3333f;
	y *= 3.3333f;

	float3 pos = st->sponge_pos;
	float3 dir = { x, y, 1.0f }; // do not normalize on purpose lol

	// Rotate the ray
	// dir.xy = mat2(m.y, -m.x, m)*dir.xy
	float nx = st->rotmy * dir.x + st->rotmx * dir.y;
	float ny = st->rotmx * dir.x - st->rotmy * dir.y;
	dir.x = nx;
	dir.y = ny;
	// dir.xz = mat2(m.y, -m.x, m)*dir.xz
	nx = st->rotmy * dir.x + st->rotmx * dir.z;
	ny = st->rotmx * dir.x - st->rotmy * dir.z;
	dir.x = nx;
	dir.z = ny;

	float t = 0.0f;
	int i;
	for (i = 0; i < SPONGE_MAX_TRACE_STEPS; ++i)
	{
		float3 q = v3_add(pos, v3_mulfl(dir, t));
		float d = sponge_sdf(q);
		if (d < t * 0.05f || d > SPONGE_FAR_DIST)
			break;
		t += d;
	}
	//return MIN((int)(t * 0.3f * 255.0f), 255);
	return 255 - i * 31;
}

// ------------------------------------------
// Puls by Rrrola "tribute", I guess?
// somewhat based on https://wakaba.c3.cx/w/puls.html

#define PULS_MAX_TRACE_STEPS 24

static float puls_sdf(float timeParam, float3 pos)
{
	float v2x = fabsf(fract(pos.x) - 0.5f) / 2.0f;
	float v2y = fabsf(fract(pos.y) - 0.5f) / 2.0f;
	float v2z = fabsf(fract(pos.z) - 0.5f) / 2.0f;
	float r = timeParam;

	float d1 = v2x + v2y + v2z - 0.1445f + r;

	v2x = 0.25f - v2x;
	v2y = 0.25f - v2y;
	v2z = 0.25f - v2z;
	float width = (0.08f - r * 2.0f) * 0.3f;
	float dx = fabsf(v2z - v2x);
	float dy = fabsf(v2x - v2y);
	float dz = fabsf(v2y - v2z);
	float d2 = dx + dy + dz - width;

	return MIN(d1, d2);
}

static int trace_puls(TraceState* st, float x, float y)
{
	float3 pos = st->puls_pos;
	float3 dir = { x, -y, 0.33594f - x * x - y * y };
	dir = (float3){ dir.y, dir.z * st->puls_cosa - dir.x * st->puls_sina, dir.x * st->puls_cosa + dir.z * st->puls_sina };
	dir = (float3){ dir.y, dir.z * st->puls_cosa - dir.x * st->puls_sina, dir.x * st->puls_cosa + dir.z * st->puls_sina };
	dir = (float3){ dir.y, dir.z * st->puls_cosa - dir.x * st->puls_sina, dir.x * st->puls_cosa + dir.z * st->puls_sina };

	float t = 0.0f;
	int i;
	for (i = 0; i < PULS_MAX_TRACE_STEPS; ++i)
	{
		float3 q = v3_add(pos, v3_mulfl(dir, t));
		float d = puls_sdf(st->puls_t_param, q);
		if (d < t * 0.07f)
			break;
		t += d * 1.7f;
	}

	//return (int)(((float)j) / (float)MAXSTEP * 255.0f);
	return MIN((int)(t * 0.25f * 255.0f), 255);
}


// ------------------------------------------


int fx_raymarch_update(float alpha, float prev_alpha)
{
	TraceState st;
	st.t = G.time;
	float r_angle = 0.6f - 0.1f * st.t + G.crank_angle_rad;
	st.rotmx = sinf(r_angle);
	st.rotmy = cosf(r_angle);

	float xor_r_angle = 0.6f - 0.03f * st.t + G.crank_angle_rad;
	st.xor_rotmx = sinf(xor_r_angle);
	st.xor_rotmy = cosf(xor_r_angle);
	st.xor_scale = 1.666f + sinf(G.time * 0.05f) * 0.3f;
	st.xor_camx = G.time * 0.4f;
	st.xor_camy = G.time * 1.7f;

	st.sph_camx = cosf(st.t * 0.3f + 1.66f);
	st.sph_camy = cosf(st.t * 0.23f + 1.0f);
	st.sph_camdist = cosf(st.t * 0.11f);

	st.sponge_pos = (float3){0.0f, 0.0f, G.time * 0.5f};

	float pulst = G.time * 5.0f;
	//st.t = pulst;
	st.puls_t_param = 0.0769f * sinf(pulst * 2.0f * -0.0708f);
	st.puls_sina = sinf(pulst * 0.00564f);
	st.puls_cosa = cosf(pulst * 0.00564f);
	st.puls_pos.x = 0.5f + 0.0134f * pulst;
	st.puls_pos.y = 1.1875f + 0.0134f * pulst;
	st.puls_pos.z = 0.875f + 0.0134f * pulst;

	float section_flt = alpha * 9.0f;
	int section_idx = (int)section_flt;
	float section_alpha = section_flt - section_idx;
	//section_idx = 4;

	float transition_in = 1.0f;
	if (section_alpha < 1.0f/8.0f)
		transition_in = CubicEaseInOut(section_alpha * 8.0f);
	int transition_x = (int)(LCD_COLUMNS / 4 * transition_in);
	int transition_y = (int)(LCD_ROWS / 4 * transition_in);
	float divider_angle1 = section_alpha * M_PIf * 1.0f;
	float divider_angle2 = divider_angle1 + M_PIf * 0.5f;
	float divider_dx1 = cosf(divider_angle1);
	float divider_dy1 = sinf(divider_angle1);
	float divider_dx2 = cosf(divider_angle2);
	float divider_dy2 = sinf(divider_angle2);

	float xsize = 1.0f;
	float ysize = 0.6f;
	float dx = xsize / LCD_COLUMNS;
	float dy = ysize / LCD_ROWS;

	// temporal: one ray for each 2x2 block, and also update one pixel within each 2x2 macroblock (16x fewer rays): 28fps (35ms)
	float y = ysize / 2 - dy;
	for (int py = 0; py < LCD_ROWS / 2; ++py, y -= dy * 2)
	{
		// Temporal: every frame update just one out of every 2x2 pixel blocks.
		// Which means every other frame we skip every other row.
		if ((G.frame_count & 1) == (py & 1))
			continue;

		float x = -xsize / 2 + dx;
		int pix_idx = py * LCD_COLUMNS / 2;
		// And for each row we step at 2 pixels, but shift location by one every other frame.
		if ((G.frame_count & 2)) {
			x += dx;
			pix_idx++;
		}
		
		if (section_idx == 0) // octa field
		{
			for (int px = 0; px < LCD_COLUMNS / 2; px += 2, x += dx * 4, pix_idx += 2)
			{
				int val = trace_octa_field(&st, x, y);
				g_screen_buffer_2x2sml[pix_idx] = val;
			}
		}
		else if (section_idx == 1) // sphere field
		{
			for (int px = 0; px < LCD_COLUMNS / 2; px += 2, x += dx * 4, pix_idx += 2)
			{
				int val = trace_sphere_field(&st, x, y);
				g_screen_buffer_2x2sml[pix_idx] = val;
			}
		}
		else if (section_idx == 2) // xor towers
		{
			for (int px = 0; px < LCD_COLUMNS / 2; px += 2, x += dx * 4, pix_idx += 2)
			{
				int val = trace_xor_towers(&st, x, y);
				g_screen_buffer_2x2sml[pix_idx] = val;
			}
		}
		else if (section_idx == 3) // sponge
		{
			for (int px = 0; px < LCD_COLUMNS / 2; px += 2, x += dx * 4, pix_idx += 2)
			{
				int val = trace_sponge(&st, x, y);
				g_screen_buffer_2x2sml[pix_idx] = val;
			}
		}
		else if (section_idx == 4) // puls
		{
			for (int px = 0; px < LCD_COLUMNS / 2; px += 2, x += dx * 4, pix_idx += 2)
			{
				int val = trace_puls(&st, x, y);
				g_screen_buffer_2x2sml[pix_idx] = val;
			}
		}
		else if (section_idx == 5) // top: sphere field, bottom: puls
		{
			for (int px = 0; px < LCD_COLUMNS / 2; px += 2, x += dx * 4, pix_idx += 2)
			{
				int val;
				if (py < transition_y)
					val = trace_sphere_field(&st, x, y);
				else
					val = trace_puls(&st, x, y);
				g_screen_buffer_2x2sml[pix_idx] = val;
			}
		}
		else if (section_idx == 6) // top: sponge, sphere field, bottom: octa field, puls
		{
			for (int px = 0; px < LCD_COLUMNS / 2; px += 2, x += dx * 4, pix_idx += 2)
			{
				int val;
				if (py < LCD_ROWS / 4)
				{
					if (px < transition_x)
						val = trace_sponge(&st, x, y);
					else
						val = trace_sphere_field(&st, x, y);
				}
				else
				{
					if (px < transition_x)
						val = trace_octa_field(&st, x, y);
					else
						val = trace_puls(&st, x, y);
				}
				g_screen_buffer_2x2sml[pix_idx] = val;
			}
		}
		else if (section_idx == 7) // same as above, divider lines rotating
		{
			float pdy = py - LCD_ROWS / 4;
			for (int px = 0; px < LCD_COLUMNS / 2; px += 2, x += dx * 4, pix_idx += 2)
			{
				float pdx = px - LCD_COLUMNS / 4;
				float det1 = divider_dx1 * pdy - divider_dy1 * pdx;
				float det2 = divider_dx2 * pdy - divider_dy2 * pdx;
				int quad_index = (det1 >= 0.0f ? 0 : 1) + (det2 >= 0.0f ? 2 : 0);

				int val;
				if (quad_index == 0)
					val = trace_puls(&st, x, y);
				else if (quad_index == 1)
					val = trace_sphere_field(&st, x, y);
				else if (quad_index == 2)
					val = trace_octa_field(&st, x, y);
				else
					val = trace_sponge(&st, x, y);
				g_screen_buffer_2x2sml[pix_idx] = val;
			}
		}
	}
	memcpy(g_screen_buffer, g_screen_buffer_2x2sml, LCD_COLUMNS / 2 * LCD_ROWS / 2);
	draw_dithered_screen_2x2(G.framebuffer, 1);

	// draw divider lines
	float3 linea, lineb;
	linea.z = lineb.z = 0;
	uint8_t pattern[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	if (section_idx == 5)
	{
		linea.x = 0; lineb.x = LCD_COLUMNS - 1;
		linea.y = lineb.y = transition_y * 2;
		drawLine(G.framebuffer, G.framebuffer_stride, &linea, &lineb, 1, pattern);
	}
	if (section_idx == 6)
	{
		linea.x = 0; lineb.x = LCD_COLUMNS - 1;
		linea.y = lineb.y = LCD_ROWS/2;
		drawLine(G.framebuffer, G.framebuffer_stride, &linea, &lineb, 1, pattern);

		linea.x = lineb.x = transition_x * 2;
		linea.y = 0; lineb.y = LCD_ROWS-1;
		drawLine(G.framebuffer, G.framebuffer_stride, &linea, &lineb, 1, pattern);
	}
	if (section_idx == 7)
	{
		linea.x = LCD_COLUMNS / 2 + divider_dx1 * LCD_COLUMNS;
		linea.y = LCD_ROWS / 2 + divider_dy1 * LCD_COLUMNS;
		lineb.x = LCD_COLUMNS / 2 - divider_dx1 * LCD_COLUMNS;
		lineb.y = LCD_ROWS / 2 - divider_dy1 * LCD_COLUMNS;
		drawLine(G.framebuffer, G.framebuffer_stride, &linea, &lineb, 1, pattern);

		linea.x = LCD_COLUMNS / 2 + divider_dx2 * LCD_COLUMNS;
		linea.y = LCD_ROWS / 2 + divider_dy2 * LCD_COLUMNS;
		lineb.x = LCD_COLUMNS / 2 - divider_dx2 * LCD_COLUMNS;
		lineb.y = LCD_ROWS / 2 - divider_dy2 * LCD_COLUMNS;
		drawLine(G.framebuffer, G.framebuffer_stride, &linea, &lineb, 1, pattern);
	}

	return 0;
}
