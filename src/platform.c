// SPDX-License-Identifier: Unlicense

#include "platform.h"

#if defined(BUILD_PLATFORM_PLAYDATE)

#include "pd_api.h"

static const char* kFontPath = "/System/Fonts/Roobert-10-Bold.pft";
static LCDFont* s_font = NULL;

void* (*plat_realloc)(void* ptr, size_t size);

static PlaydateAPI* s_pd;

void plat_init(void* data)
{
	s_pd = (PlaydateAPI*)data;
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
		plat_realloc = pd->system->realloc;

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

#elif defined(BUILD_PLATFORM_PC)
#else
#error Unknown platform! Needs to be playdate or pc.
#endif
