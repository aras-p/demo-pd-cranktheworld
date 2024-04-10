
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

static EffectType s_cur_effect = kFxVariousTest;

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
		G.global_time = 0;
		G.fx_local_time = 0;
		G.fx_start_time = -1.0f;
		font = pd->graphics->loadFont(fontpath, &err);	
		if (font == NULL)
			pd->system->error("%s:%i Couldn't load font %s: %s", __FILE__, __LINE__, fontpath, err);

#if PLAY_MUSIC
		s_music_ok = false;
		s_music = pd->sound->fileplayer->newPlayer();
		s_music_ok = pd->sound->fileplayer->loadIntoPlayer(s_music, kMusicPath) != 0;
		if (s_music_ok)
			pd->sound->fileplayer->play(s_music, 0);
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

static int update(void* userdata)
{
	PDButtons btCur, btPushed, btRel;
	G.pd->system->getButtonState(&btCur, &btPushed, &btRel);
	G.buttons_cur = btCur;
	G.buttons_pressed = btPushed;
	G.crank_angle_rad = G.pd->system->getCrankAngle() * (M_PIf / 180.0f);
	G.global_time = G.pd->system->getElapsedTime() / TIME_UNIT_LENGTH_SECONDS;
#if PLAY_MUSIC
	if (s_music_ok)
		G.global_time = (float)G.pd->sound->getCurrentTime() / 82687.5f;
#endif
	if (G.fx_start_time < 0.0f)
		G.fx_start_time = G.global_time;

	if (btPushed & kButtonB) {
		G.fx_start_time = G.global_time;
		s_cur_effect--;
		if (s_cur_effect < 0 || s_cur_effect >= kFxCount)
			s_cur_effect = kFxCount-1;
	}
	if (btPushed & kButtonA) {
		G.fx_start_time = G.global_time;
		s_cur_effect++;
		if (s_cur_effect < 0 || s_cur_effect >= kFxCount)
			s_cur_effect = 0;
	}

	G.fx_local_time = G.global_time - G.fx_start_time;
	G.framebuffer = G.pd->graphics->getFrame();
	G.framebuffer_stride = LCD_ROWSIZE;

	G.pd->graphics->clear(kColorWhite);

	int dbg_value = 0;
	//pd->system->logToConsole("Time: %.1f", pd->system->getElapsedTime());
	dbg_value = s_effects[s_cur_effect].update();

	// draw FPS indicator and debug value
	G.pd->graphics->fillRect(0, 0, 40, 32, kColorWhite);

	char buf[100];
	buf[0] = dbg_value / 10000 % 10 + '0';
	buf[1] = dbg_value / 1000 % 10 + '0';
	buf[2] = dbg_value / 100 % 10 + '0';
	buf[3] = dbg_value / 10 % 10 + '0';
	buf[4] = dbg_value / 1 % 10 + '0';
	buf[5] = 0;
	G.pd->graphics->setFont(font);
	G.pd->graphics->drawText(buf, strlen(buf), kASCIIEncoding, 0, 16);

	G.pd->system->drawFPS(0,0);

	return 1;
}
