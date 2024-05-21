// SPDX-License-Identifier: Unlicense

#include "platform.h"
#include <stdarg.h>

void app_initialize();
void app_update();

// --------------------------------------------------------------------------
#if defined(BUILD_PLATFORM_PLAYDATE)

#include "pd_api.h"

static const char* kFontPath = "/System/Fonts/Roobert-10-Bold.pft";
static LCDFont* s_font = NULL;

static PlaydateAPI* s_pd;

void* plat_malloc(size_t size)
{
	return s_pd->system->realloc(NULL, size);
}
void* plat_realloc(void* ptr, size_t size)
{
	return s_pd->system->realloc(ptr, size);
}
void plat_free(void* ptr)
{
	if (ptr != NULL)
		s_pd->system->realloc(ptr, 0);
}

void plat_gfx_clear(SolidColor color)
{
	s_pd->graphics->clear((LCDColor)color);
}
uint8_t* plat_gfx_get_frame()
{
	return s_pd->graphics->getFrame();
}
void plat_gfx_mark_updated_rows(int start, int end)
{
	s_pd->graphics->markUpdatedRows(start, end);
}
void plat_gfx_draw_stats(float par1)
{
	s_pd->graphics->fillRect(0, 0, 40, 32, kColorWhite);
	char* buf;
	int bufLen = s_pd->system->formatString(&buf, "t %i", (int)par1);
	s_pd->graphics->setFont(s_font);
	s_pd->graphics->drawText(buf, bufLen, kASCIIEncoding, 0, 16);
	plat_free(buf);
	s_pd->system->drawFPS(0, 0);
}

PlatBitmap* plat_gfx_load_bitmap(const char* file_path, const char** outerr)
{
	return (PlatBitmap*)s_pd->graphics->loadBitmap(file_path, outerr);
}

void plat_gfx_draw_bitmap(PlatBitmap* bitmap, int x, int y)
{
	s_pd->graphics->drawBitmap((LCDBitmap*)bitmap, x, y, kBitmapUnflipped);
}

PlatFile* plat_file_open_read(const char* file_path)
{
	return (PlatFile*)s_pd->file->open(file_path, kFileRead);
}
int plat_file_read(PlatFile* file, void* buf, uint32_t len)
{
	return s_pd->file->read((SDFile*)file, buf, len);
}
int plat_file_seek_cur(PlatFile* file, int pos)
{
	return s_pd->file->seek((SDFile*)file, pos, SEEK_CUR);
}
void plat_file_close(PlatFile* file)
{
	s_pd->file->close((SDFile*)file);
}

static void plat_sys_log_error_impl(const char* fmt, va_list args)
{
	s_pd->system->error(fmt, args);
}

void plat_sys_log_error(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	plat_sys_log_error_impl(fmt, args);
	va_end(args);
}

PlatFileMusicPlayer* plat_audio_play_file(const char* file_path)
{
	FilePlayer* music = s_pd->sound->fileplayer->newPlayer();
	bool s_music_ok = s_pd->sound->fileplayer->loadIntoPlayer(music, file_path) != 0;
	if (s_music_ok)
	{
		s_pd->sound->fileplayer->play(music, 1);
		return (PlatFileMusicPlayer*)music;
	}
	return NULL;
}

bool plat_audio_is_playing(PlatFileMusicPlayer* music)
{
	return s_pd->sound->fileplayer->isPlaying((FilePlayer*)music) != 0;
}

float plat_audio_get_time(PlatFileMusicPlayer* music)
{
	return s_pd->sound->fileplayer->getOffset((FilePlayer*)music);
}

void plat_audio_set_time(PlatFileMusicPlayer* music, float t)
{
	s_pd->sound->fileplayer->setOffset((FilePlayer*)music, t);
}

float plat_time_get()
{
	return s_pd->system->getElapsedTime();
}

void plat_time_reset()
{
	s_pd->system->resetElapsedTime();
}

void plat_input_get_buttons(PlatButtons* current, PlatButtons* pushed, PlatButtons* released)
{
	PDButtons cur, push, rel;
	s_pd->system->getButtonState(&cur, &push, &rel);
	*current = (PlatButtons)cur;
	*pushed = (PlatButtons)push;
	*released = (PlatButtons)rel;
}

float plat_input_get_crank_angle_rad()
{
	return s_pd->system->getCrankAngle() * (3.14159265358979323846f / 180.0f);
}

static int eventUpdate(void* userdata)
{
	app_update();
	return 1;
}


