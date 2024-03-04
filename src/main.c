#include <stdio.h>
#include <stdlib.h>

#include "pd_api.h"

static int update(void* userdata);
const char* fontpath = "/System/Fonts/Roobert-10-Bold.pft";
LCDFont* font = NULL;

#ifdef _WINDLL
__declspec(dllexport)
#endif
int eventHandler(PlaydateAPI* pd, PDSystemEvent event, uint32_t arg)
{
	(void)arg; // arg is currently only used for event = kEventKeyPressed

	if (event == kEventInit)
	{
		const char* err;
		font = pd->graphics->loadFont(fontpath, &err);		
		if (font == NULL)
			pd->system->error("%s:%i Couldn't load font %s: %s", __FILE__, __LINE__, fontpath, err);
		pd->system->setUpdateCallback(update, pd);
	}
	
	return 0;
}


#define TEXT_WIDTH 86
#define TEXT_HEIGHT 16

static int x = (400-TEXT_WIDTH)/2;
static int y = (240-TEXT_HEIGHT)/2;
static int dx = 1;
static int dy = 2;

static LCDPattern pat_1 = LCDOpaquePattern(0x11, 0x33, 0x77, 0x80, 0x81, 0x83, 0x87, 0xff);
static LCDPattern pat_2 = LCDOpaquePattern(0x11, 0x22, 0x44, 0x88, 0x81, 0x42, 0x24, 0x18);
static LCDPattern pat_3 = LCDOpaquePattern(0x55, 0x55, 0x55, 0x55, 0xaa, 0xaa, 0xaa, 0xaa);

static int triCount = 2;

static void checkCrank(PlaydateAPI *pd)
{
	float change = pd->system->getCrankChange();

	if (change > 1) {
		triCount += 1;
		if (triCount > 9999)
			triCount = 9999;
		//pd->system->logToConsole("Tri count: %d", triCount);
	}
	else if (change < -1) {
		triCount -= 1;
		if (triCount < 2) { triCount = 2; }
		//pd->system->logToConsole("Tri count: %d", triCount);
	}
}

static uint32_t XorShift32(uint32_t *state)
{
	uint32_t x = *state;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 15;
	*state = x;
	return x;
}

static uint32_t rng = 1;

static int update(void* userdata)
{
	PlaydateAPI* pd = userdata;
	checkCrank(pd);

	pd->graphics->clear(kColorWhite);

	rng = 1;

	for (int i = 0; i < triCount; i++)
	{
		int x1 = XorShift32(&rng) % LCD_COLUMNS;
		int y1 = XorShift32(&rng) % LCD_ROWS;
		int x2 = XorShift32(&rng) % LCD_COLUMNS;
		int y2 = XorShift32(&rng) % LCD_ROWS;
		int x3 = XorShift32(&rng) % LCD_COLUMNS;
		int y3 = XorShift32(&rng) % LCD_ROWS;
		int patIdx = i % 3;
		pd->graphics->fillTriangle(x1, y1, x2, y2, x3, y3, (LCDColor)(patIdx == 0 ? pat_1 : patIdx==1 ? pat_2 : pat_3));
	}

	pd->graphics->fillRect(0, 0, 100, 48, kColorWhite);

	char buf[100];
	buf[0] = triCount / 1000 % 10 + '0';
	buf[1] = triCount / 100 % 10 + '0';
	buf[2] = triCount / 10 % 10 + '0';
	buf[3] = triCount / 1 % 10 + '0';
	buf[4] = 0;
	pd->graphics->setFont(font);
	pd->graphics->drawText(buf, strlen(buf), kASCIIEncoding, 0, 20);

	x += dx;
	y += dy;
	
	if (x < 0 || x > LCD_COLUMNS - TEXT_WIDTH)
		dx = -dx;
	
	if (y < 0 || y > LCD_ROWS - TEXT_HEIGHT)
		dy = -dy;
        
	pd->system->drawFPS(0,0);

	return 1;
}

