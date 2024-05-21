// SPDX-License-Identifier: Unlicense

#include "platform.h"
#include <stdarg.h>

// --------------------------------------------------------------------------
#if defined(BUILD_PLATFORM_PLAYDATE)

#include "pd_api.h"

static const char* kFontPath = "/System/Fonts/Roobert-10-Bold.pft";
static LCDFont* s_font = NULL;

static PlaydateAPI* s_pd;

void* plat_malloc(size_t size)
{
	return s_pd->system->realloc(NULL, size);
}
void* plat_realloc(void* ptr, size_t size)
{
	return s_pd->system->realloc(ptr, size);
}
void plat_free(void* ptr)
{
	if (ptr != NULL)
		s_pd->system->realloc(ptr, 0);
}

void plat_gfx_clear(SolidColor color)
{
	s_pd->graphics->clear((LCDColor)color);
}
uint8_t* plat_gfx_get_frame()
{
	return s_pd->graphics->getFrame();
}
void plat_gfx_mark_updated_rows(int start, int end)
{
	s_pd->graphics->markUpdatedRows(start, end);
}
void plat_gfx_draw_stats(float par1)
{
	s_pd->graphics->fillRect(0, 0, 40, 32, kColorWhite);
	char* buf;
	int bufLen = s_pd->system->formatString(&buf, "t %i", (int)par1);
	s_pd->graphics->setFont(s_font);
	s_pd->graphics->drawText(buf, bufLen, kASCIIEncoding, 0, 16);
	plat_free(buf);
	s_pd->system->drawFPS(0, 0);
}

PlatBitmap* plat_gfx_load_bitmap(const char* file_path, const char** outerr)
{
	return (PlatBitmap*)s_pd->graphics->loadBitmap(file_path, outerr);
}

void plat_gfx_draw_bitmap(PlatBitmap* bitmap, int x, int y)
{
	s_pd->graphics->drawBitmap((LCDBitmap*)bitmap, x, y, kBitmapUnflipped);
}

PlatFile* plat_file_open_read(const char* file_path)
{
	return (PlatFile*)s_pd->file->open(file_path, kFileRead);
}
int plat_file_read(PlatFile* file, void* buf, uint32_t len)
{
	return s_pd->file->read((SDFile*)file, buf, len);
}
int plat_file_seek_cur(PlatFile* file, int pos)
{
	return s_pd->file->seek((SDFile*)file, pos, SEEK_CUR);
}
void plat_file_close(PlatFile* file)
{
	s_pd->file->close((SDFile*)file);
}

static void plat_sys_log_error_impl(const char* fmt, va_list args)
{
	s_pd->system->error(fmt, args);
}

void plat_sys_log_error(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	plat_sys_log_error_impl(fmt, args);
	va_end(args);
}

PlatFileMusicPlayer* plat_audio_play_file(const char* file_path)
{
	FilePlayer* music = s_pd->sound->fileplayer->newPlayer();
	bool s_music_ok = s_pd->sound->fileplayer->loadIntoPlayer(music, file_path) != 0;
	if (s_music_ok)
	{
		s_pd->sound->fileplayer->play(music, 1);
		return (PlatFileMusicPlayer*)music;
	}
	return NULL;
}

bool plat_audio_is_playing(PlatFileMusicPlayer* music)
{
	return s_pd->sound->fileplayer->isPlaying((FilePlayer*)music) != 0;
}

float plat_audio_get_time(PlatFileMusicPlayer* music)
{
	return s_pd->sound->fileplayer->getOffset((FilePlayer*)music);
}

void plat_audio_set_time(PlatFileMusicPlayer* music, float t)
{
	s_pd->sound->fileplayer->setOffset((FilePlayer*)music, t);
}

float plat_time_get()
{
	return s_pd->system->getElapsedTime();
}

void plat_time_reset()
{
	s_pd->system->resetElapsedTime();
}

void plat_input_get_buttons(PlatButtons* current, PlatButtons* pushed, PlatButtons* released)
{
	PDButtons cur, push, rel;
	s_pd->system->getButtonState(&cur, &push, &rel);
	*current = (PlatButtons)cur;
	*pushed = (PlatButtons)push;
	*released = (PlatButtons)rel;
}

float plat_input_get_crank_angle_rad()
{
	return s_pd->system->getCrankAngle() * (3.14159265358979323846f / 180.0f);
}

void app_initialize();
void app_update();

