# Porting DOOM to a new platform — step by step

This is the beginner walkthrough. By the end you will have DOOM running on a new
backend by writing **one file with six functions**. No prior DOOM-internals
knowledge required.

> Prerequisite mental model: read the top of
> [`include/genericdoom/genericdoom.h`](../include/genericdoom/genericdoom.h)
> (the power-socket analogy) and skim [`CONTRACT.md`](CONTRACT.md). That's all
> the theory you need.

---

## Step 0 — get the pieces

You need three things in one build:

1. **This package** — the contract header (`include/`) and helpers (`src/`).
2. **The DOOM engine sources** — the upstream `doomgeneric` `.c`/`.h` files that
   provide `doomgeneric_Create()` / `doomgeneric_Tick()` and the whole game.
   This package does not ship them (they are unchanged GPL code). Get them from
   <https://github.com/ozkl/doomgeneric> and put their `doomgeneric/` folder
   where your build can see it.
3. **A WAD file** — the game data. The free shareware `doom1.wad` works.

---

## Step 1 — copy the template

```sh
cp examples/template/platform_template.c platform_myplatform.c
```

It already contains `main()` and empty versions of the six callbacks, wired to
the `dg_keyqueue` helper.

---

## Step 2 — fill in the six functions

Implement them against your platform's API. Use `examples/sdl/platform_sdl.c` as
a worked reference and `examples/null/platform_null.c` to see them run with no
backend at all.

1. **`DG_Init`** — open a window/surface of `DOOMGENERIC_RESX × DOOMGENERIC_RESY`
   and initialise input. Call `dg_keyqueue_init(&my_queue)`.
2. **`DG_DrawFrame`** — copy `DG_ScreenBuffer` (32-bit `0x00RRGGBB` pixels) to
   your display, then pump your OS event queue and, for each key, do:
   ```c
   dg_keyqueue_push(&my_queue, is_down ? 1 : 0, dg_keymap_from_sdl(host_key));
   ```
   If your platform's key codes aren't SDL-like, copy `dg_keymap_from_sdl` and
   swap the `case` labels — the shape stays the same.
3. **`DG_SleepMs`** — sleep the given milliseconds.
4. **`DG_GetTicksMs`** — return a monotonic millisecond counter.
5. **`DG_GetKey`** — `return dg_keyqueue_pop(&my_queue, pressed, doomKey);`
6. **`DG_SetWindowTitle`** — set the title, or leave empty.

---

## Step 3 — build

Compile your platform file + this package's `src/` + the engine sources:

```sh
cc -Iinclude -I<engine_dir> \
   platform_myplatform.c \
   src/dg_keyqueue.c src/dg_keymap.c \
   <engine_dir>/*.c \
   <your platform libs> \
   -o doom
```

(For pixel conversion the engine already contains its own `i_video.c`; the
`dg_palette` / `dg_framebuffer` helpers in this package are the clean, tested
reference for that same math — useful if you write a from-scratch video path.)

---

## Step 4 — run

```sh
./doom -iwad doom1.wad
```

You should get a window with the title screen. Arrow keys move, `Ctrl` fires,
`Space` opens doors, `Esc` opens the menu.

---

## Troubleshooting

| Symptom | Likely cause |
|---------|--------------|
| Black window, no image | `DG_DrawFrame` not copying `DG_ScreenBuffer`, or wrong pitch — use `DOOMGENERIC_RESX * sizeof(pixel_t)` bytes per row. |
| Colours look swapped (red/blue) | Your display expects a different channel order; adjust the shifts (see `dg_palette`'s `red_shift`/`blue_shift`) or your texture format. |
| Game runs too fast / too slow | `DG_GetTicksMs` not in milliseconds, or not monotonic. |
| Keys do nothing | You queued host codes instead of DOOM codes — run them through `dg_keymap` first. |
| Image stuck in a corner | You ignored the centring offset; see `dg_framebuffer` / `examples/null`. |

---

## What "done" looks like

A single `platform_myplatform.c`, no engine edits, DOOM playable. That's the
whole point of genericdoom: the hard part (a full 3D game) is already solved; you
only teach it about *your* screen, clock and keyboard.
