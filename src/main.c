
#include "effects/fx_planes.h"

#include "pd_api.h"

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

		mini3d_setRealloc(pd->system->realloc);

		fx_planes_init();

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

	pd->graphics->clear(kColorWhite);

	int dbg_value = 0;
	dbg_value = fx_planes_update(btCur, pd->system->getCrankAngle(), pd->system->getElapsedTime(), pd->graphics->getFrame(), LCD_ROWSIZE);

	pd->graphics->fillRect(0, 0, 30, 32, kColorWhite);

	char buf[100];
	buf[0] = dbg_value / 100 % 10 + '0';
	buf[1] = dbg_value / 10 % 10 + '0';
	buf[2] = dbg_value / 1 % 10 + '0';
	buf[3] = 0;
	pd->graphics->setFont(font);
	pd->graphics->drawText(buf, strlen(buf), kASCIIEncoding, 0, 16);

	pd->system->drawFPS(0,0);

	return 1;
}
