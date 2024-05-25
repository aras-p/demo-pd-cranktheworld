cmake_minimum_required(VERSION 3.14)

cmake_policy(SET CMP0091 NEW) # MSVC runtime library control

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
if (CMAKE_SYSTEM_NAME STREQUAL Emscripten)
	set(EMSCRIPTEN TRUE)
else()
	set(EMSCRIPTEN FALSE)
endif()
if (CMAKE_SYSTEM_NAME STREQUAL Linux)
	set(LINUX TRUE)
else()
	set(LINUX FALSE)
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
	src/util/wav_ima_adpcm.c
	src/util/wav_ima_adpcm.h
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
elseif(APPLE)
	add_executable(${PLAYDATE_GAME_NAME} MACOSX_BUNDLE ${DEMO_SOURCES})
	set_target_properties(${PLAYDATE_GAME_NAME} PROPERTIES
		BUNDLE True
		MACOSX_BUNDLE_GUI_IDENTIFIER org.nesnausk.cranktheworld
		MACOSX_BUNDLE_BUNDLE_NAME "Crank The World"
		MACOSX_BUNDLE_BUNDLE_VERSION "0.1"
		MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1"
		MACOSX_BUNDLE_ICON_FILE "icon.png"
	)
else()
	add_executable(${PLAYDATE_GAME_NAME} ${DEMO_SOURCES})
endif()

if(IS_PLAYDATE_OR_SIM)
	include(${SDK}/C_API/buildsupport/playdate_game.cmake)
else()
	target_compile_definitions(${PLAYDATE_GAME_NAME} PRIVATE BUILD_PLATFORM_PC _CRT_SECURE_NO_WARNINGS)

	set_property(TARGET ${PLAYDATE_GAME_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

	# Enable debug symbols (RelWithDebInfo is not only that; it also turns on
	# incremental linking, disables some inlining, etc. etc.)
	if (CMAKE_BUILD_TYPE STREQUAL "Release" AND MSVC)
		target_compile_options(${PLAYDATE_GAME_NAME} PRIVATE
			/Zi
			/GS- # disable buffer security check
		)
		target_link_options(${PLAYDATE_GAME_NAME} PRIVATE /DEBUG /OPT:ICF /OPT:REF)
		set_property(TARGET ${PLAYDATE_GAME_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
	endif()

	if (APPLE)
		target_compile_options(${PLAYDATE_GAME_NAME} PRIVATE -x objective-c)
		target_link_libraries(${PLAYDATE_GAME_NAME} PRIVATE
			"-framework Cocoa"
			"-framework Metal"
			"-framework MetalKit"
			"-framework AudioToolbox"
			"-framework QuartzCore")
	elseif (LINUX)
		set(THREADS_PREFER_PTHREADS_FLAG ON)
		find_package(Threads REQUIRED)
		target_compile_options(${PLAYDATE_GAME_NAME} PRIVATE -Wno-format-truncation)
		target_link_libraries(${PLAYDATE_GAME_NAME} PRIVATE X11 Xi Xcursor GL asound dl m Threads::Threads)
	elseif (EMSCRIPTEN)
		set(CMAKE_EXECUTABLE_SUFFIX .html)
		target_link_options(${PLAYDATE_GAME_NAME} PRIVATE -sUSE_WEBGL2=1)
	endif()

	if(APPLE)
		set(DATA_DIR "../Resources")
	else()
		set(DATA_DIR "data")
	endif()

	add_custom_command(
		TARGET ${PLAYDATE_GAME_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory
		$<TARGET_FILE_DIR:${PLAYDATE_GAME_NAME}>/${DATA_DIR}
	)
	add_custom_command(
		TARGET ${PLAYDATE_GAME_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy
		${CMAKE_CURRENT_SOURCE_DIR}/Source/sys_img/icon.png
		${CMAKE_CURRENT_SOURCE_DIR}/Source/music.wav
		${CMAKE_CURRENT_SOURCE_DIR}/Source/BlueNoise.tga
		${CMAKE_CURRENT_SOURCE_DIR}/Source/text_crank.png
		${CMAKE_CURRENT_SOURCE_DIR}/Source/text_everybody.png
		${CMAKE_CURRENT_SOURCE_DIR}/Source/text_instr.png
		${CMAKE_CURRENT_SOURCE_DIR}/Source/text_logo.png
		${CMAKE_CURRENT_SOURCE_DIR}/Source/text_theworld.png
		${CMAKE_CURRENT_SOURCE_DIR}/Source/text_wantsto.png
        $<TARGET_FILE_DIR:${PLAYDATE_GAME_NAME}>/${DATA_DIR}
	)

endif()
