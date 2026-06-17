# Architecture — how DOOM and the porting layer fit together

This document is the **map of the engine**. It does not rewrite or test the
engine (that is intentionally out of scope — see the root README); it explains
how the ~73,000 lines are organised so you know where things live and, crucially,
where the small porting seam is.

---

## The 10,000-foot view

```
        YOUR PLATFORM                 THE PORTING SEAM              THE DOOM ENGINE
   ┌──────────────────────┐      ┌──────────────────────┐    ┌────────────────────────┐
   │ main()               │      │                      │    │ doomgeneric_Create()   │
   │   doomgeneric_Create ├─────▶│  doomkit.h       ├───▶│ D_DoomMain()  (boot)   │
   │   loop:              │      │  (the contract)      │    │                        │
   │     doomgeneric_Tick ├─────▶│                      ├───▶│ doomgeneric_Tick()     │
   │                      │      │   DG_ScreenBuffer    │◀───┤   i_video.c  (draws)   │
   │ DG_Init              │◀─────┤   DG_DrawFrame       │◀───┤   i_video.c  (presents)│
   │ DG_DrawFrame         │◀─────┤   DG_GetKey          │◀───┤   i_input.c  (reads)   │
   │ DG_SleepMs           │◀─────┤   DG_GetTicksMs      │◀───┤   i_timer.c  (timing)  │
   │ DG_GetTicksMs        │      │   DG_SleepMs         │    │                        │
   │ DG_GetKey            │      │   DG_SetWindowTitle  │    │   ...everything else   │
   │ DG_SetWindowTitle    │      │                      │    │                        │
   └──────────────────────┘      └──────────────────────┘    └────────────────────────┘
        you write this              this package documents          upstream, untouched
```

Everything in the right-hand column is original id Software code. You never edit
it. The middle column — six callbacks, two lifecycle calls, one framebuffer — is
the whole interface, and it is what `include/doomkit/` documents and what
`src/` extracts the testable bits of.

---

## The data flow of a single frame

`doomgeneric_Tick()` (in `d_main.c`) is the heartbeat. One call:

1. **Input** — the engine asks the platform for keys.
   `i_input.c` calls your `DG_GetKey()` in a loop, turning raw key events into
   DOOM events posted to the engine's event queue.
2. **Simulate** — `TryRunTics()` (`d_loop.c`) advances the game world by one or
   more 1/35-second "tics": player movement (`p_user.c`), monster AI
   (`p_enemy.c`), physics and collisions (`p_map.c`, `p_mobj.c`), etc.
3. **Render** — `D_Display()` (`d_main.c`) draws the 3D view and HUD into DOOM's
   internal 8-bit paletted buffer via the renderer (`r_*.c`).
4. **Present** — `I_FinishUpdate()` (`i_video.c`) converts that 8-bit buffer into
   the 32-bit `DG_ScreenBuffer` (the logic our `dg_palette` + `dg_framebuffer`
   helpers clean up) and calls your `DG_DrawFrame()`.

Timing throughout uses your `DG_GetTicksMs()` / `DG_SleepMs()` (via `i_timer.c`).

---

## Subsystem inventory (by filename prefix)

The engine groups files by a two-letter prefix. You rarely need to open these,
but here is what each cluster does:

| Prefix | Role | Representative files |
|--------|------|----------------------|
| `d_*`  | **D**OOM top level: startup, the main/game loop, networking glue | `d_main.c`, `d_loop.c`, `d_net.c`, `d_iwad.c` |
| `g_*`  | **G**ame coordination: skill, episodes, save/load, demos | `g_game.c` |
| `p_*`  | **P**lay simulation: the actual game world | `p_map.c`, `p_enemy.c`, `p_mobj.c`, `p_user.c`, `p_inter.c`, `p_spec.c` |
| `r_*`  | **R**enderer: the BSP software 3D engine | `r_main.c`, `r_bsp.c`, `r_draw.c`, `r_plane.c`, `r_things.c` |
| `i_*`  | **I**nterface to the machine: video, input, sound, timing | `i_video.c`, `i_input.c`, `i_timer.c`, `i_sound.c`, `i_system.c` |
| `w_*`  | **W**AD file access: the game's data archives | `w_wad.c`, `w_file.c`, `w_main.c` |
| `v_*`  | **V**ideo buffer primitives (blitting, patches) | `v_video.c` |
| `m_*`  | **M**isc utilities: menus, config, fixed-point, RNG, args | `m_menu.c`, `m_config.c`, `m_fixed.c`, `m_random.c`, `m_argv.c` |
| `s_*`  | **S**ound orchestration (what to play, when) | `s_sound.c` |
| `st_*` | **St**atus bar (the face, ammo, health) | `st_stuff.c`, `st_lib.c` |
| `hu_*` | **HU**D / heads-up text and messages | `hu_stuff.c`, `hu_lib.c` |
| `wi_*` | **W**rap-up **I**ntermission screens (between levels) | `wi_stuff.c` |
| `f_*`  | **F**inale text and the screen-melt wipe | `f_finale.c`, `f_wipe.c` |
| `am_*` | **A**uto**m**ap (the overhead map) | `am_map.c` |
| `z_*`  | **Z**one memory allocator (DOOM's own heap) | `z_zone.c` |
| `info.c` | Huge data tables: every monster/weapon state | `info.c` |

The only files that *touch the porting seam* are `i_video.c` (calls
`DG_DrawFrame`, fills `DG_ScreenBuffer`), `i_input.c` (calls `DG_GetKey`),
`i_timer.c` (calls `DG_GetTicksMs`/`DG_SleepMs`), plus `doomgeneric.c` (defines
`doomgeneric_Create`) and `d_main.c` (defines `doomgeneric_Tick`). Those are the
files this package distils.

---

## Where the cleaned helpers come from

Each `src/` helper is a clean, isolated, unit-tested extraction of logic that
lives tangled inside the engine's `i_video.c` / platform ports:

| Helper (`src/`) | Extracted from | What it isolates |
|-----------------|----------------|------------------|
| `dg_keyqueue.c` | the key ring buffer in `doomgeneric_sdl.c` | OS-thread → engine-thread event hand-off |
| `dg_keymap.c`   | `convertToDoomKey()` in `doomgeneric_sdl.c` | host key code → DOOM key code |
| `dg_palette.c`  | `cmap_to_fb()` / `I_SetPalette()` in `i_video.c` | 8-bit paletted → RGB conversion |
| `dg_framebuffer.c` | the offset math in `I_FinishUpdate()` in `i_video.c` | centring/scaling layout |

See `docs/CONTRACT.md` for the precise callback semantics and `docs/PORTING.md`
for the step-by-step porting walkthrough.
