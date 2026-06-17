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
