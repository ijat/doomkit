# null — the headless port (start here)

**The best first example to read, and the only one that runs with nothing
installed.** It is a doomkit platform port (it implements the six `DG_*`
callbacks) that draws to *no screen at all* — instead it writes one frame to an
image file so you can see that real pixels came out.

## Run it

From the project root:

```sh
make run-null
```

That builds and runs `platform_null.c` and writes `build/frame.ppm` — a
[PPM image](../../../docs/GLOSSARY.md#ppm). Open it in any image viewer to see a
centred colour-XOR test pattern; if your viewer won't open `.ppm`, convert it
with `magick build/frame.ppm out.png` (ImageMagick) or open it in GIMP. **No
compiler flags, no libraries, no WAD, no engine** required.

## Why it works with no engine

Real DOOM is ~73,000 lines we deliberately don't vendor. So this file contains a
tiny **fake engine** that stands in for it: it makes a paletted test image,
converts it into the framebuffer using the same helper modules the real engine
path uses (`dg_palette`, `dg_framebuffer`), feeds fake key presses through
`dg_keyqueue`, and calls your `DG_DrawFrame`. Running it proves the whole
contract + every helper compile and work together.

So this one file does two jobs at once:

1. it's a **port** — the six `DG_*` functions, written the way any platform
   would write them; and
2. it's a **demo/test** — the fake engine drives them end to end.

## What to read

Open [`platform_null.c`](platform_null.c) top to bottom — it is the most heavily
commented file in the project. Once it makes sense, the SDL port and the
template will look familiar, because every port has the same shape:

```
DG_Init · DG_DrawFrame · DG_SleepMs · DG_GetTicksMs · DG_GetKey · DG_SetWindowTitle
main(): doomgeneric_Create(); for(;;) doomgeneric_Tick();
```

To turn a port like this into a *real* one, you'd delete the fake engine and link
the actual DOOM engine instead — that's what [`../sdl/`](../sdl) shows and
[`../template/`](../template) scaffolds.

## Next notch: the real engine, still headless

[`platform_null_engine.c`](platform_null_engine.c) is the same six `DG_*`
callbacks written for *no screen*, but with the **fake engine removed and the
real ~73k-line DOOM engine linked in**. It starts the engine, ticks it for ~10
seconds of wall-clock time with no input (so you get the title screen and the
attract-mode demo), and writes `build/frame_engine.ppm` — real DOOM, rendered
without ever opening a window.

Unlike `make run-null`, this one needs the two things any real build needs: the
upstream **engine sources** and a **WAD** (see [PORTING.md](../../../docs/PORTING.md)
and [WAD.md](../../../docs/WAD.md)):

```sh
make run-null-engine ENGINE=/path/to/doomgeneric/doomgeneric WAD=/path/to/doom1.wad
```

It builds the verified portable engine set + `platform_null_engine.c` into
`build/null_engine`, runs it for 10 s, and writes `build/frame_engine.ppm`. Open
it the same way as `frame.ppm` above — you should see a frame of actual DOOM
gameplay, HUD and all. This is the bridge between the fake-engine demo and a
genuine port like [`../sdl/`](../sdl): same callbacks, real engine, just no
display.
