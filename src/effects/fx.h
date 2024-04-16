#pragma once

void fx_blobs_init();
void fx_planes_init();
void fx_plasma_init();
void fx_raytrace_init();
void fx_starfield_init();
void fx_voxel_init();

int fx_starfield_update(float alpha);
int fx_prettyhip_update(float alpha);
int fx_plasma_update(float alpha);

int fx_raymarch_update(float alpha);

int fx_blobs_update(float alpha);

int fx_kefren_update();
int fx_moire_update();
int fx_planes_update();
int fx_puls_update();
int fx_raytrace_update();
int fx_sponge_update();
int fx_voxel_update();

int fx_temporal_test_update();