// entry point
#ifdef _WINDLL
__declspec(dllexport)
#endif
int eventHandler(PlaydateAPI* pd, PDSystemEvent event, uint32_t arg)
{
	if (event == kEventInit)
	{
		s_pd = pd;

		const char* err;
		s_font = pd->graphics->loadFont(kFontPath, &err);
		if (s_font == NULL)
			pd->system->error("Could not load font %s: %s", kFontPath, err);

		app_initialize();
		pd->system->resetElapsedTime();
		pd->system->setUpdateCallback(eventUpdate, pd);
	}
	return 0;
}

// --------------------------------------------------------------------------
#elif defined(BUILD_PLATFORM_PC)

#define SOKOL_IMPL
#define SOKOL_D3D11
#include "external/sokol/sokol_app.h"
#include "external/sokol/sokol_gfx.h"
#include "external/sokol/sokol_log.h"
#include "external/sokol/sokol_time.h"
#include "external/sokol/sokol_audio.h"
#include "external/sokol/sokol_glue.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "external/stb/stb_image.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct PlatBitmap {
	int width, height;
	uint8_t* ga;
} PlatBitmap;

static uint8_t s_screen_buffer[SCREEN_Y * SCREEN_STRIDE_BYTES];

void* plat_malloc(size_t size)
{
	return malloc(size);
}
void* plat_realloc(void* ptr, size_t size)
{
	return realloc(ptr, size);
}
void plat_free(void* ptr)
{
	if (ptr != NULL)
		free(ptr);
}

void plat_gfx_clear(SolidColor color)
{
	memset(s_screen_buffer, color == kSolidColorBlack ? 0x0 : 0xFF, sizeof(s_screen_buffer));
}
uint8_t* plat_gfx_get_frame()
{
	return s_screen_buffer;
}
void plat_gfx_mark_updated_rows(int start, int end)
{
}
void plat_gfx_draw_stats(float par1)
{
	//@TODO
}

PlatBitmap* plat_gfx_load_bitmap(const char* file_path, const char** outerr)
{
	*outerr = "";

	char path[1000];
	size_t path_len = strlen(file_path);
	memcpy(path, file_path, path_len + 1);
	path[path_len - 3] = 'p';
	path[path_len - 2] = 'n';
	path[path_len - 1] = 'g';

	PlatBitmap* res = (PlatBitmap*)plat_malloc(sizeof(PlatBitmap));
	int comp;
	res->ga = stbi_load(path, &res->width, &res->height, &comp, 2);
	return res;
}

static inline void put_pixel_black(uint8_t* row, int x)
{
	uint8_t mask = ~(1 << (7 - (x & 7)));
	row[x >> 3] &= mask;
}

static inline void put_pixel_white(uint8_t* row, int x)
{
	uint8_t mask = (1 << (7 - (x & 7)));
	row[x >> 3] |= mask;
}

void plat_gfx_draw_bitmap(PlatBitmap* bitmap, int x, int y)
{
	if (bitmap == NULL)
		return;
	// really unoptimized, lol
	for (int py = 0; py < bitmap->height; ++py)
	{
		int yy = y + py;
		if (yy < 0 || yy >= SCREEN_Y)
			continue;
		const uint8_t* src = bitmap->ga + py * bitmap->width * 2;
		uint8_t* dst = &s_screen_buffer[yy * SCREEN_STRIDE_BYTES];
		for (int px = 0; px < bitmap->width; ++px, src += 2)
		{
			int xx = x + px;
			if (xx < 0 || xx >= SCREEN_X)
				continue;
			if (src[1] >= 128)
			{
				if (src[0] >= 128)
					put_pixel_white(dst, xx);
				else
					put_pixel_black(dst, xx);
			}
		}
	}
}

PlatFile* plat_file_open_read(const char* file_path)
{
	return (PlatFile*)fopen(file_path, "rb");
}
int plat_file_read(PlatFile* file, void* buf, uint32_t len)
{
	return (int)fread(buf, 1, len, (FILE*)file);
}
int plat_file_seek_cur(PlatFile* file, int pos)
{
	return fseek((FILE*)file, pos, SEEK_CUR);
}
void plat_file_close(PlatFile* file)
{
	fclose((FILE*)file);
}

static void plat_sys_log_error_impl(const char* fmt, va_list args)
{
	char buf[1000];
	vsnprintf_s(buf, sizeof(buf), sizeof(buf)-1, fmt, args);
	slog_func("demo", 1, 0, buf, 0, "", NULL);
}

void plat_sys_log_error(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	plat_sys_log_error_impl(fmt, args);
	va_end(args);
}

PlatFileMusicPlayer* plat_audio_play_file(const char* file_path)
{
	//@TODO
	return NULL;
}

bool plat_audio_is_playing(PlatFileMusicPlayer* music)
{
	//@TODO
	return true;
}

float plat_audio_get_time(PlatFileMusicPlayer* music)
{
	//@TODO
	return 0.5f;
}

