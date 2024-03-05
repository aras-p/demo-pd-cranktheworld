#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define M_PIf 3.14159265f

#include "pd_api.h"
#include "mini3d/scene.h"

static Point3D plane_vb[] = {
 1.000000f, -1.000000f,  1.000000f,
-1.000000f, -1.000000f, -1.000000f,
-0.754643f,  0.700373f,  1.000000f,
-0.654210f, -0.654210f,  2.619101f,
-0.480073f,  0.111070f,  2.879520f,
 0.480073f,  0.111070f,  2.879520f,
 0.654210f, -0.654210f,  2.619101f,
-1.000000f, -1.000000f,  1.000000f,
 0.754643f,  0.408254f, -1.529500f,
 1.000000f, -1.000000f, -1.000000f,
 0.754643f,  0.700373f,  1.000000f,
-0.754643f,  0.408254f, -1.529500f,
-1.283430f, -0.567080f, -1.000000f,
-1.283430f, -0.567080f,  1.000000f,
 1.283430f,  0.050230f, -1.000000f,
 1.283430f, -0.567080f, -1.000000f,
-1.283430f,  0.050230f,  1.000000f,
 1.283430f,  0.050230f,  1.000000f,
 1.283430f, -0.567080f,  1.000000f,
-1.283430f,  0.050230f, -1.000000f,
-3.291110f, -0.774914f, -1.192131f,
-3.291110f, -0.774914f, -0.118306f,
 3.291110f, -0.447044f, -1.192131f,
 3.291110f, -0.774914f, -1.192131f,
-3.291110f, -0.447044f, -0.118306f,
 3.291110f, -0.447044f, -0.118306f,
 3.291110f, -0.774914f, -0.118306f,
-3.291110f, -0.447044f, -1.192131f,
};

static uint16_t plane_ib[] = {
12, 11, 9,
10, 8, 2,
12, 10, 2,
6, 4, 7,
17, 5, 3,
1, 4, 8,
3, 6, 11,
3, 20, 17,
9, 16, 10,
14, 4, 5,
15, 26, 23,
16, 27, 19,
12, 13, 20,
19, 7, 1,
8, 13, 2,
10, 19, 1,
11, 15, 9,
18, 6, 19,
25, 21, 22,
23, 27, 24,
19, 26, 18,
16, 23, 24,
20, 25, 17,
13, 28, 20,
14, 25, 22,
13, 22, 21,
12, 3, 11,
10, 1, 8,
12, 9, 10,
6, 5, 4,
17, 14, 5,
1, 7, 4,
3, 5, 6,
3, 12, 20,
9, 15, 16,
14, 8, 4,
15, 18, 26,
16, 24, 27,
12, 2, 13,
19, 6, 7,
8, 14, 13,
10, 16, 19,
11, 18, 15,
18, 11, 6,
25, 28, 21,
23, 26, 27,
19, 27, 26,
16, 15, 23,
20, 28, 25,
13, 21, 28,
14, 17, 25,
13, 14, 22,
};

static int update(void* userdata);
const char* fontpath = "/System/Fonts/Roobert-10-Bold.pft";
LCDFont* font = NULL;

static Scene3D s_scene;
static Shape3D s_shape_plane;


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

		mini3d_setRealloc(pd->system->realloc);
		Scene3D_init(&s_scene);
		Shape3D_init(&s_shape_plane, sizeof(plane_vb)/sizeof(plane_vb[0]), plane_vb, sizeof(plane_ib) / sizeof(plane_ib[0]) / 3, plane_ib, NULL);
		Scene3DNode_addShape(&s_scene.root, &s_shape_plane);
		Scene3DNode_setRenderStyle(&s_scene.root, kRenderFilled | kRenderWireframe);
		Scene3D_setGlobalLight(&s_scene, Vector3DMake(0.3f, 1.0f, 0.3f));

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

	/*
	for (int i = 0; i < triCount; i++)
	{
		int x1 = XorShift32(&rng) % LCD_COLUMNS;
		int y1 = XorShift32(&rng) % LCD_ROWS;
		int x2 = XorShift32(&rng) % LCD_COLUMNS;
		int y2 = XorShift32(&rng) % LCD_ROWS;
		int x3 = XorShift32(&rng) % LCD_COLUMNS;
		int y3 = XorShift32(&rng) % LCD_ROWS;
		int patIdx = i % 3;
		//pd->graphics->fillTriangle(x1, y1, x2, y2, x3, y3, (LCDColor)(patIdx == 0 ? pat_1 : patIdx==1 ? pat_2 : pat_3));
		Point3D pt1 = Point3DMake(x1, y1, 0);
		Point3D pt2 = Point3DMake(x2, y2, 0);
		Point3D pt3 = Point3DMake(x3, y3, 0);
		fillTriangle(pd->graphics->getFrame(), LCD_ROWSIZE, &pt1, &pt2, &pt3, patIdx == 0 ? pat_1 : patIdx == 1 ? pat_2 : pat_3);
	}
	*/

	float cangle = pd->system->getCrankAngle() * M_PIf / 180.0f;
	float cs = cosf(cangle);
	float ss = sinf(cangle);
	Scene3D_setCamera(&s_scene, Point3DMake(cs * 5.0f, 3.0f, ss * 5.0f), Point3DMake(0,0,0), 1.0f, Vector3DMake(0,-1,0));

	Scene3D_draw(&s_scene, pd->graphics->getFrame(), LCD_ROWSIZE);

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