static int eventUpdate(void* userdata)
{
	app_update();
	return 1;
}


// entry point
#ifdef _WINDLL
__declspec(dllexport)
#endif
int eventHandler(PlaydateAPI* pd, PDSystemEvent event, uint32_t arg)
{
	if (event == kEventInit)
	{
		s_pd = pd;

		const char* err;
		s_font = pd->graphics->loadFont(kFontPath, &err);
		if (s_font == NULL)
			pd->system->error("Could not load font %s: %s", kFontPath, err);

		app_initialize();
		pd->system->resetElapsedTime();
		pd->system->setUpdateCallback(eventUpdate, pd);
	}
	return 0;
}

// --------------------------------------------------------------------------
#elif defined(BUILD_PLATFORM_PC)

#define SOKOL_IMPL
#define SOKOL_D3D11
#include "external/sokol/sokol_app.h"
#include "external/sokol/sokol_gfx.h"
#include "external/sokol/sokol_time.h"
#include "external/sokol/sokol_audio.h"
#include "external/sokol/sokol_glue.h"

#include <stdio.h>
#include <stdlib.h>

static uint8_t s_screen_buffer[SCREEN_Y * SCREEN_STRIDE_BYTES];

void* plat_malloc(size_t size)
{
	return malloc(size);
}
void* plat_realloc(void* ptr, size_t size)
{
	return realloc(ptr, size);
}
void plat_free(void* ptr)
{
	if (ptr != NULL)
		free(ptr);
}

void plat_gfx_clear(SolidColor color)
{
	//@TODO
}
uint8_t* plat_gfx_get_frame()
{
	return s_screen_buffer;
}
void plat_gfx_mark_updated_rows(int start, int end)
{
}
void plat_gfx_draw_stats(float par1)
{
	//@TODO
}

PlatBitmap* plat_gfx_load_bitmap(const char* file_path, const char** outerr)
{
	//@TODO
	return NULL;
}

void plat_gfx_draw_bitmap(PlatBitmap* bitmap, int x, int y)
{
	//@TODO
}

PlatFile* plat_file_open_read(const char* file_path)
{
	return (PlatFile*)fopen(file_path, "rb");
}
int plat_file_read(PlatFile* file, void* buf, uint32_t len)
{
	return (int)fread(buf, 1, len, (FILE*)file);
}
int plat_file_seek_cur(PlatFile* file, int pos)
{
	return fseek((FILE*)file, pos, SEEK_CUR);
}
void plat_file_close(PlatFile* file)
{
	fclose((FILE*)file);
}

static void plat_sys_log_error_impl(const char* fmt, va_list args)
{
	//@TODO
}

void plat_sys_log_error(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	plat_sys_log_error_impl(fmt, args);
	va_end(args);
}

PlatFileMusicPlayer* plat_audio_play_file(const char* file_path)
{
	//@TODO
	return NULL;
}

bool plat_audio_is_playing(PlatFileMusicPlayer* music)
{
	//@TODO
	return true;
}

float plat_audio_get_time(PlatFileMusicPlayer* music)
{
	//@TODO
	return 0.5f;
}

void plat_audio_set_time(PlatFileMusicPlayer* music, float t)
{
	//@TODO
}

float plat_time_get()
{
	//@TODO
	return 0.5f;
}

void plat_time_reset()
{
	//@TODO
}

void plat_input_get_buttons(PlatButtons* current, PlatButtons* pushed, PlatButtons* released)
{
	//@TODO
	*current = 0;
	*pushed = 0;
	*released = 0;
}

float plat_input_get_crank_angle_rad()
{
	//@TODO
	return 0.0f;
}

// sokol_app setup

static void sapp_init(void)
{
	sg_setup(&(sg_desc) {
		.environment = sglue_environment()
	});
}

static void sapp_frame(void)
{

}

static void sapp_cleanup(void)
{

}

sapp_desc sokol_main(int argc, char* argv[]) {
	(void)argc; (void)argv;
	return (sapp_desc) {
		.init_cb = sapp_init,
		.frame_cb = sapp_frame,
		.cleanup_cb = sapp_cleanup,
		.width = SCREEN_X,
		.height = SCREEN_Y,
		.window_title = "Triangle",
		.icon.sokol_default = true
	};
}


// --------------------------------------------------------------------------
#else
#error Unknown platform! Needs to be playdate or pc.
#endif
