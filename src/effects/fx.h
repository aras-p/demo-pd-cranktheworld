#pragma once

void fx_blobs_init();
void fx_plasma_init();
void fx_raytrace_init();
void fx_starfield_init();

typedef void (*fx_update_function)(float start_time, float end_time, float alpha);

void fx_starfield_update(float start_time, float end_time, float alpha);
void fx_prettyhip_update(float start_time, float end_time, float alpha);
void fx_plasma_update(float start_time, float end_time, float alpha);
void fx_blobs_update(float start_time, float end_time, float alpha);
void fx_raymarch_update(float start_time, float end_time, float alpha);
void fx_raytrace_update(float start_time, float end_time, float alpha);

int get_fade_bias(float start_time, float end_time);
