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
#if defined(__APPLE__)
#define SOKOL_METAL
#elif defined(_WIN32)
#define SOKOL_D3D11
#elif defined(__EMSCRIPTEN__)
#define SOKOL_GLES3
#else
#define SOKOL_GLCORE
#endif
#include "external/sokol/sokol_app.h"
#include "external/sokol/sokol_gfx.h"
#include "external/sokol/sokol_log.h"
#include "external/sokol/sokol_time.h"
#include "external/sokol/sokol_audio.h"
#include "external/sokol/sokol_glue.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "external/stb/stb_image.h"

#include "util/wav_ima_adpcm.h"

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

// from fpsunflower/nanofont https://gist.github.com/fpsunflower/7e6311c9580409c115a0
//
// Glyphs from http://font.gohu.org/ (8x14 version, most common ascii characters only)
// Arguments are the ascii character to print, and x and y positions within the glyph.
static inline bool glyph_pixel(unsigned int c, unsigned int x, unsigned int y) {
    c -= 33; x--; if (c > 93 || x > 6 || y > 13) return 0; int i = 98 * c + 7 * y + x;
    return (("0@P01248@00120000P49B0000000000000:DXlW2UoDX@10008@h;IR4n@R<Y?48000PYDF"
    "PP011J:U1000<T8QQQDAR4a50000@P012000000000000222448@P024@010028P0148@PP011100000"
    "ABELDU410000000048@l7124000000000000000H`01100000000n10000000000000000006<0000@P"
    "P011224488@00000`CXHY:=:D8?0000004<DT01248@000000l4:444444h700000`C8@Ph02D8?0000"
    "008HX89b?8@P000000n58`7@P05b300000`CP0O25:D8?00000POPP0112248000000l4:D8?Q25b300"
    "000`CX@Ql1244700000000H`0000<H00000000`P1000H`0110000044444@014@0000000n100PO000"
    "0000004@014@@@@@0000h948@@@@00120000`G`l5;F\\Lf0n100000l4:DXOQ25:400000hCX@Qn4:D"
    "X?000000?Q248@P0Ql000000N49DX@Q25i100000hGP01N48@PO00000PO124hAP012000000l4:@PLQ"
    "25b3000008DX@Qn5:DX@000000748@P0124L00000001248@P25b3000008DT456D8AT@00000P01248"
    "@P01n10000017G=IbP1364000008dXAU:U:E\\H000000?Q25:DX@Ql000000n4:DX?1248000000`CX"
    "@Q2U:E4GP0000P?Q25jCR8Q2100000l4:@0?P05b300000l71248@P01200000P@Q25:DX@Ql0000002"
    "5:D89BT`P1000004<HbT9[:BT800000P@QT8QQ49Q210000013:B4548@P000000h7888888@PO00000"
    "7248@P01248`10P0148P0148P0148000h01248@P0124>000015A000000000000000000000000h?00"
    "04@010000000000000000l0bGX@aL10000124XcX@Q25j300000000?Q248@8?000008@Pl5:DX@aL10"
    "000000`CX@o24`70000`AP01N48@P0100000000l5:DX@aL12T70124XcX@Q25:40000@P0P348@P01>"
    "00000240HP01248@P0a101248@T47B4940000HP01248@P01L00000000oBV<IbT910000000hCX@Q25"
    ":400000000?Q25:D8?00000000j<:DX@Qn48@00000`GX@Q25c58@P0000P>S248@P000000000l48P7"
    "@Pn0000048@`31248@030000000P@Q25:D<G0000000025:T49<H000000004<HbTE5920000000P@QT"
    "`@BX@0000000025:DX@aL12T70000h744444h70000PS01248>P0124`1001248@P01248@P0007@P01"
    "24`@P01R30000000S9S10000000"[i / 6] - '0') >> (i % 6)) & 1;
}

