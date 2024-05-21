cmake_minimum_required(VERSION 3.14)
set(CMAKE_C_STANDARD 11)

# Variables:
# - BUILD_PLATFORM=PLAYDATE when building for device
# - BUILD_PLATFORM=PLAYDATE_SIM when building for simulator
# - BUILD_PLATFORM not set when building for native PC

if(("${BUILD_PLATFORM}" STREQUAL "PLAYDATE") OR ("${BUILD_PLATFORM}" STREQUAL "PLAYDATE_SIM"))
	set(IS_PLAYDATE_OR_SIM TRUE)
else()
	set(IS_PLAYDATE_OR_SIM FALSE)
endif()

# Figure out Playdate SDK location
if (IS_PLAYDATE_OR_SIM)
	set(ENVSDK $ENV{PLAYDATE_SDK_PATH})
	if (NOT ${ENVSDK} STREQUAL "")
		# Convert path from Windows
		file(TO_CMAKE_PATH ${ENVSDK} SDK)
	endif()
	if (NOT EXISTS ${SDK})
		message(FATAL_ERROR "SDK Path not found; set ENV value PLAYDATE_SDK_PATH")
		return()
	endif()
endif()

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo")
set(CMAKE_XCODE_GENERATE_SCHEME TRUE)

# Game name
set(PLAYDATE_GAME_NAME Nesnausk_CrankTheWorld)
set(PLAYDATE_GAME_DEVICE Nesnausk_CrankTheWorld_DEVICE)

project(${PLAYDATE_GAME_NAME} C ASM)

set (DEMO_SOURCES
	src/globals.c
	src/globals.h
	src/main.c
	src/mathlib.h
	src/platform.c
	src/platform.h
	src/effects/fx.c
	src/effects/fx.h
	src/effects/fx_plasma.c
	src/effects/fx_prettyhip.c
	src/effects/fx_raymarch.c
	src/effects/fx_raytrace.c
	src/effects/fx_starfield.c
	src/mini3d/render.c
	src/mini3d/render.h
	src/util/pixel_ops.c
	src/util/pixel_ops.h
	src/util/image_loader.c
	src/util/image_loader.h
	src/external/aheasing/easing.c
	src/external/aheasing/easing.h
)

if (TOOLCHAIN STREQUAL "armgcc")
	add_executable(${PLAYDATE_GAME_DEVICE} ${DEMO_SOURCES})
	# leave assembly files in build artifacts
	target_compile_options(${PLAYDATE_GAME_DEVICE} PRIVATE -save-temps)
	target_compile_definitions(${PLAYDATE_GAME_DEVICE} PRIVATE BUILD_PLATFORM_PLAYDATE)
elseif(IS_PLAYDATE_OR_SIM)
	add_library(${PLAYDATE_GAME_NAME} SHARED ${DEMO_SOURCES})
	target_compile_definitions(${PLAYDATE_GAME_NAME} PRIVATE BUILD_PLATFORM_PLAYDATE)
else()
	add_executable(${PLAYDATE_GAME_NAME} ${DEMO_SOURCES})
endif()

if(IS_PLAYDATE_OR_SIM)
	include(${SDK}/C_API/buildsupport/playdate_game.cmake)
else()
	target_compile_definitions(${PLAYDATE_GAME_NAME} PRIVATE BUILD_PLATFORM_PC _CRT_SECURE_NO_WARNINGS)

	add_custom_command(
		TARGET ${PLAYDATE_GAME_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy
		${CMAKE_CURRENT_SOURCE_DIR}/Source/BlueNoise.tga
		${CMAKE_CURRENT_BINARY_DIR}/BlueNoise.tga
	)

endif()
