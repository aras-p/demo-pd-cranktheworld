cmake_minimum_required(VERSION 3.14)
set(CMAKE_C_STANDARD 11)

set(ENVSDK $ENV{PLAYDATE_SDK_PATH})

if (NOT ${ENVSDK} STREQUAL "")
	# Convert path from Windows
	file(TO_CMAKE_PATH ${ENVSDK} SDK)
else()
	execute_process(
			COMMAND bash -c "egrep '^\\s*SDKRoot' $HOME/.Playdate/config"
			COMMAND head -n 1
			COMMAND cut -c9-
			OUTPUT_VARIABLE SDK
			OUTPUT_STRIP_TRAILING_WHITESPACE
	)
endif()

if (NOT EXISTS ${SDK})
	message(FATAL_ERROR "SDK Path not found; set ENV value PLAYDATE_SDK_PATH")
	return()
endif()

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo")
set(CMAKE_XCODE_GENERATE_SCHEME TRUE)

# Game Name Customization
set(PLAYDATE_GAME_NAME playdate_demo)
set(PLAYDATE_GAME_DEVICE playdate_demo_DEVICE)

project(${PLAYDATE_GAME_NAME} C ASM)

set (DEMO_SOURCES
	src/main.c
	src/mathlib.c
	src/mathlib.h
	src/effects/fx_planes.c
	src/effects/fx_planes.h
	src/mini3d/mini3d.c
	src/mini3d/mini3d.h
	src/mini3d/render.c
	src/mini3d/render.h
	src/mini3d/scene.c
	src/mini3d/scene.h
	src/mini3d/shape.c
	src/mini3d/shape.h
	src/external/bitshifter_radixsort/radixsort.c
	src/external/bitshifter_radixsort/radixsort.h
)

if (TOOLCHAIN STREQUAL "armgcc")
	add_executable(${PLAYDATE_GAME_DEVICE} ${DEMO_SOURCES})
else()
	add_library(${PLAYDATE_GAME_NAME} SHARED ${DEMO_SOURCES})
endif()

include(${SDK}/C_API/buildsupport/playdate_game.cmake)

