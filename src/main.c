
#include "effects/fx.h"

#include "pd_api.h"
#include "allocator.h"
#include "util/pixel_ops.h"

//#define PLAY_MUSIC_MP3 1
#define PLAY_MUSIC_XM 1

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
	kFxCount,
} EffectType;

static Effect s_effects[kFxCount];

static EffectType s_cur_effect = kFxRaytrace;

Effect fx_planes_init(void* pd_api);
Effect fx_starfield_init(void* pd_api);
Effect fx_plasma_init(void* pd_api);
Effect fx_blobs_init(void* pd_api);
Effect fx_moire_init(void* pd_api);
Effect fx_raytrace_init(void* pd_api);
Effect fx_kefren_init(void* pd_api);
Effect fx_voxel_init(void* pd_api);

static int update(void* userdata);

const char* fontpath = "/System/Fonts/Roobert-10-Bold.pft";
LCDFont* font = NULL;

#if PLAY_MUSIC_MP3
static const char* kMusicPath = "Music.mp3";
static FilePlayer* s_music;
#endif
#if PLAY_MUSIC_XM
#include "external/libxm/include/xm.h"
static const char* kMusicPath = "cybercul.xm";
static xm_context_t* s_music_player;
static char* s_music_data;
static SoundSource* s_music;

static int xm_AudioSourceFn(void* context, int16_t* left, int16_t* right, int len)
{
	xm_generate_samples_16_mono(s_music_player, left, len);
	return 1;
}
#endif

#ifdef _WINDLL
__declspec(dllexport)
#endif
int eventHandler(PlaydateAPI* pd, PDSystemEvent event, uint32_t arg)
{
	if (event == kEventInit)
	{
		pd_realloc = pd->system->realloc;

		const char* err;
		font = pd->graphics->loadFont(fontpath, &err);	
		if (font == NULL)
			pd->system->error("%s:%i Couldn't load font %s: %s", __FILE__, __LINE__, fontpath, err);

#		if PLAY_MUSIC_MP3
		s_music = pd->sound->fileplayer->newPlayer();
		int music_ok = pd->sound->fileplayer->loadIntoPlayer(s_music, kMusicPath);
		if (!music_ok) {
			pd->system->error("Could not load music file %s", kMusicPath);
		}
		pd->sound->fileplayer->play(s_music, 0);
#		endif

#		if PLAY_MUSIC_XM
		SDFile* xm_file = pd->file->open(kMusicPath, kFileRead);
		if (!xm_file) {
			pd->system->error("Could not load music file %s", kMusicPath);
		}
		else {
			FileStat st = {0};
			pd->file->stat(kMusicPath, &st);
			s_music_data = pd_malloc(st.size);
			pd->file->read(xm_file, s_music_data, st.size);
			pd->file->close(xm_file);

			int music_ok = xm_create_context_safe(&s_music_player, s_music_data, st.size, 44100);
			if (music_ok != 0) {
				pd->system->error("Could not play music file %s err %i", kMusicPath, music_ok);
			}
			else {
				s_music = pd->sound->addSource(xm_AudioSourceFn, NULL, 0);
			}
		}

#		endif

		init_blue_noise(pd);

		s_effects[kFxPlanes] = fx_planes_init(pd);
		s_effects[kFxStarfield] = fx_starfield_init(pd);
		s_effects[kFxPlasma] = fx_plasma_init(pd);
		s_effects[kFxBlobs] = fx_blobs_init(pd);
		s_effects[kFxMoire] = fx_moire_init(pd);
		s_effects[kFxRaytrace] = fx_raytrace_init(pd);
		s_effects[kFxKefren] = fx_kefren_init(pd);
		s_effects[kFxVoxel] = fx_voxel_init(pd);

		pd->system->resetElapsedTime();
		pd->system->setUpdateCallback(update, pd);
	}
	
	return 0;
}

static int update(void* userdata)
{
	PlaydateAPI* pd = userdata;

	PDButtons btCur, btPushed, btRel;
	pd->system->getButtonState(&btCur, &btPushed, &btRel);
	if (btPushed & kButtonB) {
		s_cur_effect--;
		if (s_cur_effect < 0)
			s_cur_effect = kFxCount-1;
	}
	if (btPushed & kButtonA) {
		s_cur_effect++;
		if (s_cur_effect >= kFxCount)
			s_cur_effect = 0;
	}

	pd->graphics->clear(kColorWhite);

	int dbg_value = 0;
	//pd->system->logToConsole("Time: %.1f", pd->system->getElapsedTime());
	dbg_value = s_effects[s_cur_effect].update(btCur, btPushed, pd->system->getCrankAngle(), pd->system->getElapsedTime(), pd->graphics->getFrame(), LCD_ROWSIZE);

	pd->graphics->fillRect(0, 0, 40, 32, kColorWhite);

	char buf[100];
	buf[0] = dbg_value / 10000 % 10 + '0';
	buf[1] = dbg_value / 1000 % 10 + '0';
	buf[2] = dbg_value / 100 % 10 + '0';
	buf[3] = dbg_value / 10 % 10 + '0';
	buf[4] = dbg_value / 1 % 10 + '0';
	buf[5] = 0;
	pd->graphics->setFont(font);
	pd->graphics->drawText(buf, strlen(buf), kASCIIEncoding, 0, 16);

	pd->system->drawFPS(0,0);

	return 1;
}
