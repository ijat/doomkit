# The porting contract — precise semantics

This is the formal reference for every function and value in
`include/genericdoom/genericdoom.h`. If the header comments are the "what", this
is the "exactly how".

---

## Shared state

### `pixel_t *DG_ScreenBuffer`
- **Owner:** the engine. Allocated in `doomgeneric_Create()`, never freed by you.
- **Size:** exactly `DOOMGENERIC_RESX * DOOMGENERIC_RESY` pixels, row-major,
  top-to-bottom, left-to-right.
- **Pixel format:** `uint32_t` as `0x00RRGGBB` by default; `uint8_t` palette
  index if built with `-DCMAP256`.
- **When valid:** holds a complete frame *during* your `DG_DrawFrame()` call.
  Don't read it at other times.

---

## Lifecycle (the engine provides these)

### `void doomgeneric_Create(int argc, char **argv)`
Call **once**, before the loop. Side effects, in order: stores `argc`/`argv`;
processes a `@response` file; allocates `DG_ScreenBuffer`; calls your
`DG_Init()`; runs `D_DoomMain()` which loads the WAD and initialises every
subsystem. Reads command-line flags such as `-iwad`, `-window`, `-warp`.

### `void doomgeneric_Tick(void)`
Call **repeatedly**. One call advances input → simulation → render → present and
ends by invoking your `DG_DrawFrame()`. It does **not** sleep; you pace the loop.

---

## Callbacks (you provide these)

| Callback | Called | Must do | Must NOT do |
|----------|--------|---------|-------------|
| `void DG_Init(void)` | once, inside `doomgeneric_Create` | open window/framebuffer (`DOOMGENERIC_RESX`×`RESY`), init input | draw a frame |
| `void DG_DrawFrame(void)` | once per tick | copy `DG_ScreenBuffer` to screen; pump OS events into your key queue | block for long / free the buffer |
| `void DG_SleepMs(uint32_t ms)` | as needed | sleep ~`ms` ms | busy-wait |
| `uint32_t DG_GetTicksMs(void)` | frequently | return monotonic ms since start | go backwards / wrap early |
| `int DG_GetKey(int *pressed, unsigned char *doomKey)` | drained in a loop each tick | return 1 + set `*pressed`(0/1) and `*doomKey`, or return 0 when empty | block |
| `void DG_SetWindowTitle(const char *title)` | once (and on title change) | set window title (optional) | — |

### `DG_GetKey` in detail
The engine calls it in a `while (DG_GetKey(...))` loop, so you return **one event
per call** and `0` to signal "no more". `*doomKey` must be a code from
`dg_keys.h` (translate host codes with `dg_keymap.h`). `*pressed` is `1` for
key-down, `0` for key-up. The provided `dg_keyqueue` makes this a one-liner:

```c
int DG_GetKey(int *pressed, unsigned char *doomKey) {
    return dg_keyqueue_pop(&my_queue, pressed, doomKey);
}
```

---

## Threading & timing notes
- The engine is single-threaded and calls all callbacks from the thread that
  calls `doomgeneric_Tick()`. If your OS delivers input on another thread, that
  is the one place you may need a lock around the key queue.
- DOOM simulates at a fixed **35 Hz**. `DG_GetTicksMs` is the clock that keeps it
  honest; small inaccuracies are fine, monotonicity is not optional.
- Aim to call `doomgeneric_Tick()` at least 35×/second. Faster is fine — the
  engine renders interpolated frames and only steps the simulation on tic
  boundaries.