static void draw_text(const char* msg, int bx, int by)
{
    int lx = bx, ly = by;
    while (*msg)
    {
        // draw glyph
        for (int y = 0; y < 14; ++y)
        {
            int yy = ly + y;
            if (yy < 0 || yy >= SCREEN_Y) continue;
            uint8_t* row = s_screen_buffer + yy * SCREEN_STRIDE_BYTES;
            for (int x = 0; x < 8; ++x)
            {
                if (glyph_pixel(*msg, x, y))
                {
                    int xx = lx + x;
                    if (x >= 0 && x < SCREEN_X)
                        put_pixel_black(row, xx);
                }
            }
        }
        // move to next character or line
        if (*msg == '\n') {
            lx = bx;
            ly += 14;
        } else {
            lx += 8;
        }
        ++msg;
    }
}

void plat_gfx_draw_stats(float par1)
{
    // clear rect to white
    int rectx = 48;
    int recty = 16;
    for (int y = 0; y < recty; ++y)
    {
        uint8_t* row = s_screen_buffer + y * SCREEN_STRIDE_BYTES;
        for (int x = 0; x < rectx / 8; ++x)
            row[x] = 0xFF;
    }

    // draw text
    char buf[100];
    snprintf(buf, sizeof(buf), "t %i", (int)par1);
    draw_text(buf, 1, 1);
}

static char s_data_path[1000];

PlatBitmap* plat_gfx_load_bitmap(const char* file_path, const char** outerr)
{

	*outerr = "";

	char path[1000];
	snprintf(path, sizeof(path), "%s/%s", s_data_path, file_path);
	size_t path_len = strlen(path);
	path[path_len - 3] = 'p';
	path[path_len - 2] = 'n';
	path[path_len - 1] = 'g';

	PlatBitmap* res = (PlatBitmap*)plat_malloc(sizeof(PlatBitmap));
	int comp;
	res->ga = stbi_load(path, &res->width, &res->height, &comp, 2);
	return res;
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
	char path[1000];
	snprintf(path, sizeof(path), "%s/%s", s_data_path, file_path);
	return (PlatFile*)fopen(path, "rb");
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
	vsnprintf(buf, sizeof(buf), fmt, args);
	slog_func("demo", 1, 0, buf, 0, "", NULL);
}

void plat_sys_log_error(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	plat_sys_log_error_impl(fmt, args);
	va_end(args);
}

typedef struct PlatFileMusicPlayer {
	uint8_t* file;
	int file_size;
	wav_file_desc wav;
	wav_decode_state decode_state;
	int decode_pos;
} PlatFileMusicPlayer;

static PlatFileMusicPlayer* s_current_music;

PlatFileMusicPlayer* plat_audio_play_file(const char* file_path)
{
	s_current_music = NULL;

	// read file into memory
	char path[1000];
	snprintf(path, sizeof(path), "%s/%s", s_data_path, file_path);
	size_t path_len = strlen(path);
	path[path_len - 3] = 'w';
	path[path_len - 2] = 'a';
	path[path_len - 1] = 'v';

	FILE* file = fopen(path, "rb");
	if (file == NULL)
		return NULL;

	PlatFileMusicPlayer* res = (PlatFileMusicPlayer*)plat_malloc(sizeof(PlatFileMusicPlayer));
	res->decode_pos = 0;
	fseek(file, 0, SEEK_END);
	res->file_size = (int)ftell(file);
	fseek(file, 0, SEEK_SET);
	res->file = plat_malloc(res->file_size);
	fread(res->file, 1, res->file_size, file);
	fclose(file);

	// parse wav header
	if (!wav_parse_header(res->file, res->file_size, &res->wav))
		return NULL;

	if (res->wav.sample_format != 0x11) // not IMA ADPCM
		return NULL;

	wav_decode_state_init(&res->wav, &res->decode_state);

	s_current_music = res;
	return res;
}

bool plat_audio_is_playing(PlatFileMusicPlayer* music)
{
	return music->decode_pos < music->wav.sample_count;
}

float plat_audio_get_time(PlatFileMusicPlayer* music)
{
	return music->decode_pos / 44100.0f;
}

