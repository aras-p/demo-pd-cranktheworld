// SPDX-License-Identifier: Unlicense

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define SCREEN_X	400
#define SCREEN_Y	240
#define SCREEN_STRIDE_BYTES 52

typedef enum { // match order or LCDSolidColor
	kSolidColorBlack,
	kSolidColorWhite,
} SolidColor;

typedef enum // match order of PDButtons
{
	kPlatButtonLeft = (1 << 0),
	kPlatButtonRight = (1 << 1),
	kPlatButtonUp = (1 << 2),
	kPlatButtonDown = (1 << 3),
	kPlatButtonB = (1 << 4),
	kPlatButtonA = (1 << 5)
} PlatButtons;

typedef struct PlatBitmap PlatBitmap;
typedef struct PlatFile PlatFile;
typedef struct PlatFileMusicPlayer PlatFileMusicPlayer;

void plat_gfx_clear(SolidColor color);
uint8_t* plat_gfx_get_frame();
void plat_gfx_mark_updated_rows(int start, int end);
void plat_gfx_draw_stats(float par1);

PlatBitmap* plat_gfx_load_bitmap(const char* file_path, const char** outerr);
void plat_gfx_draw_bitmap(PlatBitmap* bitmap, int x, int y);

PlatFile* plat_file_open_read(const char* file_path);
int plat_file_read(PlatFile* file, void* buf, uint32_t len);
int plat_file_seek_cur(PlatFile* file, int pos);
void plat_file_close(PlatFile* file);

PlatFileMusicPlayer* plat_audio_play_file(const char* file_path);
bool plat_audio_is_playing(PlatFileMusicPlayer* music);
float plat_audio_get_time(PlatFileMusicPlayer* music);
void plat_audio_set_time(PlatFileMusicPlayer* music, float t);

float plat_time_get();
void plat_time_reset();

void plat_input_get_buttons(PlatButtons* current, PlatButtons* pushed, PlatButtons* released);
float plat_input_get_crank_angle_rad();


void plat_sys_log_error(const char* fmt, ...);

void* plat_malloc(size_t size);
void* plat_realloc(void* ptr, size_t size);
void plat_free(void* ptr);
