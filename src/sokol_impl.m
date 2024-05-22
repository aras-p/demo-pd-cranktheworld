// SPDX-License-Identifier: Unlicense

#if defined(BUILD_PLATFORM_PLAYDATE)
#error Should not compile this file when building for Playdate
#endif

// Sokol implementations need to be in a separate ObjC source file on Apple
// platforms

#define SOKOL_IMPL
#define SOKOL_METAL
#include "external/sokol/sokol_app.h"
#include "external/sokol/sokol_gfx.h"
#include "external/sokol/sokol_log.h"
#include "external/sokol/sokol_time.h"
#include "external/sokol/sokol_audio.h"
#include "external/sokol/sokol_glue.h"