void plat_audio_set_time(PlatFileMusicPlayer* music, float t)
{
	int sample_pos = (int)(t * 44100.0f);
	if (sample_pos < 0)
		sample_pos = 0;
	if (sample_pos > music->wav.sample_count)
		sample_pos = music->wav.sample_count;
	music->decode_pos = sample_pos;
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

static PlatButtons s_but_current, s_but_pushed, s_but_released;

void plat_input_get_buttons(PlatButtons* current, PlatButtons* pushed, PlatButtons* released)
{
	*current = s_but_current;
	*pushed = s_but_pushed;
	*released = s_but_released;
}

static float s_crank_angle = 0.0f;

float plat_input_get_crank_angle_rad()
{
	return s_crank_angle;
}

// sokol_app setup

static void audio_sample_cb(float* buffer, int num_frames, int num_channels)
{
	if (s_current_music == NULL)
	{
		memset(buffer, 0, num_frames * num_channels * sizeof(buffer[0]));
		return;
	}

	assert(1 == num_channels);

	int decode_frames = num_frames;
	if (decode_frames > s_current_music->wav.sample_count - s_current_music->decode_pos)
		decode_frames = s_current_music->wav.sample_count - s_current_music->decode_pos;
	if (decode_frames < 0)
		decode_frames = 0;

	wav_ima_adpcm_decode(buffer, s_current_music->decode_pos, decode_frames, s_current_music->wav.sample_data, &s_current_music->decode_state);

	if (decode_frames < num_frames)
	{
		memset(buffer + decode_frames * num_channels, 0, (num_frames - decode_frames) * num_channels * sizeof(buffer[0]));
	}
	s_current_music->decode_pos += decode_frames;
}

static const char* kSokolVertexSource =
#if defined(SOKOL_METAL) || defined(SOKOL_D3D11)
// HLSL / Metal
#ifdef SOKOL_METAL
"#include <metal_stdlib>\n"
"using namespace metal;\n"
"struct frame_uni { float boxx; float boxy; };\n"
"struct v2f { float2 uv; float4 pos [[position]]; };\n"
"vertex v2f vs_main(uint vidx [[vertex_id]], constant frame_uni& uni [[buffer(0)]])\n"
#else
"cbuffer uni : register(b0) { float boxx; float boxy; };\n"
"struct v2f { float2 uv : TEXCOORD0; float4 pos : SV_Position; };\n"
"v2f vs_main(uint vidx: SV_VertexID)\n"
#endif
"{\n"
"    v2f o;\n"
"    float2 uv = float2((vidx << 1) & 2, vidx & 2);\n"
"    o.uv = (uv - 0.5) * float2(uni.boxx, uni.boxy) + 0.5;\n"
"    o.pos = float4(uv * float2(2, -2) + float2(-1, 1), 0, 1);\n"
"    return o;\n"
"}\n";
#else
#ifdef SOKOL_GLCORE
"#version 410\n"
#else
"#version 300 es\n"
#endif
"uniform vec2 box;\n"
"out vec2 uv;\n"
"void main() {\n"
"  uv = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);\n"
"  gl_Position = vec4(uv * vec2(2, -2) + vec2(-1, 1), 0, 1);\n"
"  uv = (uv - vec2(0.5)) * box + vec2(0.5);\n"
"}";
#endif

static const char* kSokolFragSource =
#if defined(SOKOL_METAL) || defined(SOKOL_D3D11)
// HLSL / Metal
#ifdef SOKOL_METAL
"#include <metal_stdlib>\n"
"using namespace metal;\n"
"struct v2f { float2 uv; };\n"
"fragment float4 fs_main(v2f i [[stage_in]], texture2d<float> tex [[texture(0)]])\n"
#else
"struct v2f { float2 uv : TEXCOORD0; };\n"
"Texture2D<float4> tex : register(t0);\n"
"float4 fs_main(v2f i) : SV_Target0\n"
#endif
"{\n"
"  int x = int(i.uv.x * 400);\n"
"  int y = int(i.uv.y * 240);\n"
#ifdef SOKOL_METAL
"  float pix = tex.read(uint2(x>>3, y), 0).x;\n"
#else
"  float pix = tex.Load(int3(x>>3, y, 0)).x;\n"
#endif
"  uint val = uint(pix * 255.5);\n"
"  uint mask = 1 << (7 - (x & 7));\n"
"  float4 col = val & mask ? float4(0.694, 0.686, 0.659, 1.0) : float4(0.192, 0.184, 0.157, 1.0);\n"
"  if (any(i.uv != saturate(i.uv))) col.rgb = 0.0;\n"
"  return col;\n"
"}\n";
#else
#ifdef SOKOL_GLCORE
"#version 410\n"
#else
"#version 300 es\n"
"precision highp float;\n"
"precision highp int;\n"
#endif
"uniform sampler2D tex;\n"
"in vec2 uv;\n"
"out vec4 frag_color;\n"
"void main() {\n"
"  int x = int(uv.x * 400.0);\n"
"  int y = int(uv.y * 240.0);\n"
"  float pix = texelFetch(tex, ivec2(x>>3, y), 0).x;\n"
"  uint val = uint(pix * 255.5);\n"
"  uint mask = uint(1 << (7 - (int(x) & 7)));\n"
"  frag_color = ((val & mask) != 0u) ? vec4(0.694, 0.686, 0.659, 1.0) : vec4(0.192, 0.184, 0.157, 1.0);\n"
"  if (any(notEqual(uv, clamp(uv, 0.0, 1.0)))) frag_color.rgb = vec3(0.0);\n"
"}\n";
#endif

static sg_pass_action sok_pass;
static sg_shader sok_shader;
static sg_pipeline sok_pipe;
static sg_image sok_image;
static sg_sampler sok_sampler;

typedef struct frame_uniforms {
    float boxx;
    float boxy;
} frame_uniforms;

static void sapp_init(void)
{
	// graphics
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
        .vs.uniform_blocks[0].size = sizeof(frame_uniforms),
        .vs.uniform_blocks[0].uniforms = {
            [0] = { .name = "box", .type = SG_UNIFORMTYPE_FLOAT2 },
        },
        .vs.entry = "vs_main",
		.fs = {
			.images[0].used = true,
			.samplers[0].used = true,
			.image_sampler_pairs[0] = { .used = true, .glsl_name = "tex", .image_slot = 0, .sampler_slot = 0 },
			.source = kSokolFragSource,
            .entry = "fs_main",
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

	// audio
	saudio_setup(&(saudio_desc) {
		.sample_rate = 44100,
		.num_channels = 1,
		.stream_cb = audio_sample_cb,
		.logger.func = slog_func,
	});

	stm_setup();
	sok_start_time = stm_now();

	app_initialize();
}

static void sapp_frame(void)
{
	app_update();

	#if defined(__EMSCRIPTEN__)
	if (saudio_suspended()) {
		if (0 == (sapp_frame_count() & (1<<5))) {
			draw_text("click/tap to start", (400 - (18*8)) / 2, (240 - 7) / 2);
		}
	}
	#endif

	sg_update_image(sok_image, &(sg_image_data){.subimage[0][0] = SG_RANGE(s_screen_buffer)});

	sg_begin_pass(&(sg_pass) { .action = sok_pass, .swapchain = sglue_swapchain() });

	sg_bindings bind = {
		.fs = {
			.images[0] = sok_image,
			.samplers[0] = sok_sampler,
		},
	};
    float sx = sapp_widthf();
    float sy = sapp_heightf();
    float aspect = sx / sy;
    float normal = 400.0f / 240.0f;
    
    frame_uniforms uni = {
        .boxx = aspect > normal ? aspect / normal : 1.0f,
        .boxy = aspect > normal ? 1.0f : normal / aspect
    };

	sg_apply_pipeline(sok_pipe);
	sg_apply_bindings(&bind);
    sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &SG_RANGE(uni));
	sg_draw(0, 3, 1);
	sg_end_pass();
	sg_commit();

	s_but_pushed = s_but_released = 0;
}

static void sapp_cleanup(void)
{
	saudio_shutdown();
	sg_shutdown();
}

static void sapp_onevent(const sapp_event* evt)
{
	if (evt->type == SAPP_EVENTTYPE_KEY_DOWN)
	{
		if (evt->key_code == SAPP_KEYCODE_LEFT) {
			s_but_pushed |= kPlatButtonLeft;
			s_but_current |= kPlatButtonLeft;
		}
		if (evt->key_code == SAPP_KEYCODE_RIGHT) {
			s_but_pushed |= kPlatButtonRight;
			s_but_current |= kPlatButtonRight;
		}
		if (evt->key_code == SAPP_KEYCODE_UP) {
			s_but_pushed |= kPlatButtonUp;
			s_but_current |= kPlatButtonUp;
		}
		if (evt->key_code == SAPP_KEYCODE_DOWN) {
			s_but_pushed |= kPlatButtonDown;
			s_but_current |= kPlatButtonDown;
		}
		if (evt->key_code == SAPP_KEYCODE_A) {
			s_but_pushed |= kPlatButtonA;
			s_but_current |= kPlatButtonA;
		}
		if (evt->key_code == SAPP_KEYCODE_B) {
			s_but_pushed |= kPlatButtonB;
			s_but_current |= kPlatButtonB;
		}
        if (evt->key_code == SAPP_KEYCODE_ESCAPE) {
            sapp_quit();
        }
	}
	if (evt->type == SAPP_EVENTTYPE_KEY_UP)
	{
		if (evt->key_code == SAPP_KEYCODE_LEFT) {
			s_but_released |= kPlatButtonLeft;
			s_but_current &= ~kPlatButtonLeft;
		}
		if (evt->key_code == SAPP_KEYCODE_RIGHT) {
			s_but_released |= kPlatButtonRight;
			s_but_current &= ~kPlatButtonRight;
		}
		if (evt->key_code == SAPP_KEYCODE_UP) {
			s_but_released |= kPlatButtonUp;
			s_but_current &= ~kPlatButtonUp;
		}
		if (evt->key_code == SAPP_KEYCODE_DOWN) {
			s_but_released |= kPlatButtonDown;
			s_but_current &= ~kPlatButtonDown;
		}
		if (evt->key_code == SAPP_KEYCODE_A) {
			s_but_released |= kPlatButtonA;
			s_but_current &= ~kPlatButtonA;
		}
		if (evt->key_code == SAPP_KEYCODE_B) {
			s_but_released |= kPlatButtonB;
			s_but_current &= ~kPlatButtonB;
		}
	}
	if (evt->type == SAPP_EVENTTYPE_MOUSE_SCROLL)
	{
		s_crank_angle += evt->scroll_y * 0.03f;
		s_crank_angle = fmodf(s_crank_angle, 3.14159265f * 2.0f);
	}
}

sapp_desc sokol_main(int argc, char* argv[]) {
	(void)argc; (void)argv;

	// figure out where is the data folder
#ifdef __APPLE__
	snprintf(s_data_path, sizeof(s_data_path), "%s", [[NSBundle mainBundle].resourcePath UTF8String]);
#else
    strncpy(s_data_path, "data", sizeof(s_data_path));
#endif

	sapp_desc res = (sapp_desc) {
		.init_cb = sapp_init,
		.frame_cb = sapp_frame,
		.cleanup_cb = sapp_cleanup,
		.event_cb = sapp_onevent,
		.width = SCREEN_X * 2,
		.height = SCREEN_Y * 2,
		.window_title = "Everybody Wants to Crank the World",
		.icon.sokol_default = true,
		.logger.func = slog_func,
	};

	// try to load icon
	int icon_width, icon_height, icon_comp;
    char icon_path[1000];
    snprintf(icon_path, sizeof(icon_path), "%s/icon.png", s_data_path);
	uint8_t* icon_image = stbi_load(icon_path, &icon_width, &icon_height, &icon_comp, 4);
	if (icon_image != NULL) {
		res.icon.sokol_default = false;
		res.icon.images[0] = (sapp_image_desc){
			.width = icon_width,
			.height = icon_height,
			.pixels.ptr = icon_image,
			.pixels.size = icon_width * icon_height * 4
		};
	}

	return res;
}


// --------------------------------------------------------------------------
#else
#error Unknown platform! Needs to be playdate or pc.
#endif
