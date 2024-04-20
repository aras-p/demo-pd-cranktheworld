# "Everybody Wants to Crank the World" demo by nesnausk! (2024)

A real-time rendering demo for [Playdate](https://play.date/).

* **Video capture**: TODO youtube / file
* **Playdate build**: TODO link
* **pouët**: TODO link

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

Also wanted to check out how much of shadertoy-like raymarching could a Playdate do. Turns out, not "a lot", lol.
That is why the raymarched / raytraced scenes run at like "temporal" update pattern that updates only every 4th or
6th or 8th etc. pixel every frame. Majority of the demo runs at 30FPS, with some parts dropping to about 24FPS.

Some of the scenes/effects are ~~ripped off~~ *inspired* by other shadertoys or demos:
- [twisty cuby](https://www.shadertoy.com/view/MtdyWj) by DJDoomz
- [Pretty Hip](https://www.shadertoy.com/view/XsBfRW) by Fabrice Neyret
- [Xor Towers](https://www.shadertoy.com/view/7lsXR2) by Greg Rostami
- [Menger Sponge Variation](https://www.shadertoy.com/view/ldyGWm) by Shane
- [Puls](https://www.pouet.net/prod.php?which=53816) by Řrřola

When the music finishes, the demo switches to "interactive mode" where you can switch between the effects/scenes with
Playdate A/B buttons. You can also use the crank to orbit/rotate the camera or change some other scene parameter.
Actually, you can use the crank to control the camera during the regular demo playback as well.