void plat_audio_set_time(PlatFileMusicPlayer* music, float t)
{
	//@TODO
}

static uint64_t sok_start_time;

float plat_time_get()
{
	return (float)stm_sec(stm_since(sok_start_time));
}

void plat_time_reset()
{
	sok_start_time = stm_now();
}

void plat_input_get_buttons(PlatButtons* current, PlatButtons* pushed, PlatButtons* released)
{
	//@TODO
	*current = 0;
	*pushed = 0;
	*released = 0;
}

float plat_input_get_crank_angle_rad()
{
	//@TODO
	return 0.0f;
}

// sokol_app setup

static const char* kSokolVertexSource =
"struct v2f {\n"
"  float2 uv : TEXCOORD0;\n"
"  float4 pos : SV_Position;\n"
"};\n"
"v2f main(uint vidx: SV_VertexID) {\n"
"  v2f o;\n"
"  float2 uv = float2((vidx << 1) & 2, vidx & 2);\n"
"  o.pos = float4(uv * float2(2, -2) + float2(-1, 1), 0, 1);\n"
"  o.uv = uv;\n"
"  return o;\n"
"}\n";
static const char* kSokolFragSource =
"Texture2D<float4> tex : register(t0);\n"
"float4 main(float2 uv : TEXCOORD0) : SV_Target0 {\n"
"  int x = int(uv.x * 400);\n"
"  int y = int(uv.y * 240);\n"
"  uint val = uint(tex.Load(int3(x>>3, y, 0)).r * 255.5);\n"
"  uint mask = 1 << (7 - (x & 7));\n"
"  float4 col = val & mask ? 0.9 : 0.1;\n"
"  return col;\n"
"}\n";

static sg_pass_action sok_pass;
static sg_shader sok_shader;
static sg_pipeline sok_pipe;
static sg_image sok_image;
static sg_sampler sok_sampler;

static void sapp_init(void)
{
	sg_setup(&(sg_desc) {
		.environment = sglue_environment(),
		.logger.func = slog_func,
	});
	sok_pass = (sg_pass_action){
		.colors[0] = {.load_action = SG_LOADACTION_CLEAR, .clear_value = {0.6f, 0.4f, 0.3f, 1.0f} }
	};

	sok_sampler = sg_make_sampler(&(sg_sampler_desc) {
		.min_filter = SG_FILTER_LINEAR,
		.mag_filter = SG_FILTER_LINEAR,
	});

	sok_image = sg_make_image(&(sg_image_desc) {
		.width = SCREEN_STRIDE_BYTES,
		.height = SCREEN_Y,
		.pixel_format = SG_PIXELFORMAT_R8, //@TODO
		.usage = SG_USAGE_STREAM,
	});
	sok_shader = sg_make_shader(&(sg_shader_desc) {
		.vs.source = kSokolVertexSource,
		.fs = {
			.images[0].used = true,
			.samplers[0].used = true,
			.image_sampler_pairs[0] = { .used = true, .image_slot = 0, .sampler_slot = 0 },
			.source = kSokolFragSource,
		},
	});
	sok_pipe = sg_make_pipeline(&(sg_pipeline_desc) {
		.shader = sok_shader,
		.depth = {
			.compare = SG_COMPAREFUNC_ALWAYS,
			.write_enabled = false
		},
		.index_type = SG_INDEXTYPE_NONE,
		.cull_mode = SG_CULLMODE_NONE,
	});


	stm_setup();
	sok_start_time = stm_now();

	app_initialize();
}

static void sapp_frame(void)
{
	app_update();

	sg_update_image(sok_image, &(sg_image_data){.subimage[0][0] = SG_RANGE(s_screen_buffer)});

	sg_begin_pass(&(sg_pass) { .action = sok_pass, .swapchain = sglue_swapchain() });

	sg_bindings bind = {
		.fs = {
			.images[0] = sok_image,
			.samplers[0] = sok_sampler,
		},
	};

	sg_apply_pipeline(sok_pipe);
	sg_apply_bindings(&bind);
	sg_draw(0, 3, 1);
	sg_end_pass();
	sg_commit();
}

static void sapp_cleanup(void)
{
	sg_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[]) {
	(void)argc; (void)argv;
	return (sapp_desc) {
		.init_cb = sapp_init,
		.frame_cb = sapp_frame,
		.cleanup_cb = sapp_cleanup,
		.width = SCREEN_X * 2,
		.height = SCREEN_Y * 2,
		.window_title = "Everybody Wants to Crank the World",
		.icon.sokol_default = true,
		.logger.func = slog_func,
	};
}


// --------------------------------------------------------------------------
#else
#error Unknown platform! Needs to be playdate or pc.
#endif
