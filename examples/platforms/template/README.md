# template — start a new port here

A blank skeleton: copy it, fill in **six TODOs**, and you have a doomkit port for
your platform. That's the whole job.

## Use it

```sh
cp examples/platforms/template/platform_template.c platform_myplatform.c
# then open platform_myplatform.c and fill in the six TODOs
```

The file already contains `main()` and empty versions of the six callbacks,
pre-wired to the `dg_keyqueue` helper, so you only write the platform-specific
bits:

| # | Callback | Fill in |
|---|----------|---------|
| 1 | `DG_Init` | open a window/framebuffer of `DOOMGENERIC_RESX`×`RESY`; init input |
| 2 | `DG_DrawFrame` | copy `DG_ScreenBuffer` (`0x00RRGGBB` pixels) to your screen; queue key events |
| 3 | `DG_SleepMs` | sleep `ms` milliseconds |
| 4 | `DG_GetTicksMs` | return a monotonic millisecond counter |
| 5 | `DG_GetKey` | one-liner: `return dg_keyqueue_pop(&q, pressed, doomKey);` |
| 6 | `DG_SetWindowTitle` | set the title (or leave empty) |

## When you're stuck

- The step-by-step walkthrough is [`docs/PORTING.md`](../../../docs/PORTING.md).
- The exact meaning of each callback is in
  [`docs/CONTRACT.md`](../../../docs/CONTRACT.md) and the contract header
  [`include/doomkit/doomkit.h`](../../../include/doomkit/doomkit.h).
- Two worked references: [`../null/`](../null) (runnable, no deps) and
  [`../sdl/`](../sdl) (a real window). Your file will look just like them, with
  your platform's API in the blanks.

You also need the upstream DOOM engine sources to link a playable binary — see
the root [README](../../../README.md#running-real-doom).
