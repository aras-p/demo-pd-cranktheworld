﻿# "Everybody Wants to Crank the World" demo by nesnausk! (2024)

A real-time rendering demo for [Playdate](https://play.date/).

* **Video**: device playing it ([youtube](https://www.youtube.com/watch?v=QjAKiwQxrQI) / [mp4 file](https://aras-p.info/files/demos/2024/Nesnausk_CrankTheWorld_20240421.mp4)), just the screen ([youtube](https://www.youtube.com/watch?v=3NjHOVtTPjY) / [mp4 file](https://aras-p.info/files/demos/2024/Nesnausk_CrankTheWorld_screen20240421.mp4))
* **Playdate build**: [Nesnausk_CrankTheWorld.pdx.zip](https://aras-p.info/files/demos/2024/Nesnausk_CrankTheWorld-20240421.zip) (3MB), should also work in Playdate Simulator on Windows.
* **pouët**: https://www.pouet.net/prod.php?which=96955
* Took 4th place at [Outline 2024](https://outlinedemoparty.nl/) "Newskool Demo" category

![Screenshot](/log/playdate-20240419-140918.png?raw=true "Screenshot")
![Screenshot](/log/playdate-20240419-140936.png?raw=true "Screenshot")
![Screenshot](/log/playdate-20240419-141104.png?raw=true "Screenshot")
![Screenshot](/log/playdate-20240419-141133.png?raw=true "Screenshot")

### Credits

- Everything except music: NeARAZ
- Music: stalas001

### Details

I wanted to code some oldskool demo effects that I never did back when 30 years ago everyone else was doing them.
You know: plasma, kefren bars, blobs, starfield. And so I did!

Also wanted to check out how much of shadertoy-like raymarching could a Playdate do. Turns out, not "a lot", lol
(Playdate hardware is a single-core ARM Cortex-M7F CPU at 180MHz, and no special graphics hardware at all).
That is why the raymarched / raytraced scenes run at like "temporal" update pattern that updates only every 4th or
6th or 8th etc. pixel every frame. Majority of the demo runs at 30FPS, with some parts dropping to about 24FPS.

The 1-bit/pixel screen is interesting too, to tackle that I went the easy way and did a screen-space blue noise
based dithering.

The whole demo is pretty small, except the music track takes a whopping 3MB, because it is just an ADPCM WAV file.
The CPU cost of doing something like MP3 playback was too high, and I did not go the MIDI/MOD/XM route since the music
track is just a GarageBand experiment that my kid did several years ago.

Some of the scenes/effects are ~~ripped off~~ *inspired* by other shadertoys or demos:
- [twisty cuby](https://www.shadertoy.com/view/MtdyWj) by DJDoomz
- [Ring Twister](https://www.shadertoy.com/view/Xt23z3) by Flyguy
- [Pretty Hip](https://www.shadertoy.com/view/XsBfRW) by Fabrice Neyret
- [Xor Towers](https://www.shadertoy.com/view/7lsXR2) by Greg Rostami
- [Menger Sponge Variation](https://www.shadertoy.com/view/ldyGWm) by Shane
- [Puls](https://www.pouet.net/prod.php?which=53816) by Řrřola

When the music finishes, the demo switches to "interactive mode" where you can switch between the effects/scenes with
Playdate A/B buttons. You can also use the crank to orbit/rotate the camera or change some other scene parameter.
Actually, you can use the crank to control the camera during the regular demo playback as well.

Blog post with some more details about development: https://aras-p.info/blog/2024/05/20/Crank-the-World-Playdate-demo/

### Building

The demo can be built for several "platforms":
- Playdate device itself,
- Playdate SDK simulator,
- Regular PC (Windows/Mac/Linux) binary. This one does not require having Playdate or the SDK for it.
- Web via Emscripten (see separate build instructions below)

On Windows / Visual Studio, easiest is opening the folder directly from within VS. Then pick one of the configuration configs and build.
The actual Playdate (and simulator) builds I only tried on Windows. They may or might not work when done on other host platforms.

For PC build, the usual cmake way should work, i.e.
```
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release
```

On Linux, you might need to have these installed: `libglu1-mesa-dev`, `mesa-common-dev`, `xorg-dev`, `libasound-dev`.

### Building for Emscripten

Building for Emscripten is best done on macOS or Linux. For Windows, cmake might need to be instructed to use the
"UNIX Makefiles" or "Ninja" generator.

First setup the Emscripten SDK into the root of the project directory:

```
git clone https://github.com/emscripten-core/emsdk
cd emsdk
./emsdk install 3.1.60
./emsdk activate 3.1.60
cd ..
```

...back in the project root directory:

```
mkdir build
cd build
cmake --preset emscripten-release ..
cmake --build .
```

...this will build 3 files (Nesnausk_CrankTheWorld.wasm/.html/.js).

Finally run the build result via

```
../emsdk/upstream/emscripten/emrun Nesnausk_CrankTheWorld.html
```

### License

Everything I wrote myself is Unlicense / Public Domain. However some 3rd party libraries are used too:
- (only on PC): [**Sokol**](https://github.com/floooh/sokol): sokol_app, sokol_audio, sokol_gfx, sokol_time. zlib/libpng license.
- (only on PC): [**stb_image.h**](https://github.com/nothings/stb/blob/master/stb_image.h): for loading PNG files. Public domain.
- [**AHEasing**](https://github.com/warrenm/AHEasing). Public domain.
