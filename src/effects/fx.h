#pragma once

void fx_blobs_init();
void fx_plasma_init();
void fx_raytrace_init();
void fx_starfield_init();

int fx_starfield_update(float alpha);
int fx_prettyhip_update(float alpha);
int fx_plasma_update(float alpha);
int fx_blobs_update(float alpha);

int fx_raymarch_update(float alpha, float prev_alpha);
int fx_raytrace_update(float rel_time, float alpha);
