# Run doomkit from C

The native case: link your program against `libdoomgeneric` and register six
callbacks through the C API. (You *could* instead define the `DG_*` symbols
directly and static-link the engine — that's the classic doomgeneric way, shown
in `examples/sdl/` — but the registration API is what every other language uses,
so we use it here too for consistency.)

## What you need

1. **The shared library** `libdoomgeneric.{so,dylib,dll}` — build it once by
   following [`bindings/README.md`](../../../bindings/README.md).
2. **A WAD** — the free shareware `doom1.wad` is fine.
3. A C compiler.

## Build

Assuming the library and its import header are reachable:

```sh
# macOS
cc -I../../../bindings main.c -L/path/to/lib -ldoomgeneric -o doomdemo

# Linux (you may also need -Wl,-rpath so the loader finds the .so at runtime)
cc -I../../../bindings main.c -L/path/to/lib -ldoomgeneric -Wl,-rpath,/path/to/lib -o doomdemo
```

## Run

```sh
./doomdemo -iwad /path/to/doom1.wad
```

The program boots the engine, runs 200 frames, and writes `frame.ppm` at frame
100 so you can confirm real pixels came out. Open `frame.ppm` in any image
viewer. A real app would loop `dg_tick()` forever and blit `dg_screen_buffer()`
to a window instead.

## How it maps to the API

| You implement | When the engine calls it |
|---------------|--------------------------|
| `cb_init` | once, during `dg_create` |
| `cb_draw_frame` | once per `dg_tick` (here it saves a PPM) |
| `cb_sleep_ms` / `cb_get_ticks_ms` | for pacing |
| `cb_get_key` | to read input (returns 0 = none here) |
| `cb_set_window_title` | once |

`dg_set_callbacks(&cb)` must be called **before** `dg_create`. See
[`bindings/doomgeneric_capi.h`](../../../bindings/doomgeneric_capi.h) for the
full contract.
