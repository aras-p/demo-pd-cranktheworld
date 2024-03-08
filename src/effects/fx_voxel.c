#include "fx.h"

#include "pd_api.h"
#include "../mathlib.h"
#include "../allocator.h"
#include "../util/image_loader.h"
#include "../util/pixel_ops.h"

#define HEIGHTMAP_SHIFT 8
#define HEIGHTMAP_SIZE (1<<HEIGHTMAP_SHIFT)
#define HEIGHTMAP_MASK (HEIGHTMAP_SIZE-1)
static uint8_t* s_heightmap;
static uint8_t* s_heightmap_color;

static uint8_t s_buffer[LCD_COLUMNS * LCD_ROWS];

static void init_heightmap(void* pd_api)
{
	int img_w, img_h;
	s_heightmap = read_tga_file_grayscale("Heightmap.tga", pd_api, &img_w, &img_h);
	s_heightmap_color = read_tga_file_grayscale("HeightmapColor.tga", pd_api, &img_w, &img_h);
}

typedef struct Camera {
	float3 pos;
	float angle;
	float horizon;
	float distance;
} Camera;

static Camera s_camera = { {128,60,78}, 0, 100, 200 };

static int fx_voxel_update(uint32_t buttons_cur, uint32_t buttons_pressed, float crank_angle, float time, uint8_t* framebuffer, int framebuffer_stride)
{
	s_camera.angle = crank_angle * M_PIf / 180.0f;

	float cam_sin = sinf(s_camera.angle);
	float cam_cos = cosf(s_camera.angle);

	if (buttons_cur & kButtonUp)
	{
		s_camera.pos.x += cam_sin * 1.1f;
		s_camera.pos.z += cam_cos * 1.1f;
	}
	if (buttons_cur & kButtonDown)
	{
		s_camera.pos.x -= cam_sin * 1.1f;
		s_camera.pos.z -= cam_cos * 1.1f;
	}
	if (buttons_cur & kButtonLeft)
	{
		s_camera.distance -= 1;
		if (s_camera.distance < 20)
			s_camera.distance = 20;
	}
	if (buttons_cur & kButtonRight)
	{
		s_camera.distance += 1;
		if (s_camera.distance > 1000)
			s_camera.distance = 1000;
	}

	// collision with heightmap
	float cam_height = s_camera.pos.y;
	int cam_tex_index = ((((int)s_camera.pos.z) & HEIGHTMAP_MASK) << HEIGHTMAP_SHIFT) + (((int)s_camera.pos.x) & HEIGHTMAP_MASK);
	float min_height = s_heightmap[cam_tex_index] + 10.0f;
	if (min_height > cam_height) {
		cam_height = min_height;
	}

	int occlusion[LCD_COLUMNS];
	for (int i = 0; i < LCD_COLUMNS; ++i)
		occlusion[i] = LCD_ROWS;
	memset(s_buffer, 0xFF, sizeof(s_buffer));

	float deltaz = 1.0f;
	const float inv_columns = 1.0f / LCD_COLUMNS;

	// front to back
	for (float z = 1.0f; z < s_camera.distance; z += deltaz)
	{
		// camera uses 90 degree FOV
		float plx = -cam_cos * z - cam_sin * z;
		float ply = cam_sin * z - cam_cos * z;
		float prx = cam_cos * z - cam_sin * z;
		float pry = -cam_sin * z - cam_cos * z;

		float dx = (prx - plx) * inv_columns;
		float dy = (pry - ply) * inv_columns ;
		plx += s_camera.pos.x;
		ply += s_camera.pos.z;
		float invz = 1.0f / z * 100.0f;
		for (int x = 0; x < LCD_COLUMNS; x++) {
			int tex_index = ((((int)ply) & HEIGHTMAP_MASK) << HEIGHTMAP_SHIFT) + (((int)plx) & HEIGHTMAP_MASK);
			int screen_y = (int)((cam_height - s_heightmap[tex_index]) * invz + s_camera.horizon);
			if (screen_y < 0) screen_y = 0;
			for (int y = screen_y; y < occlusion[x]; ++y) {
				s_buffer[y * LCD_COLUMNS + x] = s_heightmap_color[tex_index]; // (uint8_t)(z * 2);
			}
			if (screen_y < occlusion[x]) {
				occlusion[x] = screen_y;
			}
			plx += dx;
			ply += dy;
		}
		deltaz += 0.005f;
	}

	for (int y = 0; y < LCD_ROWS; ++y) {
		draw_dithered_scanline(&s_buffer[y * LCD_COLUMNS], y, 0, framebuffer);
	}
	return 0;
}

Effect fx_voxel_init(void* pd_api)
{
	init_heightmap(pd_api);
	return (Effect) {fx_voxel_update};
}
