#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define M_PIf 3.14159265f

#include "pd_api.h"
#include "mini3d/scene.h"

static Point3D plane_vb[] = {
{ 1.000000f, -1.000000f,  1.000000f},
{-1.000000f, -1.000000f, -1.000000f},
{-0.754643f,  0.700373f,  1.000000f},
{-0.654210f, -0.654210f,  2.619101f},
{-0.480073f,  0.111070f,  2.879520f},
{ 0.480073f,  0.111070f,  2.879520f},
{ 0.654210f, -0.654210f,  2.619101f},
{-1.000000f, -1.000000f,  1.000000f},
{ 0.754643f,  0.408254f, -1.529500f},
{ 1.000000f, -1.000000f, -1.000000f},
{ 0.754643f,  0.700373f,  1.000000f},
{-0.754643f,  0.408254f, -1.529500f},
{-1.283430f, -0.567080f, -1.000000f},
{-1.283430f, -0.567080f,  1.000000f},
{ 1.283430f,  0.050230f, -1.000000f},
{ 1.283430f, -0.567080f, -1.000000f},
{-1.283430f,  0.050230f,  1.000000f},
{ 1.283430f,  0.050230f,  1.000000f},
{ 1.283430f, -0.567080f,  1.000000f},
{-1.283430f,  0.050230f, -1.000000f},
{-3.291110f, -0.774914f, -1.192131f},
{-3.291110f, -0.774914f, -0.118306f},
{ 3.291110f, -0.447044f, -1.192131f},
{ 3.291110f, -0.774914f, -1.192131f},
{-3.291110f, -0.447044f, -0.118306f},
{ 3.291110f, -0.447044f, -0.118306f},
{ 3.291110f, -0.774914f, -0.118306f},
{-3.291110f, -0.447044f, -1.192131f},
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
	if (event == kEventInit)
	{
		const char* err;
		font = pd->graphics->loadFont(fontpath, &err);		
		if (font == NULL)
			pd->system->error("%s:%i Couldn't load font %s: %s", __FILE__, __LINE__, fontpath, err);

		mini3d_setRealloc(pd->system->realloc);
		Scene3D_init(&s_scene);
		Shape3D_init(&s_shape_plane, sizeof(plane_vb)/sizeof(plane_vb[0]), plane_vb, sizeof(plane_ib) / sizeof(plane_ib[0]) / 3, plane_ib, NULL);
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

#define MAX_PLANES 500
static int planeCount = 10;
static Matrix3D planeXforms[MAX_PLANES];
static float planeDistances[MAX_PLANES];
static int planeOrder[MAX_PLANES];


static void checkCrank(PlaydateAPI *pd)
{
	PDButtons btCur, btPushed, btRel;
	pd->system->getButtonState(&btCur, &btPushed, &btRel);
	if (btCur & kButtonLeft)
	{
		planeCount -= 1;
		if (planeCount < 1)
			planeCount = 1;
	}
	if (btCur & kButtonRight)
	{
		planeCount += 1;
		if (planeCount > MAX_PLANES)
			planeCount = MAX_PLANES;
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

static int CompareZ(const void* a, const void* b)
{
	int ia = *(const int*)a;
	int ib = *(const int*)b;
	float za = planeDistances[ia];
	float zb = planeDistances[ib];
	if (za < zb)
		return +1;
	if (za > zb)
		return -1;
	return 0;
}

static int update(void* userdata)
{
	PlaydateAPI* pd = userdata;
	checkCrank(pd);

	pd->graphics->clear(kColorWhite);

	rng = 1;

	float cangle = pd->system->getCrankAngle() * M_PIf / 180.0f;
	float cs = cosf(cangle);
	float ss = sinf(cangle);
	Scene3D_setCamera(&s_scene, Point3DMake(cs * 8.0f, 3.0f, ss * 8.0f), Point3DMake(0,0,0), 1.0f, Vector3DMake(0,-1,0));

	// position and sort planes
	for (int i = 0; i < planeCount; ++i)
	{
		float px = ((XorShift32(&rng) & 63) - 31.5f);
		float py = ((XorShift32(&rng) & 15) - 7.5f);
		float pz = ((XorShift32(&rng) & 63) - 31.5f);
		float rot = (XorShift32(&rng) % 360);
		float rx = ((XorShift32(&rng) & 63) - 31.5f);
		float ry = 60;
		float rz = ((XorShift32(&rng) & 63) - 31.5f);
		if (i == 0) {
			px = py = pz = 0.0f;
			rot = 0;
			rx = 0;
			ry = 1;
			rz = 0;
		}
		planeXforms[i] = Matrix3DMakeRotation(rot, Vector3DMake(rx,ry,rz));
		planeXforms[i].dx = px;
		planeXforms[i].dy = py;
		planeXforms[i].dz = pz;
		//planeXforms[i] = Matrix3DMakeTranslate(px, py, pz);
		Point3D center = Matrix3D_apply(s_scene.camera, Matrix3D_apply(planeXforms[i], s_shape_plane.center));
		planeDistances[i] = center.z;
		planeOrder[i] = i;
	}
	qsort(planeOrder, planeCount, sizeof(planeOrder[0]), CompareZ);

	// draw
	for (int i = 0; i < planeCount; ++i)
	{
		int idx = planeOrder[i];
		Scene3D_drawShape(&s_scene, pd->graphics->getFrame(), LCD_ROWSIZE, &s_shape_plane, &planeXforms[idx], kRenderFilled | kRenderWireframe, 0.0f);
	}

	pd->graphics->fillRect(0, 0, 30, 32, kColorWhite);

	char buf[100];
	buf[0] = planeCount / 100 % 10 + '0';
	buf[1] = planeCount / 10 % 10 + '0';
	buf[2] = planeCount / 1 % 10 + '0';
	buf[3] = 0;
	pd->graphics->setFont(font);
	pd->graphics->drawText(buf, strlen(buf), kASCIIEncoding, 0, 16);

	x += dx;
	y += dy;
	
	if (x < 0 || x > LCD_COLUMNS - TEXT_WIDTH)
		dx = -dx;
	
	if (y < 0 || y > LCD_ROWS - TEXT_HEIGHT)
		dy = -dy;
        
	pd->system->drawFPS(0,0);

	return 1;
}
