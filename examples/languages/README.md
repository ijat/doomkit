# Run doomkit from other languages

DOOM is C, but you can drive it from almost any language because the engine is
exposed through a tiny flat C ABI (see [`../../bindings/`](../../bindings)). The
recipe is the same everywhere:

1. **Build `libdoomgeneric` once** — `make lib ENGINE=/path/to/doomgeneric/doomgeneric`
   from the package root (details in [`../../bindings/README.md`](../../bindings/README.md)).
2. **Load it from your language** and register six callbacks.
3. **Loop**: `dg_create(argc, argv)` once, then `dg_tick()` repeatedly.

Every example here is the *same* tiny headless program — it boots the engine,
runs ~200 frames, and writes `frame.ppm` at frame 100 so you can confirm real
pixels came out. Only the FFI mechanism differs.

## Pick your language

| Language | FFI mechanism | Folder | Notes |
|----------|---------------|--------|-------|
| C | link + register | [`c/`](c) | the baseline |
| C++ | link + register | [`cpp/`](cpp) | RAII framebuffer view; non-capturing callbacks |
| Go | cgo + `//export` | [`go/`](go) | needs a `bridge.c` helper |
| C# | P/Invoke | [`csharp/`](csharp) | keep delegates rooted |
| Java | Panama FFM (JDK 22+) | [`java/`](java) | pure Java, no JNI |
| Python | ctypes (stdlib) | [`python/`](python) | shortest binding |
| Rust | `extern "C"` | [`rust/`](rust) | no crates needed |
| Node.js | koffi (FFI) | [`nodejs/`](nodejs) | `npm install`, no native build |
| Kotlin/Android | NDK + JNI | [`../kotlin-android/`](../kotlin-android) | engine compiled into the app `.so` |

## The shared shape, in pseudocode

```
register({ init, draw_frame, sleep_ms, get_ticks_ms, get_key, set_window_title })
create(argv)                 # boots engine, calls your init, loads the WAD
loop: tick()                 # each tick ends by calling your draw_frame

# inside draw_frame: read dg_screen_buffer() -> RESX*RESY pixels of 0x00RRGGBB
# inside get_key:    return one queued (pressed, doomKey) event, or 0
```

## Two rules that bite everyone

1. **Pixel format is `0x00RRGGBB`** (32-bit). If red/blue look swapped, your
   display wants a different byte order — repack per pixel (Android does this).
2. **Keep your callbacks alive.** In GC'd languages (C#, Python, Java, Go) the
   runtime must not collect the function objects while the engine holds pointers
   to them. Each example shows where to root them.

## What's verified

The C ABI and shared library are real and tested: in this repo's environment the
**C, C++, Python, Rust, C#, and Node.js** bindings were compiled/loaded against
the actual `libdoomgeneric` and call through it successfully. **Go, Java, and
Kotlin/Android** are provided as correct reference code but need their own
toolchains (Go, JDK 22, Android SDK/NDK) to build. None *play* DOOM without a WAD
and the upstream engine — they are bindings, not a packaged game.
