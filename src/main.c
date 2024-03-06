
#include "effects/fx.h"

#include "pd_api.h"
#include "allocator.h"
#include "util/pixel_ops.h"

typedef enum
{
	kFxPlanes = 0,
	kFxStarfield,
	kFxPlasma,
	kFxBlobs,
	kFxCount,
} EffectType;

static Effect s_effects[kFxCount];

static EffectType s_cur_effect = kFxBlobs;

Effect fx_planes_init(void* pd_api);
Effect fx_starfield_init(void* pd_api);
Effect fx_plasma_init(void* pd_api);
Effect fx_blobs_init(void* pd_api);


static int update(void* userdata);

const char* fontpath = "/System/Fonts/Roobert-10-Bold.pft";
LCDFont* font = NULL;

#ifdef _WINDLL
__declspec(dllexport)
#endif
int eventHandler(PlaydateAPI* pd, PDSystemEvent event, uint32_t arg)
{
	if (event == kEventInit)
	{
		const char* err;
		font = pd->graphics->loadFont(fontpath, &err);	
		if (font == NULL)
			pd->system->error("%s:%i Couldn't load font %s: %s", __FILE__, __LINE__, fontpath, err);

		pd_realloc = pd->system->realloc;

		init_blue_noise(pd);

		s_effects[kFxPlanes] = fx_planes_init(pd);
		s_effects[kFxStarfield] = fx_starfield_init(pd);
		s_effects[kFxPlasma] = fx_plasma_init(pd);
		s_effects[kFxBlobs] = fx_blobs_init(pd);

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
