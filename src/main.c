
#include "effects/fx.h"

#include "pd_api.h"
#include "mathlib.h"
#include "allocator.h"
#include "util/pixel_ops.h"

typedef enum
{
	kFxPlanes = 0,
	kFxStarfield,
	kFxPlasma,
	kFxBlobs,
	kFxMoire,
	kFxRaytrace,
	kFxKefren,
	kFxVoxel,
	kFxPuls,
	kFxSponge,
	kFxTemporalTest,
	kFxVariousTest,
	kFxCount,
} EffectType;

static Effect s_effects[kFxCount];

static EffectType s_cur_effect = kFxStarfield;

Effect fx_planes_init();
Effect fx_starfield_init();
Effect fx_plasma_init();
Effect fx_blobs_init();
Effect fx_moire_init();
Effect fx_raytrace_init();
Effect fx_kefren_init();
Effect fx_voxel_init();
Effect fx_puls_init();
Effect fx_sponge_init();
Effect fx_temporal_test_init();
Effect fx_various_test_init();

static int update(void* userdata);

const char* fontpath = "/System/Fonts/Roobert-10-Bold.pft";
LCDFont* font = NULL;

#define PLAY_MUSIC 1
#if PLAY_MUSIC
static const char* kMusicPath = "music.pda";
static FilePlayer* s_music;
//static int32_t s_music_offset_samples;
static bool s_music_ok;
#endif

#ifdef _WINDLL
__declspec(dllexport)
#endif
int eventHandler(PlaydateAPI* pd, PDSystemEvent event, uint32_t arg)
{
	if (event == kEventInit)
	{
		const char* err;
		G.pd = pd;
		G.time = -1.0f;
		font = pd->graphics->loadFont(fontpath, &err);	
		if (font == NULL)
			pd->system->error("%s:%i Couldn't load font %s: %s", __FILE__, __LINE__, fontpath, err);

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

		init_blue_noise();

		s_effects[kFxPlanes] = fx_planes_init();
		s_effects[kFxStarfield] = fx_starfield_init();
		s_effects[kFxPlasma] = fx_plasma_init();
		s_effects[kFxBlobs] = fx_blobs_init();
		s_effects[kFxMoire] = fx_moire_init();
		s_effects[kFxRaytrace] = fx_raytrace_init();
		s_effects[kFxKefren] = fx_kefren_init();
		s_effects[kFxVoxel] = fx_voxel_init();
		s_effects[kFxPuls] = fx_puls_init();
		s_effects[kFxSponge] = fx_sponge_init();
		s_effects[kFxTemporalTest] = fx_temporal_test_init();
		s_effects[kFxVariousTest] = fx_various_test_init();

		pd->system->resetElapsedTime();
		pd->system->setUpdateCallback(update, pd);
	}
	
	return 0;
}

static int s_beat_frame_done = -1;

static int track_current_time()
{
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


	// "beat" is if during this frame the tick would change
	int beat_at_end_of_frame = (int)(G.time + TIME_LEN_30FPSFRAME);
	G.beat = true;
	if (s_beat_frame_done >= beat_at_end_of_frame)
		G.beat = false;
	return beat_at_end_of_frame;
}

static int update(void* userdata)
{
	PDButtons btCur, btPushed, btRel;
	G.pd->system->getButtonState(&btCur, &btPushed, &btRel);
	G.buttons_cur = btCur;
	G.buttons_pressed = btPushed;
	G.crank_angle_rad = G.pd->system->getCrankAngle() * (M_PIf / 180.0f);
	int beat_at_end_of_frame = track_current_time();

	// A/B switch between effects
	if (btPushed & kButtonB) {
		s_cur_effect--;
		if (s_cur_effect < 0 || s_cur_effect >= kFxCount)
			s_cur_effect = kFxCount-1;
	}
	if (btPushed & kButtonA) {
		s_cur_effect++;
		if (s_cur_effect < 0 || s_cur_effect >= kFxCount)
			s_cur_effect = 0;
	}

	G.framebuffer = G.pd->graphics->getFrame();
	G.framebuffer_stride = LCD_ROWSIZE;

	G.pd->graphics->clear(kColorWhite);

	// update the effect
	int dbg_value = 0;
	//pd->system->logToConsole("Time: %.1f", pd->system->getElapsedTime());
	dbg_value = s_effects[s_cur_effect].update();

	s_beat_frame_done = beat_at_end_of_frame;

	// draw FPS, time, debug values
	G.pd->graphics->fillRect(0, 0, 70, 32, kColorWhite);

	char* buf;
	int bufLen = G.pd->system->formatString(&buf, "t %i b %i v %i", (int)G.time, G.beat, dbg_value);
	G.pd->graphics->setFont(font);
	G.pd->graphics->drawText(buf, bufLen, kASCIIEncoding, 0, 16);
	pd_free(buf);

	G.pd->system->drawFPS(0,0);

	return 1;
}
