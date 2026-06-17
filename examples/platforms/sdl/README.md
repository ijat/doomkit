# sdl — DOOM in a real desktop window (SDL2)

A "real" port: [SDL2](https://www.libsdl.org/) gives us a window, a pixel
texture, a clock, and keyboard events, and we wire those into the six `DG_*`
callbacks. Read it after [`../null/`](../null) — the structure is identical, only
the backend differs.

## What you need

1. **SDL2** installed (`brew install sdl2`, `apt install libsdl2-dev`, …).
2. **The upstream DOOM engine sources** (this package doesn't vendor them) — see
   the root [README](../../../README.md#running-real-doom).
3. **A WAD** (the free shareware `doom1.wad` works).

## Build & run

This port uses the *classic* doomgeneric style: it defines the `DG_*` symbols
directly and is compiled together with the engine (no shared library).

```sh
ENGINE=<engine_dir>   # path to doomgeneric's inner folder (the one with d_main.c)

cc -I$ENGINE -Iinclude $(sdl2-config --cflags) \
   examples/platforms/sdl/platform_sdl.c \
   src/dg_keyqueue.c src/dg_keymap.c \
   $(ls $ENGINE/*.c | grep -v 'doomgeneric_') \
   $(sdl2-config --libs) -o doom

./doom -iwad /path/to/doom1.wad
```

The `grep -v 'doomgeneric_'` filter is required: a bare `*.c` glob would pull in
`doomgeneric_allegro.c`, `doomgeneric_xlib.c`, and other platform backends that
define conflicting `DG_*` symbols and require unrelated libraries. Only
`platform_sdl.c` (this file) should provide those symbols.

There is no `make` target for this one because it needs SDL2 **and** the engine
to produce a playable binary.

## What it demonstrates

| Callback | SDL backing |
|----------|-------------|
| `DG_Init` | create an SDL window + streaming RGB texture |
| `DG_DrawFrame` | upload `DG_ScreenBuffer` to the texture, present, pump events |
| `DG_GetTicksMs` / `DG_SleepMs` | `SDL_GetTicks` / `SDL_Delay` |
| `DG_GetKey` | pop from `dg_keyqueue` (filled while pumping SDL events) |

Input shows the two helpers working together: `dg_keymap_from_sdl()` turns an
`SDL_Keycode` into a DOOM key code, and `dg_keyqueue` carries it from SDL's event
pump to `DG_GetKey`.

Controls: arrows move, **Ctrl** fires, **Space** opens doors, **Esc** opens the
menu.
