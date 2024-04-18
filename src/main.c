

#include "pd_api.h"

#include "allocator.h"
#include "effects/fx.h"
#include "globals.h"
#include "mathlib.h"
#include "util/pixel_ops.h"

static int update(void* userdata);


static const char* kFontPath = "/System/Fonts/Roobert-10-Bold.pft";
static LCDFont* s_font = NULL;

#define PLAY_MUSIC 1
#if PLAY_MUSIC
static const char* kMusicPath = "music.pda";
static FilePlayer* s_music;
static bool s_music_ok;
#endif

typedef struct DemoImage {
	const char* file;
	int x, y;
	int tstart, tend;
	LCDBitmap* bitmap;
} DemoImage;

static DemoImage s_images[] = {
	{"text_everybody.pdi", 105, 8, 16, 32 },
	{"text_wantsto.pdi", 120, 56, 17, 32 },
	{"text_crank.pdi", 89, 103, 18, 32 },
	{"text_theworld.pdi", 136, 163, 19, 32 },
	{"text_instr.pdi", 5, 183, 300, 400 },
};
#define DEMO_IMAGE_COUNT (sizeof(s_images)/sizeof(s_images[0]))

#ifdef _WINDLL
__declspec(dllexport)
#endif
int eventHandler(PlaydateAPI* pd, PDSystemEvent event, uint32_t arg)
{
	if (event == kEventInit)
	{
		const char* err;
		G.rng = 1;
		G.pd = pd;
		G.frame_count = 0;
		G.time = G.prev_time = G.prev_prev_time = -1.0f;
		s_font = pd->graphics->loadFont(kFontPath, &err);
		if (s_font == NULL)
			pd->system->error("Could not load font %s: %s", kFontPath, err);

		for (int i = 0; i < DEMO_IMAGE_COUNT; ++i)
		{
			s_images[i].bitmap = pd->graphics->loadBitmap(s_images[i].file, &err);
			if (s_images[i].bitmap == NULL)
				pd->system->error("Could not load bitmap %s: %s", s_images[i].file, err);
		}

#if PLAY_MUSIC
		s_music_ok = false;
		s_music = pd->sound->fileplayer->newPlayer();
		s_music_ok = pd->sound->fileplayer->loadIntoPlayer(s_music, kMusicPath) != 0;
		if (s_music_ok)
			pd->sound->fileplayer->play(s_music, 1);
		else
			pd->system->error("Could not load music file %s", kMusicPath);
#endif

		pd_realloc = pd->system->realloc;

		init_pixel_ops();
		fx_blobs_init();
		fx_planes_init();
		fx_plasma_init();
		fx_raytrace_init();
		fx_starfield_init();
		fx_voxel_init();

		pd->system->resetElapsedTime();
		pd->system->setUpdateCallback(update, pd);
	}
	
	return 0;
}

static int s_beat_frame_done = -1;

static int track_current_time()
{
	G.frame_count++;
	G.prev_prev_time = G.prev_time;
	G.prev_time = G.time;
	G.time = G.pd->system->getElapsedTime() / TIME_UNIT_LENGTH_SECONDS;
#if PLAY_MUSIC
	if (s_music_ok)
	{
		//G.time = (float)G.pd->sound->getCurrentTime() / 20671.875f;

		// Up/Down seek in time
		if (G.buttons_pressed & kButtonUp) {
			float offset = G.pd->sound->fileplayer->getOffset(s_music);
			offset -= 5.0f;
			offset = MAX(0, offset);
			G.pd->sound->fileplayer->setOffset(s_music, offset);
		}
		if (G.buttons_pressed & kButtonDown) {
			float offset = G.pd->sound->fileplayer->getOffset(s_music);
			offset += 5.0f;
			offset = MAX(0, offset);
			G.pd->sound->fileplayer->setOffset(s_music, offset);
		}
		G.time = G.pd->sound->fileplayer->getOffset(s_music) / TIME_UNIT_LENGTH_SECONDS;
	}
#endif

	if (G.prev_time > G.time)
		G.prev_time = G.time;
	if (G.prev_prev_time > G.prev_time)
		G.prev_prev_time = G.prev_time;

	// "beat" is if during this frame the tick would change
	int beat_at_end_of_frame = (int)(G.time + TIME_LEN_30FPSFRAME);
	G.beat = true;
	if (s_beat_frame_done >= beat_at_end_of_frame)
		G.beat = false;
	return beat_at_end_of_frame;
}

static int update_effect()
{
#if 0
	// A/B switch between effects
	if (G.buttons_pressed & kButtonB) {
		s_cur_effect--;
		if (s_cur_effect < 0 || s_cur_effect >= kFxCount)
			s_cur_effect = kFxCount - 1;
	}
	if (G.buttons_pressed & kButtonA) {
		s_cur_effect++;
		if (s_cur_effect < 0 || s_cur_effect >= kFxCount)
			s_cur_effect = 0;
	}
#endif

	int dbg_val = 0;
	float t = G.time;
	if (t < 32)
	{
		float a = t / 32.0f;
		dbg_val = fx_starfield_update(a);
	}
	else if (t < 64)
	{
		float a = (t - 32.0f) / 32.0f;
		dbg_val = fx_prettyhip_update(a);
	}
	else if (t < 80)
	{
		float a = invlerp(64.0f, 80.0f, t);
		dbg_val = fx_plasma_update(a);
	}
	else if (t < 96)
	{
		float a = invlerp(80.0f, 96.0f, t);
		dbg_val = fx_blobs_update(a);
	}
	else if (t < 240)
	{
		float a = invlerp(96.0f, 240.0f, t);
		float prev_a = invlerp(96.0f, 240.0f, G.prev_prev_time);
		dbg_val = fx_raymarch_update(a, prev_a);
	}
	else
	{
		float rt = t - 240.0f;
		float a = invlerp(240.0f, 304.0f, t);
		dbg_val = fx_raytrace_update(rt, a);
	}
	return dbg_val;
}

static void update_images()
{
	float t = G.time;
	for (int i = 0; i < DEMO_IMAGE_COUNT; ++i)
	{
		const DemoImage* img = &s_images[i];
		if (img->bitmap == NULL || t < img->tstart || t > img->tend)
			continue;
		G.pd->graphics->drawBitmap(img->bitmap, img->x, img->y, kBitmapUnflipped);
	}
}

static int update(void* userdata)
{
	// track inputs and time
	PDButtons btCur, btPushed, btRel;
	G.pd->system->getButtonState(&btCur, &btPushed, &btRel);
	G.buttons_cur = btCur;
	G.buttons_pressed = btPushed;
	G.crank_angle_rad = G.pd->system->getCrankAngle() * (M_PIf / 180.0f);

	int beat_at_end_of_frame = track_current_time();

	G.framebuffer = G.pd->graphics->getFrame();
	G.framebuffer_stride = LCD_ROWSIZE;

	// update the effect
	int dbg_value = update_effect();

	s_beat_frame_done = beat_at_end_of_frame;

	update_images();

	// draw FPS, time, debug values
	G.pd->graphics->fillRect(0, 0, 70, 32, kColorWhite);
	char* buf;
	int bufLen = G.pd->system->formatString(&buf, "t %i b %i v %i", (int)G.time, G.beat, dbg_value);
	G.pd->graphics->setFont(s_font);
	G.pd->graphics->drawText(buf, bufLen, kASCIIEncoding, 0, 16);
	pd_free(buf);
	G.pd->system->drawFPS(0,0);

	// tell OS that we've updated the whole screen
	G.pd->graphics->markUpdatedRows(0, LCD_ROWS-1);

	return 1;
}
