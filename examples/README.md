# Examples

Each example is one *port* — an implementation of the six `DG_*` callbacks for a
particular backend. They all have the same shape; only the backend differs.

| File | Backend | Deps | Built by `make`? | What it shows |
|------|---------|------|------------------|----------------|
| [`null/platform_null.c`](null/platform_null.c) | none (headless) | none | ✅ `make run-null` | The whole contract running with a *fake engine*; writes `frame.ppm`. The best place to start reading. |
| [`sdl/platform_sdl.c`](sdl/platform_sdl.c) | SDL2 | SDL2 | ❌ (needs SDL + engine) | A real windowed port, using the key helpers. |
| [`template/platform_template.c`](template/platform_template.c) | yours | none | ❌ | A blank skeleton with six TODOs — copy this to start a new port. |
| [`minimal_main.c`](minimal_main.c) | — | — | ❌ | The canonical `Create()` + `Tick()` loop on its own. |
| [`wasm/platform_wasm.c`](wasm/) | browser (WebAssembly) | Emscripten | ✅ `make wasm` | Runs DOOM in a `<canvas>`; pick a WAD in the page. |
| [`kotlin-android/`](kotlin-android/) | Android (Kotlin) | NDK/JNI | ❌ | Renders to a `Surface`; engine compiled into the app `.so`. |

> The language-FFI examples (drive a prebuilt `libdoomgeneric` from Go, C#,
> Java, Python, Rust, …) live one level down in
> [`languages/`](languages/).

## Why are some not built by `make`?

`make run-null` builds and runs the headless example because it needs nothing
external. The SDL and template examples need either a library (SDL2) **and/or**
the actual DOOM engine sources (which provide `doomgeneric_Create()` /
`doomgeneric_Tick()`) to link into a playable binary. This package intentionally
does **not** vendor the 73k-line engine — see the root `README.md` for how to
drop it in.

## The pattern every port follows

```
DG_Init()           open display + input
DG_DrawFrame()      blit DG_ScreenBuffer, then pump OS events into the key queue
DG_SleepMs()        sleep
DG_GetTicksMs()     monotonic ms clock
DG_GetKey()         pop one event from the key queue  (one-liner via dg_keyqueue)
DG_SetWindowTitle() optional
main()              doomgeneric_Create(); for(;;) doomgeneric_Tick();
```

Read `null/platform_null.c` first (it is heavily commented and runnable), then
copy `template/platform_template.c` and fill in the six TODOs for your platform.
